#include "mainwnd.h"

#include <shellapi.h> // DragAcceptFiles
#pragma comment(lib, "Shell32.lib")
#include <windowsx.h>
//#include <shellscalingapi.h>
//#pragma comment(lib, "shcore.lib")

#include <stdbool.h>
#include <stdlib.h> // calloc, free
#include <stdint.h>

#include "toolbar.h"
#include "navwnd.h"
#include "body.h"
#include "dpi.h"
#include "wnddata.h"
#include "msg.h"
#include "wnd.h"
#include "font.h"
#include "debug.h"
#include "resdefs.h"
#include "albumPanel.h"
#include "thread.h"
#include "cdt.h"
#include "cdtenc.h"
#include "cdtdec.h"
#include "sheet.h"
#include "commdlg.h"
#include "cdrom.h"
#include "msgbox.h"
#include "filepath.h"
#include "clipboard.h"
#include "format.h"
#include "str.h"
#include "msgbox.h"


static const wchar_t kClsName[] = L"NCTEMAINWND";
//static const wchar_t kWndName[] = L"Neo CDText Editor";

enum {
	kMinCx = 720,
	kMinCy = 600,
};

enum MenuId{
	kMenuBegin = 100,
	kMenuAbout = kMenuBegin,
	kMenuEnd
};

enum ChildId{
	kIdBegin = 1000,
	kIdNav = kIdBegin,
	kIdBody,
	kIdEnd
};

static inline bool
customizeSystemMenu(HWND wnd) {
	HMENU root = GetSystemMenu(wnd, FALSE);
	if (!AppendMenu(root, MF_SEPARATOR, 0, NULL)) return false;
	if (!AppendMenu(root, MF_STRING, kMenuAbout, L"&About...")) return false;
	DrawMenuBar(wnd);
	return true;
}

static inline bool
createChildren(HWND parent) {
	HWND wnd = toolbar_create(parent);
	if (!wnd) return false;
	ShowWindow(wnd, SW_SHOWNA);
	if (!navwnd_create(parent, kIdNav)) return false;
	if (!body_create(parent, kIdBody)) return false;
	return true;
}

static inline void
resizeChildren(HWND parent) {
	SIZE sz = wnd_getClientSize(parent);
	HWND wnd = GetDlgItem(parent, kDlgToolbar);
	int cx = sz.cx - wnd_getCx(GetDlgItem(parent, kIdNav));
	int cy = sz.cy - wnd_getCy(wnd);

	wnd_setCx(wnd, sz.cx);
	wnd_setSize(GetDlgItem(parent, kIdBody), cx, cy);
}

static inline void
updateLayout(HWND wnd, int cx, int cy) {
	RECT rc = { 0, 0, kRcToolbarFileW, kRcToolbarH };
	HWND child = GetDlgItem(wnd, kDlgToolbar);
	MapDialogRect(child, &rc);
	SetWindowPos(child, NULL, 0, 0, cx, rc.bottom, SWP_NOZORDER | SWP_NOSENDCHANGING | SWP_NOCOPYBITS | SWP_NOREDRAW);
	const navCx = rc.right;
	const navCy = cy - rc.bottom;
	SetWindowPos(GetDlgItem(wnd, kIdNav), NULL, 0, rc.bottom, navCx, navCy, SWP_NOZORDER | SWP_NOCOPYBITS | SWP_NOREDRAW);
	SetWindowPos(GetDlgItem(wnd, kIdBody), NULL, navCx, rc.bottom, cx - navCx, navCy, SWP_NOZORDER | SWP_NOSENDCHANGING | SWP_NOCOPYBITS | SWP_NOREDRAW);
}

static inline BOOL
onCreate(HWND wnd, LPCREATESTRUCT cs) {
	//PROCESS_DPI_AWARENESS dpia;
	//GetProcessDpiAwareness(NULL, &dpia);
	if (!cdt_init(&theCdt)) return FALSE;
	dpi_refresh(wnd);
	font_init();
	customizeSystemMenu(wnd);

	if (!createChildren(wnd)) return FALSE;
	if (!threads_init(GetDlgItem(wnd, kIdNav))) return false;
	
	SIZE sz = wnd_getClientSize(wnd);
	updateLayout(wnd, sz.cx, sz.cy);

	msg_sendSetTrackRange(GetDlgItem(wnd, kDlgToolbar), 1, 3);
	SendMessage(wnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, UISF_HIDEACCEL | UISF_HIDEFOCUS), 0);
	SetFocus(GetDlgItem(wnd, kIdBody));

	DragAcceptFiles(wnd, TRUE);
	return TRUE;
}

static inline void
onClose(HWND wnd) {
	BOOL ok = DestroyWindow(wnd);
}

static inline void
onDestroy(HWND wnd) {
	cdt_uninit(&theCdt);
	font_uninit();
	threads_uninit();
	PostQuitMessage(0);
}

static inline void
onWindowPosChanged(HWND wnd, const LPWINDOWPOS wp) {
	if (!wnd_sizeChanged(wp)) return;
	resizeChildren(wnd);
}

static inline void
onGetMinMaxInfo(HWND wnd, LPMINMAXINFO mmi) {
	mmi->ptMinTrackSize.x = dpi_scale(kMinCx);
	mmi->ptMinTrackSize.y = dpi_scale(kMinCy);
}

static inline void
onDpiChanged(HWND wnd, int dpi, const RECT* rc) {
	dpi_set(dpi);
	font_refresh();

	InvalidateRect(wnd, NULL, TRUE);
	updateLayout(wnd, rc->right - rc->left, rc->bottom - rc->top);
}

static inline void
showAbout(HWND parent) {
	msgbox_show(parent, kTextAbout, 0, MB_OK);
}

static inline void
saveFocus(HWND wnd) {
	HWND wndFocus = GetFocus();
	if (!wndFocus) return;
	assert(IsChild(wnd, wndFocus));
	SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR)(ULONG_PTR)wndFocus);
}

static inline void
restoreFocus(HWND wnd) {
	HWND wndFocus = (HWND)(ULONG_PTR)GetWindowLongPtr(wnd, GWLP_USERDATA);
	if (!wndFocus) return;
	assert(IsChild(wnd, wndFocus));
	SetFocus(wndFocus);
}

static inline void
onSysCommand(HWND wnd, UINT cmd, int x, int y) {
	switch (cmd) {
	case kMenuAbout:
		showAbout(wnd);
		break;
	case SC_MINIMIZE:
		saveFocus(wnd);
		// fallthrough
	default:
		FORWARD_WM_SYSCOMMAND(wnd, cmd, x, y, DefWindowProc);
		break;
	}
}

static inline void
onShowWindow(HWND wnd, BOOL isShow, UINT status) {
	if (!isShow) saveFocus(wnd);
	FORWARD_WM_SHOWWINDOW(wnd, isShow, status, DefWindowProc);
}

static inline void
onActivate(HWND wnd, UINT state, HWND wndOther, BOOL isMinimized) {
	if (state == WA_INACTIVE) {
		saveFocus(wnd);
	}
	else if (!isMinimized) {
		restoreFocus(wnd);
	}
}

static inline void
refreshUi(HWND wnd, bool showAllFields) {
	msg_sendDataChanged(GetDlgItem(wnd, kIdBody), showAllFields);
	msg_sendDataChanged(GetDlgItem(wnd, kDlgToolbar), showAllFields);
	msg_sendDataChanged(GetDlgItem(wnd, kIdNav), showAllFields);
}

static inline void
handleNew(HWND wnd) {
	if (IDYES != msgbox_show(wnd, kTextPromptAbandonUnsaved, 0, MB_YESNO | MB_ICONEXCLAMATION)) return;
	cdt_reset(&theCdt);
	refreshUi(wnd, true);
}

static inline bool
openSheet(HWND wnd, Cdt* cdt, const wchar_t* path) {
	Sheet f;
	if (!sheet_openForReading(&f, path)) return false;
	bool ok = sheet_read(f, cdt);
	sheet_close(f);
	return ok;
}

static inline bool
openBin(HWND wnd, Cdt* cdt, const wchar_t* path) {
	CdtDecContext ctx = cdt_createDecContext();
	if (!ctx) return false;
	int blockCount = cdt_decodeFile(path, cdt, ctx);
	cdt_destroyDecContext(ctx);
	return blockCount;
}

static inline bool
decodeBin(Cdt* cdt, const uint8_t* bin, int size) {
	CdtDecContext ctx = cdt_createDecContext();
	if (!ctx) return false;
	int blockCount = cdt_decode(bin + sizeof(DWORD), size - sizeof(DWORD) + 1, cdt, ctx);
	cdt_destroyDecContext(ctx);
	return blockCount;
}

static inline void
saveDump(HWND wnd, const void* bin, int size) {
	wchar_t path[MAX_PATH];
	if (!commdlg_getSaveBinPath(wnd, path, ARRAYSIZE(path))) return;
	cd_saveTextDump(path, bin, size);
}

static inline bool
openDrive(HWND wnd, Cdt* cdt, wchar_t driveLetter, bool save) {
	int error;
	HANDLE h = cd_open(driveLetter, &error);
	if (h == INVALID_HANDLE_VALUE) {
		if (error == cd_kNoCd) msgbox_show(wnd, kTextCdNoCd, 0, MB_OK | MB_ICONINFORMATION);
		else msgbox_show(wnd, kTextCdOtherError, 0, MB_OK | MB_ICONINFORMATION);
		return false;
	}

	bool result = false;
	int size;
	uint8_t* bin = cd_dumpText(h, &size, &error);
	if (bin) {
		if (save) saveDump(wnd, bin, size);
		result = decodeBin(cdt, bin, size);
		HeapFree(GetProcessHeap(), 0, bin);
		cd_close(h);
	}
	else {
		cd_close(h);
		if (error == cd_kNoText) msgbox_show(wnd, kTextCdNoText, 0, MB_OK | MB_ICONINFORMATION);
		else if (error == cd_kOther) msgbox_show(wnd, kTextCdOtherError, 0, MB_OK | MB_ICONINFORMATION);
	}
	return result;
}

static inline void
mergeCdt(HWND wnd, Cdt* cdt) {
	int start = msg_sendGetNav(GetDlgItem(wnd, kIdNav));
	cdt_transfer(&theCdt, start, cdt);
	refreshUi(wnd, false);
}

static inline void
handleOpenFile(HWND wnd) {
	Cdt cdt;
	if (!cdt_init(&cdt)) return;

	bool ok = false;
	wchar_t path[MAX_PATH];
	int rc = commdlg_getOpenPath(wnd, path, ARRAYSIZE(path));
	switch (rc) {
	case filetype_kBin:
		ok = openBin(wnd, &cdt, path);
		break;
	case filetype_kSheet:
		ok = openSheet(wnd, &cdt, path);
		break;
	case filetype_kDrive:
		ok = openDrive(wnd, &cdt, path[0], false);
		break;
	case filetype_kDriveAndDump:
		ok = openDrive(wnd, &cdt, path[0], true);
		break;
	}

	if (ok) {
		if (cdt_setTrackRange(&theCdt, cdt.trackFirst, cdt.trackLast)) {
			mergeCdt(wnd, &cdt);
		}
	}
	cdt_uninit(&cdt);
}

static inline void
handleSave(HWND wnd) {
	if (!commdlg_getSaveBinPath(wnd, thePath, filepath_kCch)) return;
	//BYTE albumMask = msg_sendGetFields(wnd, fields_kAlbum);
	//BYTE trackMask = msg_sendGetFields(wnd, fields_kTrack);
	//CdtEncContext ctx = cdt_createEncContext(albumMask, trackMask);
	CdtEncContext ctx = cdt_createEncContext();
	if (!ctx) return;
	bool ok = false;
	if (cdt_encode(&theCdt, ctx)) {
		ok = cdt_saveBin(thePath, ctx);
	}
	cdt_destroyEncContext(ctx);
	if (!ok) msgbox_show(wnd, kTextFileSaveError, 0, MB_OK | MB_ICONERROR);
}

static void
handleOpenDrop(HWND wnd, HDROP drop) {
	Cdt cdt;
	cdt_init(&cdt);
	UINT count = DragQueryFile(drop, (UINT)-1, NULL, 0);
	for (UINT i = 0; i < count; ++i) {
		if (DragQueryFile(drop, i, thePath, filepath_kCch) < 6) continue;
		bool ok = false;
		if (filepath_hasExt(thePath, L"txt")) {
			ok = openSheet(wnd, &cdt, thePath);
		}
		else {
			ok = openBin(wnd, &cdt, thePath);
		}
		if (ok) mergeCdt(wnd, &cdt);
	}
	cdt_uninit(&cdt);
}

static inline void
copyBlock(HWND wnd, int srcIndex) {
	int dstIndex = msg_sendGetNav(GetDlgItem(wnd, kIdNav));
	if (!cdt_copyBlock(&theCdt, srcIndex, dstIndex)) return;

	msg_sendDataChanged(GetDlgItem(wnd, kIdBody), false);
	msg_sendDataChanged(GetDlgItem(wnd, kIdNav), false);
}

static inline void
pasteSheet(HWND wnd, const wchar_t* text) {
	Sheet sheet = sheet_fromWcs(text, lstrlen(text));
	if (!sheet) return;

	bool ok = false;
	Cdt cdt;
	if (cdt_init(&cdt)) {
		ok = sheet_readWcs(sheet, &cdt);
	}
	sheet_close(sheet);
	if (ok) {
		mergeCdt(wnd, &cdt);
	}

	cdt_uninit(&cdt);
}

//static inline void
//pasteEac(HWND wnd, const wchar_t* text) {
//	Cdt cdt;
//	if (!cdt_init(&cdt)) return;
//	if (eac_parse(&cdt, text)) {
//		int blockIndex = msg_sendGetNav(GetDlgItem(wnd, kIdNav));
//		int count = cdt_transfer(&theCdt, blockIndex, &cdt);
//		if (count) refreshUi(wnd);
//	}
//	cdt_uninit(&cdt);
//}

static inline void
handleOpenText(HWND wnd, const wchar_t* text) {
	switch (format_getFormat(text)) {
	case format_kIndex:
		copyBlock(wnd, format_getIndex(text));
		return;
	case format_kSheet:
		pasteSheet(wnd, text);
		return;
	//case format_kEac:
	//	pasteEac(wnd, text);
	//	return;
	};
}

static inline void
onCommand(HWND wnd, int command, LPARAM lp) {
	switch (command) {
	case command_kNew:
		handleNew(wnd);
		break;
	case command_kOpenFile:
		handleOpenFile(wnd);
		break;
	case command_kSave:
		handleSave(wnd);
		break;
	case command_kOpenDrop:
		handleOpenDrop(wnd, (HDROP)lp);
		break;
	case command_kOpenText:
		handleOpenText(wnd, (const wchar_t*)lp);
		break;
	}
}

static inline void
onFocusNextCtl(HWND wnd, int id, bool prev) {
	switch (id) {
	case kDlgTrack:
		if (prev) {
			HWND parent = GetDlgItem(GetDlgItem(wnd, kIdBody), kDlgAlbum);
			int ctlId = albumPanel_getBottomEditId(parent);
			HWND focus = GetDlgItem(parent, ctlId);
			assert(focus);
			wnd_focusThisCtl(parent, focus);
		}
		else {
			SetFocus(GetDlgItem(wnd, kIdNav));
		}
		break;
	}
}

static inline void
onDropFiles(HWND wnd, HDROP drop) {
	handleOpenDrop(wnd, drop);
	DragFinish(drop);
}

static inline void
onCompletenessChanged(HWND wnd, bool allBlocks) {
	msg_sendCompletenessChanged(GetDlgItem(wnd, kIdNav), allBlocks);
	msg_sendCompletenessChanged(GetDlgItem(wnd, kDlgToolbar), allBlocks);
}

static inline bool
onDataChanged(HWND wnd, bool showAllFields) {
	refreshUi(wnd, showAllFields);
	return true;
}

static inline void
handleCopyAsCue(HWND wnd, const cdt_Block* block) {
	format_copy(wnd, block, theCdt.toc, cue_blockToText);
}

static inline void
handleCopyAsEac(HWND wnd, const cdt_Block* block) {
	format_copy(wnd, block, theCdt.toc, eac_blockToText);
}

static inline void
onCopy(HWND wnd, int format, int blockIndex) {
	const cdt_Block* block = theCdt.blocks + blockIndex;
	switch (format) {
	case format_kCue:
		handleCopyAsCue(wnd, block);
		break;
	case format_kEac:
		handleCopyAsEac(wnd, block);
		break;
	}
}

static LRESULT WINAPI
wndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		HANDLE_MSG(wnd, WM_CREATE, onCreate);
		HANDLE_MSG(wnd, WM_CLOSE, onClose);
		HANDLE_MSG(wnd, WM_DESTROY, onDestroy);
		HANDLE_MSG(wnd, WM_WINDOWPOSCHANGED, onWindowPosChanged);
		HANDLE_MSG(wnd, WM_GETMINMAXINFO, onGetMinMaxInfo);
		HANDLE_MSG(wnd, WM_SYSCOMMAND, onSysCommand);
		HANDLE_MSG(wnd, WM_SHOWWINDOW, onShowWindow);
		HANDLE_MSG(wnd, WM_ACTIVATE, onActivate);
		HANDLE_MSG(wnd, WM_DROPFILES, onDropFiles);
		MSG_HANDLE_WM_DPICHANGED(wnd, onDpiChanged);
		MSG_HANDLE_COMMAND(wnd, onCommand);
		MSG_HANDLE_FOCUSNEXTCTL(wnd, onFocusNextCtl);
		MSG_HANDLE_COMPLETENESSCHANGED(wnd, onCompletenessChanged);
		MSG_HANDLE_DATACHANGED(wnd, onDataChanged);
		MSG_HANDLE_COPY(wnd, onCopy);
		MSG_FORWARD_NAV(GetDlgItem(wnd, kIdBody));
		MSG_FORWARD_SETTRACKRANGE(GetDlgItem(wnd, kIdBody));
		MSG_FORWARD_SETFIELDS(GetDlgItem(wnd, kIdBody));
		MSG_FORWARD_GETFIELDS(GetDlgItem(wnd, kIdBody));
		MSG_FORWARD_GETNAV(GetDlgItem(wnd, kIdNav));
		MSG_FORWARD_LAYOUTCHANGED(GetDlgItem(wnd, kDlgToolbar));
		MSG_FORWARD_GETLAYOUTHINT(GetDlgItem(wnd, kIdBody));
	case WM_PARENTNOTIFY:
		OutputDebugString(L"mainwnd parent notify\n");
		return 0;
	}
	return DefWindowProc(wnd, msg, wParam, lParam);
}

static inline bool
registerClass(void) {
	WNDCLASSEX wcx = {
		.cbSize = sizeof(WNDCLASSEX),
		.style = 0,
		.lpfnWndProc = wndProc,
		.hInstance = GetModuleHandle(NULL),
		.hIcon = LoadIcon(NULL, IDI_WINLOGO),
		.hCursor = LoadCursor(NULL, IDC_ARROW),
		.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1),
		.lpszMenuName = NULL,
		.lpszClassName = kClsName,
		.hIconSm = LoadIcon(NULL, IDI_WINLOGO),
	};
	return RegisterClassEx(&wcx);
}

HWND
mainwnd_create(void) {
	if (!registerClass()) return NULL;

	return CreateWindowEx(
		WS_EX_NOPARENTNOTIFY | WS_EX_APPWINDOW | WS_EX_OVERLAPPEDWINDOW | WS_EX_CONTROLPARENT,
		kClsName, wcs_load(NULL, kTextMainWndTitle),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, GetModuleHandle(NULL), NULL);
}
