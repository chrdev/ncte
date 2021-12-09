#include "commdlg.h"

#include <Dlgs.h>

#include <stdbool.h>
#include <assert.h>

#include "resdefs.h"
#include "debug.h"
#include "sys.h"


typedef struct CustData {
	wchar_t driveLetter;
	FileType fileType;
}CustData;


static inline bool
isCdDrive(const wchar_t* path) {
	return DRIVE_CDROM == GetDriveType(path);
}

static inline void
showExtract(HWND dlg, bool show) {
	int cmd = show ? SW_SHOWNA : SW_HIDE;
	ShowWindow(dlg, cmd);
}

static void
layout(HWND dlg) {
	RECT refRc, rc;
	HWND parent = GetParent(dlg);
	
	GetWindowRect(GetDlgItem(parent, IDCANCEL), &refRc);
	GetWindowRect(GetDlgItem(dlg, kIdExtract), &rc);
	const offset = refRc.right - rc.right;
	GetWindowRect(dlg, &rc);
	MapWindowPoints(NULL, parent, (POINT*)&rc, 1);
	SetWindowPos(dlg, NULL, rc.left + offset, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_NOSIZE);
}

static inline void
handleSelChange(HWND dlg, CustData* custData) {
	enum { kCchPath = 4 }; // eg. L"E:\"
	wchar_t path[kCchPath];
	HWND parent = GetParent(dlg);
	HWND btn = GetDlgItem(dlg, kIdExtract);
	int cch = (int)SendMessage(parent, CDM_GETFILEPATH, 0, 0);
	if (cch != kCchPath) {
		showExtract(dlg, false);
		return;
	}
	SendMessage(parent, CDM_GETFILEPATH, (WPARAM)cch, (LPARAM)path);
	if (!isCdDrive(path)) {
		showExtract(dlg, false);
		return;
	}

	custData->driveLetter = path[0];
	layout(dlg);
	showExtract(dlg, true);
}

static inline void
onNotify(HWND dlg, OFNOTIFY* notify) {
	switch (notify->hdr.code) {
	case CDN_INITDONE:
		SetWindowLongPtr(dlg, GWLP_USERDATA, (LONG_PTR)notify->lpOFN->lCustData);
		break;
	case CDN_SELCHANGE:
	case CDN_FOLDERCHANGE:
		handleSelChange(dlg, (CustData*)notify->lpOFN->lCustData);
		break;
	//case CDN_SHAREVIOLATION:
	//	OutputDebugString(L"   share violation\n");
	//	break;
	//case CDN_HELP:
	//	OutputDebugString(L" help\n");
	//	break;
	//case CDN_FILEOK:
	//	OutputDebugString(L"   file ok \n");
	//	break;
	//case CDN_TYPECHANGE:
	//	OutputDebugString(L"    type chang\n");
	//	break;
	//default:
	//	OutputDebugString(L"    unknown dlg notify\n");
	//	break;
	}
}

static inline void
onWindowPosChanged(HWND dlg, WINDOWPOS* wp) {
	if (!IsWindowVisible(dlg)) return;
	layout(dlg);
}

static inline void
handleExtract(HWND dlg) {
	CustData* custData = (CustData*)GetWindowLongPtr(dlg, GWLP_USERDATA);
	custData->fileType = IsDlgButtonChecked(dlg, kIdSaveDump) ? filetype_kDriveAndDump : filetype_kDrive;
	PostMessage(GetParent(dlg), WM_COMMAND, IDCANCEL, 0);
}

static inline void
onCommand(HWND dlg, WORD id, WORD code, HWND ctl) {
	switch (id) {
	case kIdExtract:
		handleExtract(dlg);
		break;
	}
}

static inline void
initLayout(HWND dlg) {
	// Tests show that dialog template doesn't guarantee correct sizes under different DPIs / OSes, so we need to layout by code.
	HWND parent = GetParent(dlg);
	RECT rc1, rc;
	GetWindowRect(GetDlgItem(parent, cmb1), &rc1);
	GetWindowRect(GetDlgItem(parent, IDCANCEL), &rc);
	const spacingCx = rc.left - rc1.right;
	const buttonCx = rc.right - rc.left;
	const buttonCy = rc.bottom - rc.top;
	const dlgCy = buttonCy + sys_getScrollCy();

	GetWindowRect(GetDlgItem(dlg, kIdSaveDump), &rc);
	const checkCx = rc.right - rc.left;
	SetWindowPos(GetDlgItem(dlg, kIdExtract), NULL, checkCx + spacingCx, 0, buttonCx, buttonCy, SWP_NOZORDER | SWP_NOSENDCHANGING | SWP_NOACTIVATE);
	SetWindowPos(dlg, NULL, 0, 0, 2, dlgCy, SWP_NOZORDER | SWP_NOSENDCHANGING | SWP_NOACTIVATE);
}

static inline void
onInitDialog(HWND dlg) {
	initLayout(dlg);
}

UINT_PTR CALLBACK openHookProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		onInitDialog(dlg);
		return FALSE;
	case WM_NOTIFY:
		onNotify(dlg, (OFNOTIFY*)lParam);
		return TRUE;
	case WM_COMMAND:
		onCommand(dlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
		return TRUE;
	case WM_WINDOWPOSCHANGED:
		onWindowPosChanged(dlg, (WINDOWPOS*)lParam);
		return TRUE;
	}
	return FALSE;
}

FileType
commdlg_getOpenPath(HWND owner, wchar_t* path, DWORD size) {
	static const wchar_t kFilter[] =
		L"Binaries Or Sheets (*.cdt, *.bin;*.txt)\0*.cdt;*.bin;*.txt\0"
		L"Bin Files (*.cdt, *.bin)\0*.cdt;*.bin\0"
		L"Sheet Files (*.txt)\0*.txt\0"
		L"All Files (*.*)\0*.*\0";
	static const DWORD flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_SHAREAWARE | OFN_ENABLESIZING
		| OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST
		| OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;

	CustData custData = { 0 };
	path[0] = L'\0';

	OPENFILENAME ofn = {
		.lStructSize = sizeof(OPENFILENAME),
		.hwndOwner = owner,
		.hInstance = GetModuleHandle(NULL),
		.lpstrFilter = kFilter,
		.nFilterIndex = 1,
		.lpstrFile = path,
		.nMaxFile = size,
		.Flags = flags,
		.lCustData = (LPARAM)&custData,
		.lpfnHook = openHookProc,
		.lpTemplateName = MAKEINTRESOURCE(kDlgOpenPanel),
	};
	if (GetOpenFileName(&ofn)) {
		const wchar_t* ext = ofn.lpstrFile + ofn.nFileExtension;
		if (!lstrcmpi(ext, L"txt")) return filetype_kSheet;
		return filetype_kBin;
	}
	else if (custData.fileType) {
		path[0] = custData.driveLetter;
		path[1] = L'\0'; // No need for a NUL in this case, but follow a good practice anyway.
		return custData.fileType;
	}
	return filetype_kNone;
}

UINT_PTR CALLBACK saveBinHookProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	return FALSE;
}

bool
commdlg_getSaveBinPath(HWND owner, wchar_t* path, DWORD size) {
	static const wchar_t kFilter[] =
		L"Cdt Files (*.cdt)\0*.cdt\0"
		L"Bin Files (*.bin)\0*.bin\0";
	static const DWORD flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_SHAREAWARE | OFN_ENABLESIZING
		| OFN_NOCHANGEDIR | OFN_NOTESTFILECREATE | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST
		| OFN_ENABLEHOOK;

	path[0] = L'\0';
	OPENFILENAME ofn = {
		.lStructSize = sizeof(OPENFILENAME),
		.hwndOwner = owner,
		.hInstance = GetModuleHandle(NULL),
		.lpstrFilter = kFilter,
		.nFilterIndex = 1,
		.lpstrFile = path,
		.nMaxFile = size,
		.Flags = flags,
		.lpstrDefExt = L"cdt",
		.lpfnHook = saveBinHookProc,
	};

	return GetSaveFileName(&ofn);
}
