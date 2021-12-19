#include "toolbar.h"

#include <windowsx.h>

#include <stdbool.h>
#include <stdlib.h> // _countof

#include "msg.h"
#include "commctrl.h"
#include "wnd.h"
#include "font.h"
#include "fieldsSelector.h"
#include "resdefs.h"
#include "str.h"
#include "cdt.h"
#include "sys.h"
#include "debug.h"
#include "color.h"
#include "bit.h"


// USERDATA usage
// 0x    00          00            00                 00
//             trackRangeValid          editsChangeNotificationEnabled

static inline void
setTrackRangeValid(HWND wnd, bool valid) {
	ULONG_PTR v = (ULONG_PTR)GetWindowLongPtr(wnd, GWLP_USERDATA);
	if (valid) v |= 0x00010000;
	else v &= 0x11001111;
	SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR)v);
}

static inline bool
isTrackRangeValid(HWND wnd) {
	return (DWORD)GetWindowLongPtr(wnd, GWLP_USERDATA) & 0x00010000;
}

static inline void
enableChangeNotification(HWND wnd, bool enable) {
	ULONG_PTR v = (ULONG_PTR)GetWindowLongPtr(wnd, GWLP_USERDATA);
	if (enable) v |= 0x01;
	else v &= 0x00;
	SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR)v);
}

static inline bool
isChangeNotificationEnabled(HWND wnd) {
	return LOBYTE(GetWindowLongPtr(wnd, GWLP_USERDATA));
}

static inline void
setIconFont(HWND parent) {
	dlg_setItemFont(parent, kIdNew, theFonts.icon.handle, setFont_kNoRedraw);
	dlg_setItemFont(parent, kIdOpen, theFonts.icon.handle, setFont_kNoRedraw);
	dlg_setItemFont(parent, kIdSave, theFonts.icon.handle, setFont_kNoRedraw);

	dlg_setItemFont(parent, kIdAlbumFields, theFonts.icon.handle, setFont_kNoRedraw);
	dlg_setItemFont(parent, kIdTrackFields, theFonts.icon.handle, setFont_kNoRedraw);
}

static inline void
initButtons(HWND parent) {
	setIconFont(parent);
	EnableWindow(GetDlgItem(parent, kIdSave), FALSE);

}

static inline void
initTrackEdits(HWND parent) {
	HWND wnd;
	wnd = GetDlgItem(parent, kIdTrackFirstSpin);
	commctrl_updownSetRange(wnd, 1, 99);
	commctrl_updownSetPos(wnd, 1);
	wnd = GetDlgItem(parent, kIdTrackLastSpin);
	commctrl_updownSetRange(wnd, 1, 99);
	commctrl_updownSetPos(wnd, 1);

	wnd = GetDlgItem(parent, kIdTrackFirst);
	commctrl_editSetLimitText(wnd, 2);
	wnd = GetDlgItem(parent, kIdTrackLast);
	commctrl_editSetLimitText(wnd, 2);

	enableChangeNotification(parent, true);
	setTrackRangeValid(parent, true);
}

static inline void
createTooltips(HWND parent) {
	HWND ctl = commctrl_createTooltip(parent, kIdAlbumFields, kTextFieldsSelectorHelp);
	SendMessage(ctl, TTM_SETMAXTIPWIDTH, 0, 300);
	ctl = commctrl_createTooltip(parent, kIdTrackFields, kTextFieldsSelectorHelp);
	SendMessage(ctl, TTM_SETMAXTIPWIDTH, 0, 300);
}

static inline BOOL
onInitDialog(HWND dlg, HWND focusWnd, LPARAM lp) {
	initButtons(dlg);
	initTrackEdits(dlg);
	createTooltips(dlg);

	return FALSE;
}

static inline void
onPaint(HWND dlg) {
	PAINTSTRUCT ps;
	HDC dc = BeginPaint(dlg, &ps);

	RECT rc;
	GetClientRect(dlg, &rc);
	DrawEdge(dc, &rc, EDGE_RAISED, BF_BOTTOM);

	EndPaint(dlg, &ps);
	dlg_setResult(dlg, 0);
}

static inline bool
validateTrackRange(HWND parent, int* first, int* last) {
	BOOL ok;
	*first = (int)GetDlgItemInt(parent, kIdTrackFirst, &ok, FALSE);
	if (!ok) return false;
	*last = (int)GetDlgItemInt(parent, kIdTrackLast, &ok, FALSE);
	if (!ok) return false;

	if (*first > 0 && *first <= *last && *last < 100) return true;
	return false;
}

static inline void
refreshEditsValidStatus(HWND wnd) {
	InvalidateRect(GetDlgItem(wnd, kIdTrackFirst), NULL, FALSE);
	InvalidateRect(GetDlgItem(wnd, kIdTrackLast), NULL, FALSE);
}

static inline void
handleTrackRangeChanged(HWND wnd) {
	int first, last;
	if (validateTrackRange(wnd, &first, &last)) {
		if (msg_sendSetTrackRange(GetParent(wnd), first, last)) {
			SetDlgItemText(wnd, kIdTrackCount, wcs_loadf(NULL, kTextTrackCount, last - first + 1));
			if (!isTrackRangeValid(wnd)) {
				setTrackRangeValid(wnd, true);
				refreshEditsValidStatus(wnd);
			}
			msg_sendCompletenessChanged(GetParent(wnd), true);
		}
		else {
			if (isTrackRangeValid(wnd)) {
				setTrackRangeValid(wnd, false);
				refreshEditsValidStatus(wnd);
			}
		}
	}
	else {
		if (isTrackRangeValid(wnd)) {
			SetDlgItemText(wnd, kIdTrackCount, wcs_loadf(NULL, kTextTrackCount,0));
			setTrackRangeValid(wnd, false);
			refreshEditsValidStatus(wnd);
		}
	}
}


static inline void
showAlbumFieldsSelector(HWND parent) {
	RECT rc;
	GetWindowRect(GetDlgItem(parent, kIdAlbumFields), &rc);
	int x = sys_isDropdownMenuRtl() ? rc.right : rc.left;
	albumFieldsSelector_popup(parent, x, rc.bottom);
}

static inline void
showTrackFieldsSelector(HWND parent) {
	RECT rc;
	GetWindowRect(GetDlgItem(parent, kIdTrackFields), &rc);
	int x = sys_isDropdownMenuRtl() ? rc.right : rc.left;
	trackFieldsSelector_popup(parent, x, rc.bottom);
}

static inline void
handleButtonClicked(HWND wnd, int id, HWND ctl) {
	switch (id) {
	case kIdAlbumFields:
		showAlbumFieldsSelector(wnd);
		break;
	case kIdTrackFields:
		showTrackFieldsSelector(wnd);
		break;
	case kIdNew:
		msg_sendCommand(GetParent(wnd), command_kNew, 0);
		break;
	case kIdOpen:
		msg_sendCommand(GetParent(wnd), command_kOpenFile, 0);
		break;
	case kIdSave:
		msg_sendCommand(GetParent(wnd), command_kSave, 0);
		break;
	}
}

static inline void
onCommand(HWND wnd, int id, HWND ctl, UINT code) {
	switch (code) {
	case EN_CHANGE:
		if (isChangeNotificationEnabled(wnd)) handleTrackRangeChanged(wnd);
		break;
	case BN_CLICKED:
		handleButtonClicked(wnd, id, ctl);
		break;
	}
	dlg_setResult(wnd, 0);
}

static inline HBRUSH
onCtlColorEdit(HWND wnd, HDC dc, HWND ctl) {
	if (isTrackRangeValid(wnd)) {
		return GetSysColorBrush(COLOR_WINDOW);
	}
	else {
		SetDCBrushColor(dc, kColorInvalid);
		return GetStockObject(DC_BRUSH);
	}
}

static inline void
onSetTrackRange(HWND wnd, int first, int last) {
	enableChangeNotification(wnd, false);
	SetDlgItemInt(wnd, kIdTrackFirst, first, FALSE);
	SetDlgItemInt(wnd, kIdTrackLast, last, FALSE);
	enableChangeNotification(wnd, true);
	handleTrackRangeChanged(wnd);
	dlg_setResult(wnd, isTrackRangeValid(wnd));
}

static inline void
onCompletenessChanged(HWND wnd, bool allBlocks) {
	HWND ctl = GetDlgItem(wnd, kIdSave);
	EnableWindow(ctl, cdt_canSave(&theCdt));
}

static inline void
onDataChanged(HWND wnd, bool showAllFields) {
	enableChangeNotification(wnd, false);
	SetDlgItemInt(wnd, kIdTrackFirst, theCdt.trackFirst, FALSE);
	SetDlgItemInt(wnd, kIdTrackLast, theCdt.trackLast, FALSE);
	enableChangeNotification(wnd, true);

	SetDlgItemText(wnd, kIdTrackCount, wcs_loadf(NULL, kTextTrackCount, theCdt.trackLast - theCdt.trackFirst + 1));
	setTrackRangeValid(wnd, true);
	EnableWindow(GetDlgItem(wnd, kIdSave), cdt_canSave(&theCdt));

	msg_sendSetFields(wnd, fields_kAlbum, showAllFields ? 0xFF : cdt_getAlbumFields(&theCdt));
	msg_sendSetFields(wnd, fields_kTrack, showAllFields ? 0xFF : cdt_getTrackFields(&theCdt));

	dlg_setResult(wnd, 1);
}

static inline void
layout(HWND wnd, HWND refWNd, WORD* data) {
	assert(data[0] == 4);
	RECT rc;
	HWND ctl = GetDlgItem(wnd, kIdTrackLabel);
	GetWindowRect(ctl, &rc);
	int offset = (int)data[1] - rc.left;
	wnd_offsetX(ctl, offset);
	wnd_offsetX(GetDlgItem(wnd, kIdTrackCount), offset);

	ctl = GetDlgItem(wnd, kIdTrackFirst);
	GetWindowRect(ctl, &rc);
	offset = (int)data[2] - rc.left;
	wnd_offsetX(ctl, offset);
	wnd_offsetX(GetDlgItem(wnd, kIdTrackFirstSpin), offset);
	wnd_offsetX(GetDlgItem(wnd, kIdTrackHyphen), offset);
	wnd_offsetX(GetDlgItem(wnd, kIdTrackLast), offset);
	wnd_offsetX(GetDlgItem(wnd, kIdTrackLastSpin), offset);

	ctl = GetDlgItem(wnd, kIdAlbumFields);
	GetWindowRect(ctl, &rc);
	offset = (int)data[3] - rc.left;
	wnd_offsetX(ctl, offset);
	wnd_offsetX(GetDlgItem(wnd, kIdTrackFields), offset);
}

static inline void
onLayoutChanged(HWND wnd, WORD id) {
	assert(id == kDlgAlbum);
	WORD data[4] = { 4 };
	HWND refWnd = msg_sendGetLayoutHint(GetParent(wnd), GetDlgCtrlID(wnd), data);
	POINT pt[] = {
		{ data[1], 0 },
		{ data[2], 0 },
		{ data[3], 0 },
	};
	layout(wnd, refWnd, data);
}

static inline void
onDpiChangedAfterParent(HWND wnd) {
	setIconFont(wnd);
}

static inline BYTE
getFilledFields(HWND wnd, int fieldsClass) {
	switch (fieldsClass) {
	case fields_kAlbum: return cdt_getAlbumFields(&theCdt);
	case fields_kTrack: return cdt_getTrackFields(&theCdt);
	}
	assert(false);
	return 0;
}

static inline void
onGetFilledFields(HWND wnd, int fieldsClass) {
	dlg_setResult(wnd, getFilledFields(wnd, fieldsClass));
}

static inline void
updateAlbumFieldsButton(HWND wnd, BYTE hidden) {
	wchar_t text[] = {
		hidden ? L'a' : L'A',
		L'E',
		L'\0'
	};
	SetWindowText(GetDlgItem(wnd, kIdAlbumFields), text);
};

static inline void
updateTrackFieldsButton(HWND wnd, BYTE hidden) {
	wchar_t text[] = {
		hidden ? L't' : L'T',
		L'E',
		L'\0'
	};
	SetWindowText(GetDlgItem(wnd, kIdTrackFields), text);
}

static inline void
updateFieldsButtons(HWND wnd, int fieldsClass, BYTE fields) {
	BYTE filled = getFilledFields(wnd, fieldsClass);
	BYTE hidden = bit_getHidden8(fields, filled);
	switch (fieldsClass) {
	case fields_kAlbum:
		updateAlbumFieldsButton(wnd, hidden);
		fields = msg_sendGetFields(wnd, fields_kTrack);
		filled = getFilledFields(wnd, fields_kTrack);
		hidden = bit_getHidden8(fields, filled);
		updateTrackFieldsButton(wnd, hidden);
		break;
	case fields_kTrack:
		updateTrackFieldsButton(wnd, hidden);
		fields = msg_sendGetFields(wnd, fields_kAlbum);
		filled = getFilledFields(wnd, fields_kAlbum);
		hidden = bit_getHidden8(fields, filled);
		updateAlbumFieldsButton(wnd, hidden);
		break;
	}
}

static inline void
onSetFields(HWND wnd, int fieldsClass, BYTE fields) {
	bool result = msg_sendSetFields(GetParent(wnd), fieldsClass, fields);
	if (result) updateFieldsButtons(wnd, fieldsClass, fields);
	dlg_setResult(wnd, result);
}

static inline void
onGetFields(HWND wnd, int fieldsClass) {
	dlg_setResult(wnd, msg_sendGetFields(GetParent(wnd), fieldsClass));
}

static INT_PTR CALLBACK
dlgProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		HANDLE_MSG(wnd, WM_INITDIALOG, onInitDialog);
	case WM_COMMAND:
		onCommand(wnd, (int)LOWORD(wParam), (HWND)lParam, (UINT)HIWORD(wParam));
		return TRUE;
	case WM_PAINT:
		onPaint(wnd);
		return TRUE;
	case WM_CTLCOLOREDIT:
		return (INT_PTR)onCtlColorEdit(wnd, (HDC)wParam, (HWND)lParam);
	case MSG_SETTRACKRANGE:
		onSetTrackRange(wnd, LOBYTE(wParam), HIBYTE(wParam));
		return TRUE;
	case MSG_SETFIELDS:
		onSetFields(wnd, (int)wParam, (BYTE)lParam);
		return TRUE;
	case MSG_GETFIELDS:
		onGetFields(wnd, (int)wParam);
		return TRUE;
	case MSG_GETFILLEDFIELDS:
		onGetFilledFields(wnd, (int)wParam);
		return TRUE;
	case MSG_COMPLETENESSCHANGED:
		onCompletenessChanged(wnd, (bool)wParam);
		return TRUE;
	case MSG_DATACHANGED:
		onDataChanged(wnd, (bool)wParam);
		return TRUE;
	case MSG_LAYOUTCHANGED:
		onLayoutChanged(wnd, (WORD)wParam);
		return TRUE;
	case WM_DPICHANGED_AFTERPARENT:
		onDpiChangedAfterParent(wnd);
		return TRUE;
	}
	return FALSE;
}

HWND
toolbar_create(HWND parent) {
	HWND wnd = CreateDialog((HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE), MAKEINTRESOURCE(kDlgToolbar), parent, dlgProc);
	if (!wnd) return NULL;
	SetWindowLongPtr(wnd, GWLP_ID, (LONG_PTR)kDlgToolbar);
	return wnd;
}
