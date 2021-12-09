#pragma once

#include <Windows.h>
#include <Uxtheme.h>

#include <stdbool.h>
#include <malloc.h>


static inline bool
wnd_setLTWH(HWND wnd, int x, int y, int cx, int cy) {
	return SetWindowPos(wnd, NULL, x, y, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSENDCHANGING);
}

static inline bool
wnd_setRect(HWND wnd, const RECT* rect) {
	return wnd_setLTWH(wnd, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);
}

static inline bool
wnd_setSize(HWND wnd, int cx, int cy) {
	return SetWindowPos(wnd, NULL, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSENDCHANGING);
}

static inline bool
wnd_setCy(HWND wnd, int cy) {
	RECT rect;
	if (!GetWindowRect(wnd, &rect)) return false;
	return wnd_setSize(wnd, rect.right - rect.left, cy);
}

static inline bool
wnd_setCx(HWND wnd, int cx) {
	RECT rect;
	if (!GetWindowRect(wnd, &rect)) return false;
	return wnd_setSize(wnd, cx, rect.bottom - rect.top);
}

static inline bool
wnd_setX(HWND wnd, int x) {
	RECT rc = { 0 };
	if (!GetWindowRect(wnd, &rc)) return false;
	MapWindowPoints(NULL, GetAncestor(wnd, GA_PARENT), (POINT*)&rc, 1);
	return SetWindowPos(wnd, NULL, x, rc.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSENDCHANGING | SWP_NOSIZE);
}

static inline bool
wnd_setY(HWND wnd, int y) {
	RECT rc = { 0 };
	if (!GetWindowRect(wnd, &rc)) return false;
	MapWindowPoints(NULL, GetAncestor(wnd, GA_PARENT), (POINT*)&rc, 1);
	return SetWindowPos(wnd, NULL, rc.left, y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSENDCHANGING | SWP_NOSIZE);
}


static inline bool
wnd_move(HWND wnd, int x, int y) {
	return SetWindowPos(wnd, NULL, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSENDCHANGING | SWP_NOSIZE);
}

static inline bool
wnd_offsetX(HWND wnd, int delta) {
	RECT rc;
	if (!GetWindowRect(wnd, &rc)) return false;
	rc.left += delta;
	MapWindowPoints(NULL, GetParent(wnd), (POINT*)&rc, 1);
	return SetWindowPos(wnd, NULL, rc.left, rc.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOSENDCHANGING | SWP_NOREDRAW);
}

static inline SIZE
wnd_getClientSize(HWND wnd) {
	RECT rect = { 0 };
	GetClientRect(wnd, &rect);
	SIZE result = { rect.right - rect.left, rect.bottom - rect.top };
	return result;
}

static inline SIZE
wnd_getSize(HWND wnd) {
	RECT rc = { 0 };
	if (!GetWindowRect(wnd, &rc)) return *(SIZE*)&rc;
	OffsetRect(&rc, -rc.left, -rc.top);
	return *((SIZE*)&rc + 1);
}

static inline int
wnd_getClientCx(HWND wnd) {
	RECT rect = { 0 };
	GetClientRect(wnd, &rect);
	return rect.right - rect.left;
}

static inline int
wnd_getClientCy(HWND wnd) {
	RECT rect = { 0 };
	GetClientRect(wnd, &rect);
	return rect.bottom - rect.top;
}

static inline int
wnd_getCx(HWND wnd) {
	RECT rc = { 0 };
	GetWindowRect(wnd, &rc);
	return rc.right - rc.left;
}

static inline int
wnd_getCy(HWND wnd) {
	RECT rc = { 0 };
	GetWindowRect(wnd, &rc);
	return rc.bottom - rc.top;
}

static inline void
wnd_setRedraw(HWND wnd, bool redraw) {
	SendMessage(wnd, WM_SETREDRAW, redraw, 0);
}

//static inline int
//wnd_getX(HWND wnd) {
//	RECT rc;
//	GetWindowRect(wnd, &rc);
//	MapWindowPoints(NULL, GetParent(wnd), (POINT*)&rc, 1);
//	return rc.left;
//}
//
//static inline int
//wnd_getY(HWND wnd) {
//	RECT rc;
//	GetWindowRect(wnd, &rc);
//	MapWindowPoints(NULL, GetParent(wnd), (POINT*)&rc, 1);
//	return rc.top;
//}

enum setFont_Redraw{
	setFont_kNoRedraw,
	setFont_kRedraw
};

static inline void
wnd_setFont(HWND wnd, HFONT font, enum setFont_Redraw redraw) {
	SendMessage(wnd, WM_SETFONT, (WPARAM)font, (LPARAM)redraw);
}

static inline HFONT
wnd_getFont(HWND wnd) {
	return (HFONT)SendMessage(wnd, WM_GETFONT, 0, 0);
}

static inline int
wnd_getFontHeight(HWND wnd) {
	HDC dc = GetDC(wnd);
	if (!dc) return 0;
	TEXTMETRIC tm;
	GetTextMetrics(dc, &tm);
	ReleaseDC(wnd, dc);
	return tm.tmHeight;
}

static inline SIZE
wnd_getTextSize(HWND wnd) {
	SIZE result = { 0 };
	int cch = GetWindowTextLength(wnd);
	if (!cch) return result;

	__try {
		wchar_t* text = _alloca(sizeof(wchar_t) * ((size_t)cch + 1));
		HDC dc = GetDC(wnd);
		GetTextExtentPoint32(dc, text, cch, &result);
		ReleaseDC(wnd, dc);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		_resetstkoflw();
	}

	return result;
}

static inline bool
wnd_isActiveCapture(HWND wnd) {
	HWND wndActive = GetActiveWindow();
	return (wndActive == wnd || IsChild(wndActive, wnd)) && GetCapture() == wnd;
}

static inline void
wnd_setNoParentNotify(HWND wnd) {
	SetWindowLong(wnd, GWL_EXSTYLE, GetWindowLong(wnd, GWL_EXSTYLE) | WS_EX_NOPARENTNOTIFY);
}

static inline int
wnd_getScrollPosV(HWND wnd) {
	SCROLLINFO si = { sizeof(si), SIF_POS };
	GetScrollInfo(wnd, SB_VERT, &si);
	return si.nPos;
}

static inline HTHEME
wnd_openThemeData(HWND wnd, const wchar_t* classList) {
	if (!IsThemeActive()) return NULL;
	return OpenThemeData(wnd, classList);
}

static inline void
wnd_focusNextCtl(HWND wnd, bool prev) {
	PostMessage(wnd, WM_NEXTDLGCTL, (WPARAM)prev, FALSE);
}

static inline void
wnd_focusThisCtl(HWND wnd, HWND ctl) {
	PostMessage(wnd, WM_NEXTDLGCTL, (WPARAM)ctl, TRUE);
}

// Caution: return pointer to static buffer
// len can be NULL
const wchar_t*
wnd_getText(HWND wnd, int* len);

static inline bool
wnd_sizeChanged(const WINDOWPOS* wp) {
	if (wp->flags & SWP_NOSIZE) return false;
	if (wp->cx <= 0 || wp->cy <= 0) return false;
	return true;
}

static inline LONG_PTR
dlg_setResult(HWND wnd, LONG_PTR result) {
	return SetWindowLongPtr(wnd, DWLP_MSGRESULT, result);
}

static inline void
dlg_setItemFont(HWND wnd, WORD id, HFONT font, enum setFont_Redraw redraw) {
	wnd_setFont(GetDlgItem(wnd, id), font, redraw);
}
