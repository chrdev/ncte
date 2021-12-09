#include "mselector.h"

#include <windowsx.h>

#include <assert.h>
#include <stdbool.h>

#include "wnddata.h"
#include "wnd.h"
#include "dpi.h"
#include "font.h"
#include "msg.h"
#include "bit.h"
#include "debug.h"
#include "theme.h"
#include "sys.h"
#include "kbd.h"
#include "cdt.h"
#include "color.h"


static const wchar_t kClsName[] = L"MSELECTOR";


typedef struct WndData {
	HWND wnd;
	HWND parent;
	
	HTHEME themeButton;
	HTHEME themeMenu;

	int itemCount;
	int hot;
	int itemCx;
	int itemCy;

	int msgId;
	const ResStr* items;
	BYTE fields;
	BYTE hiddenFields;
	cdt_getFieldsFunc getFields;
	cdt_getHiddenFieldsFunc getHiddenFields;
	int notifyHideFieldsExtra;

	bool wasMouseOnMe;
	bool done;
}WndData;

static inline void
openThemeData(WndData* wd) {
	wd->themeButton = wnd_openThemeData(wd->wnd, L"BUTTON");
	wd->themeMenu = wnd_openThemeData(wd->wnd, L"MENU");
}

static inline void
closeThemeData(WndData* wd) {
	if (wd->themeButton) CloseThemeData(wd->themeButton), wd->themeButton = NULL;
	if (wd->themeMenu) CloseThemeData(wd->themeMenu), wd->themeMenu = NULL;
}

static inline BOOL
onCreate(HWND wnd, LPCREATESTRUCT cs) {
	STOR_WNDDATA;
	wd->wnd = wnd;
	wd->hot = -1;

	openThemeData(wd);

	return TRUE;
}

static inline void
onDestroy(HWND wnd) {
	USE_WNDDATA;
	closeThemeData(wd);
	free(wd);
}

static inline void
onWindowPosChanged(HWND wnd, const LPWINDOWPOS pos) {
	// blank
}

//static int
//itemFromPt(const WndData* wd, int x, int y) {
//	int result = -1;
//	POINT pt = { x, y };
//	RECT rect = {
//		.right = im->cx,
//	};
//	for (int i = 0; i < kItemCount; ++i) {
//		rect.top = i * im->cy;
//		rect.bottom = rect.top + im->cy;
//
//		if (PtInRect(&rect, pt)) {
//			result = i;
//			break;
//		}
//	}
//	return result;
//}

//static inline void
//onLButtonDown(HWND wnd, BOOL isDoubleClick, int x, int y, UINT keyFlags) {
//	SetCapture(wnd);
//}
//
//static inline void
//onLButtonUp(HWND wnd, int x, int y, UINT keyFlags) {
//	USE_WNDDATA;
//	ReleaseCapture();
//	//int index = itemFromPt(x, y, &wd->itemMetrics);
//	//if (index < 0) return;
//	//if (wd->selected == index) return;
//	//selectItem(wd, index);
//	//notifyNav(wd);
//}

//static inline int
//onMouseActivate(HWND wnd, HWND wndTopLevel, UINT hittestCode, UINT msg) {
//	SetFocus(wnd);
//	return MA_ACTIVATE;
//}

//static inline void
//onKey(HWND wnd, UINT vk, BOOL isDown, int repeatCount, UINT flags) {
//	USE_WNDDATA;
//	switch (vk) {
//	case VK_DOWN:
//		shiftItem(wd, kDown);
//		break;
//	case VK_UP:
//		shiftItem(wd, kUp);
//		break;
//	case VK_HOME:
//		if (wd->selected != 0) selectItem(wd, 0);
//		break;
//	case VK_END:
//		if (wd->selected != kItemCount - 1) selectItem(wd, kItemCount - 1);
//		break;
//	}
//}
//
//static inline UINT
//onGetDlgCode(HWND wnd, LPMSG msg) {
//	if (!msg) return DLGC_WANTMESSAGE;
//	if (msg->message >= WM_KEYFIRST && msg->message <= WM_KEYLAST && msg->wParam == VK_TAB) {
//		return (UINT)DefWindowProc(wnd, msg->message, msg->wParam, msg->lParam);
//	}
//	return DLGC_WANTMESSAGE;
//}
//
//static inline void
//drawFocus(const WndData* wd, HDC dc) {
//	RECT rc;
//	GetClientRect(wd->wnd, &rc);
//	DrawFocusRect(dc, &rc);
//}
//
//static inline void
//onSetFocus(HWND wnd, HWND wndOther) {
//	USE_WNDDATA;
//	HDC dc = GetDC(wnd);
//	drawFocus(wd, dc);
//	ReleaseDC(wnd, dc);
//}
//
//static inline void
//onKillFocus(HWND wnd, HWND wndOther) {
//	USE_WNDDATA;
//	if (GetCapture() == wnd) ReleaseCapture();
//	//if (wd->wnd) DestroyWindow(wnd);
//	//HDC dc = GetDC(wnd);
//	//drawFocus(wd, dc);
//	//ReleaseDC(wnd, dc);
//}
//

static inline void
getItemRect(const WndData* wd, int index, RECT* rc) {
	rc->left = 0;
	rc->right = rc->left + wd->itemCx;
	rc->top = index * wd->itemCy;
	rc->bottom = rc->top + wd->itemCy;
}

static inline int
getItemFromPt(const WndData* wd, int x, int y) {
	for (int i = 0; i < wd->itemCount; ++i) {
		RECT rc;
		getItemRect(wd, i, &rc);
		if (PtInRect(&rc, (POINT){ x, y })) return i;
	}
	return -1;
}

static inline void
drawHiddenMark(HDC dc, const RECT* itemRc, const RECT* iconRc) {
	RECT rc = {
		itemRc->left + 1,
		iconRc->top + 1,
		iconRc->left - 1,
		iconRc->bottom - 1,
	};
	SetBkColor(dc, kColorInfo);
	ExtTextOut(dc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
}

static inline void
drawItem(const WndData* wd, HDC dc, int index) {
	RECT rc;
	getItemRect(wd, index, &rc);
	bool checked = wd->fields & (1 << index);
	bool hot = wd->hot == index;
	bool hidden = wd->hiddenFields & (1 << index);

	int iconCx = sys_getMenuCheckCx() + theDpi.edgeCx + theDpi.edgeCx;
	int iconCy = sys_getMenuCheckCy() + theDpi.edgeCy + theDpi.edgeCy;
	int iconPadding = sys_getPaddedBorder();
	RECT iconRc = {
		.left = rc.left + iconPadding,
		.right = rc.left + iconCx,
		.top = rc.top + (wd->itemCy - iconCy) / 2,
		.bottom = rc.top + iconCy,
	};

	if (hidden) drawHiddenMark(dc, &rc, &iconRc);

	theme_drawCheckbox(wd->themeButton, dc, &iconRc, checked, hot);

	rc.left = iconRc.right + iconPadding;
	theme_drawMenuItem(wd->themeMenu, dc, &rc, hot);
	SelectObject(dc, theFonts.menu.handle);
	rc.left += iconPadding;
	theme_drawText(wd->themeMenu, dc, &rc, wd->items[index].p, wd->items[index].c, checked, hot);
}

static inline void
paint(HWND wnd, HDC dc, RECT* updateRc) {
	USE_WNDDATA;

	for (int i = 0; i < wd->itemCount; ++i) {
		drawItem(wd, dc, i);
	}
}

static inline void
onPaint(HWND wnd, HDC dc) {
	if (dc) {
		RECT rc;
		GetClientRect(wnd, &rc);
		paint(wnd, dc, &rc);
		return;
	}
	if (!GetUpdateRect(wnd, NULL, FALSE)) return;
	PAINTSTRUCT ps;
	dc = BeginPaint(wnd, &ps);
	paint(wnd, dc, &ps.rcPaint);
	EndPaint(wnd, &ps);
}

static inline BYTE
getFields(const WndData* wd, int item) {
	if (kbd_isCtrlDown()) {
		return 1 << item;
	}
	else if (kbd_isShiftDown()) {
		return wd->getFields(&theCdt);
	}
	else {
		return bit_flip8(wd->fields, item);
	}
}

static inline void
onLRButtonUp(HWND wnd, int x, int y, UINT flags) {
	USE_WNDDATA;
	int item = getItemFromPt(wd, x, y);
	if (item < 0) {
		wd->done = true;
		return;
	}

	BYTE fields = getFields(wd, item);
	if (!msg_sendSetFields(GetParent(wnd), wd->msgId, fields)) return;

	wd->fields = fields;
	wd->hiddenFields = wd->getHiddenFields(&theCdt, fields);

	//RECT rc;
	//getItemRect(wd, item, &rc);
	//InvalidateRect(wnd, &rc, FALSE);
	InvalidateRect(wnd, NULL, TRUE);

	msg_sendHideFields(wd->parent, wd->notifyHideFieldsExtra, wd->hiddenFields);
}

static inline void
onMouseMove(HWND wnd, int x, int y, UINT flags) {
	USE_WNDDATA;
	int index = getItemFromPt(wd, x, y);
	if (index < 0) return;
	if (index == wd->hot) return;

	RECT rc;
	getItemRect(wd, wd->hot, &rc);
	InvalidateRect(wnd, &rc, TRUE);
	getItemRect(wd, index, &rc);
	InvalidateRect(wnd, &rc, TRUE);
	
	wd->hot = index;
}

static inline void
onThemeChanged(HWND wnd) {
	USE_WNDDATA;
	closeThemeData(wd);
	openThemeData(wd);
}

static LRESULT WINAPI
wndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		HANDLE_MSG(wnd, WM_CREATE, onCreate);
		HANDLE_MSG(wnd, WM_DESTROY, onDestroy);
		HANDLE_MSG(wnd, WM_WINDOWPOSCHANGED, onWindowPosChanged);
		HANDLE_MSG(wnd, WM_LBUTTONUP, onLRButtonUp);
		HANDLE_MSG(wnd, WM_RBUTTONUP, onLRButtonUp);
		HANDLE_MSG(wnd, WM_MOUSEMOVE, onMouseMove);
		MSG_HANDLE_WM_THEMECHANGED(wnd, onThemeChanged);
	case WM_PAINT:
	case WM_PRINTCLIENT:
		onPaint(wnd, (HDC)wParam);
		return 0;
	}
	return DefWindowProc(wnd, msg, wParam, lParam);
}

static inline bool
registerClass(void) {
	WNDCLASS wc = {
		.style = CS_PARENTDC | CS_DROPSHADOW,
		.lpfnWndProc = wndProc,
		.hInstance = GetModuleHandle(NULL),
		.hCursor = LoadCursor(NULL, IDC_ARROW),
		.hbrBackground = (HBRUSH)(COLOR_MENU + 1),
		.lpszMenuName = NULL,
		.lpszClassName = kClsName,
	};
	return RegisterClass(&wc) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}


static inline void
handleMsg(WndData* wd) {
	RECT rc;
	GetWindowRect(wd->wnd, &rc);

	MSG msg = { 0 };
	for(;;)
	{
		for (; !wd->done && !PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE); wd->done = !WaitMessage());
		switch (msg.message) {
		//case WM_MOUSEMOVE:
		//	if (wd->wasMouseOnMe) {
		//		if (!PtInRect(&rc, msg.pt)) return;
		//	}
		//	else {
		//		if (PtInRect(&rc, msg.pt)) wd->wasMouseOnMe = true;
		//	}
		//	break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
			if (!PtInRect(&rc, msg.pt))	return;
			break;
		case WM_KEYDOWN:
			if (msg.wParam == VK_TAB) return;
			if (msg.wParam == VK_ESCAPE) return;
			if (msg.wParam == VK_RETURN) return;
			break;
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		case WM_SYSCOMMAND:
			return;
		case WM_QUIT:
			return;
		}

		if (!GetMessage(&msg, NULL, 0, 0)) break;
		if (wd->done) break;
		if (!wnd_isActiveCapture(wd->parent)) break;

		if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST) {
			POINT pt = msg.pt;
			MapWindowPoints(NULL, wd->wnd, &pt, 1);
			msg.lParam = MAKELPARAM(pt.x, pt.y);
			msg.hwnd = wd->wnd;
		}
		else if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) {
			msg.hwnd = wd->wnd;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (wd->done) break;
		if (!wnd_isActiveCapture(wd->parent)) break;
	}
}

void
mselector_popup(HWND parent, const MSelectorParams* params) {
	assert(params->itemCount <= 8);

	if (!registerClass()) return;

	WndData* wd = calloc(1, sizeof(WndData));
	if (!wd) return;
	wd->parent = parent;
	wd->itemCount = params->itemCount;
	wd->items = params->items;
	wd->fields = params->fields;
	wd->getFields = params->getFieldsFunc;
	wd->getHiddenFields = params->getHiddenFieldsFunc;
	wd->notifyHideFieldsExtra = params->notifyHideFieldsExtra;
	wd->hiddenFields = wd->getHiddenFields(&theCdt, wd->fields);
	wd->itemCx = params->cx;
	wd->itemCy = sys_getMenuItemCy();
	wd->msgId = params->msgId;

	DWORD style = WS_POPUP | WS_BORDER;
	DWORD exStyle = WS_EX_PALETTEWINDOW | WS_EX_NOACTIVATE | WS_EX_NOPARENTNOTIFY;
	RECT rc = {
		.left = params->x,
		.top = params->y,
		.right = params->x + params->cx,
		.bottom = params->y + wd->itemCy * wd->itemCount };
	AdjustWindowRectExForDpi(&rc, style, FALSE, exStyle, theDpi.dpi);

	SetCapture(parent);

	HWND wnd = CreateWindowEx(
		exStyle,
		kClsName, NULL,
		style,
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		parent, NULL, GetModuleHandle(NULL), wd);
	if (!wnd) return;
	ShowWindow(wnd, SW_SHOWNA);

	handleMsg(wd);
	ReleaseCapture();
	DestroyWindow(wnd);
}
