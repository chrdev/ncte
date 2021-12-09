#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include <assert.h>

#include "dpi.h"


static inline bool
commctrl_buttonChecked(HWND wnd) {
	return BST_CHECKED == SendMessage(wnd, BM_GETCHECK, 0, 0);
}

// Returns new check state
static inline bool
commctrl_buttonToggleCheck(HWND wnd) {
	int state = 1 - (int)SendMessage(wnd, BM_GETCHECK, 0, 0);
	SendMessage(wnd, BM_SETCHECK, state, 0);
	return (bool)state;
}

static inline HWND
commctrl_createPushButton(HWND parent, intptr_t id, const wchar_t* text) {
	DWORD style = BS_PUSHBUTTON | WS_GROUP | WS_TABSTOP | WS_VISIBLE | WS_CHILD;
	return CreateWindowEx(
		WS_EX_NOPARENTNOTIFY,
		WC_BUTTON, text, style,
		0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
		parent, (HMENU)id, GetModuleHandle(NULL), NULL);
}

static inline HWND
commctrl_createGroup(HWND parent, intptr_t id, const wchar_t* text) {
	DWORD style = BS_GROUPBOX | WS_VISIBLE | WS_CHILD;
	return CreateWindowEx(
		WS_EX_NOPARENTNOTIFY,
		WC_BUTTON, text, style,
		0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
		parent, (HMENU)id, GetModuleHandle(NULL), NULL);
}

enum StaticStyle {
	staticStyle_kNone,
	staticStyle_kEtchedVert = SS_ETCHEDVERT,
	staticStyle_kNoPrefix = SS_NOPREFIX,

};
static inline HWND
commctrl_createStatic(HWND parent, intptr_t id, enum StaticStyle extraStyle, const wchar_t* text) {
	DWORD style = extraStyle | WS_VISIBLE | WS_CHILD;
	return CreateWindowEx(
		0,
		WC_STATIC, text, style,
		0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
		parent, (HMENU)id, GetModuleHandle(NULL), NULL);
}

static inline int
commctrl_staticGetAlignedYOffset(HWND wnd, HWND wndToAlign) {
	RECT rc, rcToAlign;
	DWORD style = (DWORD)GetWindowLong(wnd, GWL_STYLE);
	DWORD exStyle = (DWORD)GetWindowLong(wnd, GWL_EXSTYLE);
	GetWindowRect(wnd, &rc);
	OffsetRect(&rc, -rc.left, -rc.top);
	AdjustWindowRectExForDpi(&rc, style, FALSE, exStyle, theDpi.dpi);
	
	style = (DWORD)GetWindowLong(wndToAlign, GWL_STYLE);
	exStyle = (DWORD)GetWindowLong(wndToAlign, GWL_EXSTYLE);
	GetWindowRect(wndToAlign, &rcToAlign);
	OffsetRect(&rcToAlign, -rcToAlign.left, -rcToAlign.top);
	AdjustWindowRectExForDpi(&rcToAlign, style, FALSE, exStyle, theDpi.dpi);
	
	return rc.top - rcToAlign.top;
}

enum EditStyle {
	editStyle_kNone,
	editStyle_kNumber = ES_NUMBER,
};

static inline HWND
commctrl_createEdit(HWND parent, intptr_t id, enum EditStyle extraStyle, const wchar_t* text) {
	DWORD style = extraStyle | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_GROUP | WS_TABSTOP;

	return CreateWindowEx(
		0,
		WC_EDIT, text, style,
		0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
		parent, (HMENU)id, GetModuleHandle(NULL), NULL);
}

static inline void
commctrl_editSetLimitText(HWND wnd, int cch) {
	SendMessage(wnd, EM_SETLIMITTEXT, (WPARAM)cch, 0);
}

static inline bool
commctrl_editIsDirty(HWND wnd) {
	return (bool)SendMessage(wnd, EM_GETMODIFY, 0, 0);
}

static inline void
commctrl_editSetDirty(HWND wnd, bool dirty) {
	SendMessage(wnd, EM_SETMODIFY, (WPARAM)dirty, 0);
}

static inline void
commctrl_dlgEditSetDirty(HWND dlg, int id, bool dirty) {
	SendDlgItemMessage(dlg, id, EM_SETMODIFY, (WPARAM)dirty, 0);
}

static inline void
commctrl_editAdjustRect(RECT* rc) {
	//rc->left -= theDpi.edgeCx;
	//rc->right += theDpi.edgeCy;
	rc->top -= theDpi.edgeCx;
	rc->bottom += theDpi.edgeCy;
}

static inline HWND
commctrl_createTooltip(HWND parent, int partnerId, WORD strId) {
	HWND wnd = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		parent, NULL, GetModuleHandle(NULL),
		NULL);
	if (!wnd) return NULL;

	TOOLINFO ti = {
		.cbSize = sizeof(ti),
		.uFlags = TTF_IDISHWND | TTF_SUBCLASS,
		.hwnd = parent,
		.uId = (UINT_PTR)GetDlgItem(parent, partnerId),
		.lpszText = MAKEINTRESOURCE(strId),
	};
	SendMessage(wnd, TTM_ADDTOOL, 0, (LPARAM)&ti);

	SetWindowPos(wnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
	return wnd;
}

static inline HWND
commctrl_createCombo(HWND parent, intptr_t id) {
	DWORD style = CBS_DROPDOWNLIST | WS_VSCROLL | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_GROUP | WS_TABSTOP;

	return CreateWindowEx(
		0,
		WC_COMBOBOX, NULL, style,
		0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
		parent, (HMENU)id, GetModuleHandle(NULL), NULL);
}

static inline bool
commctrl_comboAddString(HWND wnd, const wchar_t* text) {
	int rc = (int)SendMessage(wnd, CB_ADDSTRING, 0, (LPARAM)text);
	return rc >= 0;
}

static inline int
commctrl_comboGetItemHeight(HWND wnd, int item) {
	return (int)SendMessage(wnd, CB_GETITEMHEIGHT, item, 0);
}

static inline bool
commctrl_comboSelectItem(HWND wnd, int index) {
	return CB_ERR != SendMessage(wnd, CB_SETCURSEL, (WPARAM)index, 0);
}

static inline void
commctrl_comboAdjustRect(HWND wnd, RECT* rc) {
	int cy = commctrl_comboGetItemHeight(wnd, -1);
	int offset = (cy - rc->bottom + rc->top) / 2;
	rc->top -= offset;
	rc->bottom = rc->top + cy;
}

static inline bool
commctrl_comboSetDroppedWidth(HWND wnd, int cx) {
	return CB_ERR != SendMessage(wnd, CB_SETDROPPEDWIDTH, (WPARAM)cx, 0);
}

static inline bool
commctrl_comboInitStorage(HWND wnd, size_t count, size_t cb) {
	return CB_ERRSPACE != SendMessage(wnd, CB_INITSTORAGE, (WPARAM)count, (LPARAM)cb);
}

static inline int
commctrl_comboGetCurSel(HWND wnd) {
	return (int)SendMessage(wnd, CB_GETCURSEL, 0, 0);
}

static inline int
commctrl_dlgComboGetCurSel(HWND dlg, int id) {
	return (int)SendDlgItemMessage(dlg, id, CB_GETCURSEL, 0, 0);
}

static inline bool
commctrl_dlgComboSetCurSel(HWND dlg, int id, int sel) {
	return CB_ERR != SendDlgItemMessage(dlg, id, CB_SETCURSEL, (WPARAM)sel, 0);
}

static inline void
commctrl_updownSetRange(HWND wnd, int low, int high) {
	SendMessage(wnd, UDM_SETRANGE32, (WPARAM)low, (LPARAM)high);
}

// Returns the previous position
static inline int
commctrl_updownSetPos(HWND wnd, int pos) {
	return (int)SendMessage(wnd, UDM_SETPOS32, 0, (LPARAM)pos);
}
