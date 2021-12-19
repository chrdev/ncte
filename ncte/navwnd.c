// We can use a listbox for navigation, but let's have fun by customizing a raw window
//
#include "navwnd.h"

#include <windowsx.h>

#include <stdbool.h>
#include <stdlib.h> // calloc, free

#include "wnddata.h"
#include "dpi.h"
#include "wnd.h"
#include "font.h"
#include "msg.h"
#include "debug.h"
#include "resdefs.h"
#include "cdt.h"
#include "file.h"
#include "sys.h"
#include "color.h"
#include "clipboard.h"
#include "format.h"
#include "msgbox.h"


static const wchar_t kClsName[] = L"NAVWND";

enum {
	kItemHeightInChar = 3,
	kIdealWidthInChar = 15,

	kMarginTL = 7,
	kContentPaddingInChar = 1,
};

typedef struct WndData {
	HWND wnd;

	int itemCx;
	int itemCy;

	int selection;

	// If earlySelect is true, select item by mouse button down. When nav newly got focus because of a button down, we need this behavior.
	// If earlySelect is false, select item by mouse button up. This is the normal behavior.
	bool earlySelect;
}WndData;

static inline void
getItemRect(const WndData* wd, int index, RECT* rect) {
	assert(index >= 0 && index < cdt_kBlockCountMax);
	rect->left = 0;
	rect->right = wd->itemCx;
	rect->top = index * wd->itemCy;
	rect->bottom = rect->top + wd->itemCy;
}

static int
itemFromPt(const WndData* wd, int x, int y) {
	if (x < 0 || x >= wd->itemCx) return -1;
	int index = y / wd->itemCy;
	if (index < 0 || index >= cdt_kBlockCountMax) return -1;
	return index;
}

// pt is both input and output, in screen coordinates.
// If pt.x or pt.y is negative, fill pt with center point of selected item, return selected item index.
// if both pt.x and pt.y are positive, return item index from pt, pt is not changed.
static int
itemFromScreenPt(const WndData* wd, POINT* pt) {
	if (pt->x < 0 || pt->y < 0) {
		if (wd->selection < 0) return -1;
		RECT rc;
		getItemRect(wd, wd->selection, &rc);
		MapWindowPoints(wd->wnd, NULL, (POINT*)&rc, 2);
		pt->x = rc.left + (rc.right - rc.left) / 2;
		pt->y = rc.top + (rc.bottom -rc.top) / 2;
		return wd->selection;
	}
	else {
		POINT p = *pt;
		MapWindowPoints(NULL, wd->wnd, &p, 1);
		return itemFromPt(wd, p.x, p.y);
	}
}

static void
invalidateItem(const WndData* wd, int index) {
	RECT rc;
	getItemRect(wd, index, &rc);
	InvalidateRect(wd->wnd, &rc, FALSE);
}

static inline void
selectItem(WndData* wd, int index) {
	assert(index >= 0 && index < cdt_kBlockCountMax);
	assert(wd->selection != index);

	if (!cdt_reserveBlock(&theCdt, index)) return;

	HDC dc = GetDC(wd->wnd);
	if (wd->selection >= 0) {
		int oldSel = wd->selection;
		wd->selection = -1;
		invalidateItem(wd, oldSel);
	}
	wd->selection = index;
	invalidateItem(wd, index);
	ReleaseDC(wd->wnd, dc);

	msg_sendNav(GetParent(wd->wnd), index);
}

static inline BOOL
onCreate(HWND wnd, LPCREATESTRUCT cs) {
	STOR_WNDDATA;
	wd->wnd = wnd;

	return TRUE;
}

static inline void
onDestroy(HWND wnd) {
	USE_WNDDATA;
	free(wd);
}

static inline int
getItemCy(int cx) {
	return cx * 3 / 5;
}

static inline void
onWindowPosChanged(HWND wnd, const WINDOWPOS* wp) {
	// blank;
}

static inline void
onWindowPosChanging(HWND wnd, WINDOWPOS* wp) {
	if (!wnd_sizeChanged(wp)) return;
	USE_WNDDATA;
	wd->itemCx = wp->cx;
	wd->itemCy = getItemCy(wp->cx);
	wp->cy = wd->itemCy * cdt_kBlockCountMax;
}

//static inline void
//onMouseMove(HWND wnd, int x, int y, UINT keyFlags) {
//	USE_WNDDATA;
//	TRACKMOUSEEVENT tme = { 
//		.cbSize = sizeof(TRACKMOUSEEVENT),
//		.dwFlags = TME_LEAVE,
//		.hwndTrack = wnd,
//		.dwHoverTime = 0
//	};
//	TrackMouseEvent(&tme);
//
//	int index = itemFromPt(x, y, &wd->itemMetrics);
//	if (wd->hot == index) return;
//	changeHotItem(wd, index);
//}

//static inline void
//onMouseLeave(HWND wnd) {
//	USE_WNDDATA;
//	if (wd->hot < 0) return;
//	changeHotItem(wd, -1);
//}

enum Direction {
	kUp = -1,
	kDown = 1,
};

static inline void
shiftItem(WndData* wd, enum Direction direction) {
	int index = wd->selection + direction;
	if (index < 0 || index >= cdt_kBlockCountMax) return;
	assert(index != wd->selection);
	selectItem(wd, index);
}

static inline void
navByPt(WndData* wd, int x, int y) {
	int index = itemFromPt(wd, x, y);
	if (index < 0) return;
	if (wd->selection == index) return;
	selectItem(wd, index);
}

static inline void
onLButtonDown(HWND wnd, BOOL isDoubleClick, int x, int y, UINT keyFlags) {
	SetCapture(wnd);
	USE_WNDDATA;
	if (!wd->earlySelect) return;
	wd->earlySelect = false;
	navByPt(wd, x, y);
}

static inline void
onLButtonUp(HWND wnd, int x, int y, UINT keyFlags) {
	ReleaseCapture();
	USE_WNDDATA;
	navByPt(wd, x, y);
}

static inline void
drawFocus(HDC dc, const RECT* itemRect) {
	RECT rc = {
		.left = itemRect->right - theFonts.normal.aveCharWidth / 3 * 2,
		.right = itemRect->right,
		.top = itemRect->top,
		.bottom = itemRect->bottom,
	};
	SetBkColor(dc, GetSysColor(COLOR_HIGHLIGHT));
	ExtTextOut(dc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
}

static inline void
drawBk(HDC dc, bool selected, int completeness, RECT* rc) {
	if (selected) {
		SetBkColor(dc, GetSysColor(COLOR_BTNHIGHLIGHT));
		ExtTextOut(dc, 0, 0, ETO_OPAQUE, rc, NULL, 0, NULL);
		InflateRect(rc, -theDpi.edgeCx, -theDpi.edgeCy);
	}
	else {
		DrawEdge(dc, rc, EDGE_RAISED, BF_RECT | BF_MIDDLE | BF_ADJUST);
	}

	if (completeness != cdt_kBlockHasAnyTitle) return;
	const int markSize = theFonts.normal.height * 5 / 8;
	const POINT pt[] = {
		{ rc->left, rc->top },
		{ rc->left, rc->top + markSize },
		{ rc->left + markSize, rc->top },
	};
	SelectObject(dc, GetStockObject(NULL_PEN));
	HBRUSH br = CreateSolidBrush(kColorAttention);
	SelectObject(dc, br);
	Polygon(dc, pt, ARRAYSIZE(pt));
	SelectObject(dc, GetStockBrush(NULL_BRUSH));
	DeleteObject(br);
}

static inline void
setIndexFont(HDC dc, bool selected) {
	HFONT font = selected ? theFonts.bold.handle : theFonts.normal.handle;
	SelectObject(dc, font);
	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, GetSysColor(COLOR_BTNTEXT));
}

static inline void
drawIndex(HDC dc, int index, RECT* rc) {
	wchar_t indexChar = index + L'1';
	rc->left += theFonts.normal.aveCharWidth * 4 / 3;
	rc->top += theFonts.normal.height * 1 / 6;
	ExtTextOut(dc, rc->left, rc->top, ETO_CLIPPED, rc, &indexChar, 1, NULL);
}

static inline void
setLanguageFont(HDC dc, bool selected) {
	HFONT font = selected ? theFonts.smallBold.handle : theFonts.smallNormal.handle;
	SelectObject(dc, font);
}

static inline void
drawLanguage(HDC dc, int index, RECT* rc) {
	const wchar_t* text;
	int len;
	text = cdt_getEncodingText(theCdt.blocks[index].album->encoding, &len);
	rc->top += theFonts.normal.height;
	ExtTextOut(dc, rc->left, rc->top, ETO_CLIPPED, rc, text, len, NULL);
	rc->top += theFonts.normal.height;
	text = cdt_getLanguageText(theCdt.blocks[index].album->language, &len);
	ExtTextOut(dc, rc->left, rc->top, ETO_CLIPPED, rc, text, len, NULL);	
}


// itemRc can be NULL. If itemRc is NULL, this func calculates the item rectangle
static inline void
drawItem(const WndData* wd, HDC dc, int index, const RECT* itemRect) {
	//dbg_printf(L"draw nav item %d: %d[%d]\n", i + 1, rc.top, rc.bottom - rc.top);
	assert(index >= 0 && index < cdt_kBlockCountMax);

	int completeness = cdt_blockGetCompleteness(theCdt.blocks + index);
	bool selected = index == wd->selection;
	RECT rc = *itemRect;

	drawBk(dc, selected, completeness, &rc);
	setIndexFont(dc, selected);
	drawIndex(dc, index, &rc);
	
	if (completeness != cdt_kNone) {
		setLanguageFont(dc, selected);
		drawLanguage(dc, index, &rc);
	}

	if (selected && GetFocus() == wd->wnd) {
		//LRESULT uiStates = SendMessage(wd->wnd, WM_QUERYUISTATE, 0, 0);
		//if (uiStates & UISF_HIDEFOCUS) return;
		drawFocus(dc, itemRect);
	}
}

static inline void
draw(HWND wnd, HDC dc, HRGN rgn) {
	USE_WNDDATA;
	RECT itemRc = { 0, 0, wd->itemCx, wd->itemCy };
	for (int i = 0; i < cdt_kBlockCountMax; ++i) {
		if (RectInRegion(rgn, &itemRc)) {
			drawItem(wd, dc, i, &itemRc);
		}
		OffsetRect(&itemRc, 0, wd->itemCy);
	}
}

static inline void
onPaint(HWND wnd) {
	HRGN rgn = CreateRectRgn(0, 0, 0, 0);
	int rc = GetUpdateRgn(wnd, rgn, FALSE);
	if (rc == ERROR || rc == NULLREGION) {
		DeleteObject(rgn);
		return;
	}

	PAINTSTRUCT ps;
	HDC dc = BeginPaint(wnd, &ps);
	draw(wnd, dc, rgn);
	EndPaint(wnd, &ps);

	DeleteObject(rgn);
}

static inline void
onPrintClient(HWND wnd, HDC dc) {
	RECT rc;
	GetClientRect(wnd, &rc);
	HRGN rgn = CreateRectRgnIndirect(&rc);
	draw(wnd, dc, rgn);
	DeleteObject(rgn);
}

static inline void
onSetFocus(HWND wnd, HWND wndOther) {
	USE_WNDDATA;
	if (wd->earlySelect) return;
	if (UISF_HIDEFOCUS == SendMessage(wnd, WM_QUERYUISTATE, 0, 0)) return;
	invalidateItem(wd, wd->selection);
}

static inline void
onKillFocus(HWND wnd, HWND wndOther) {
	if (GetCapture() == wnd) ReleaseCapture();
	if (UISF_HIDEFOCUS == SendMessage(wnd, WM_QUERYUISTATE, 0, 0)) return;
	USE_WNDDATA;
	invalidateItem(wd, wd->selection);
}

static inline void
onKey(HWND wnd, UINT vk, BOOL isDown, int repeatCount, UINT flags) {
	USE_WNDDATA;
	switch (vk) {
	case VK_DOWN:
		shiftItem(wd, kDown);
		break;
	case VK_UP:
		shiftItem(wd, kUp);
		break;
	case VK_HOME:
		if (wd->selection != 0) selectItem(wd, 0);
		break;
	case VK_END:
		if (wd->selection != cdt_kBlockCountMax - 1) selectItem(wd, cdt_kBlockCountMax - 1);
		break;
	}
}

static inline UINT
onGetDlgCode(HWND wnd, LPMSG msg) {
	return DLGC_BUTTON | DLGC_DEFPUSHBUTTON | DLGC_WANTARROWS;
}

static inline void
onMouseActivate(HWND wnd) {
	if (GetFocus() == wnd) return;
	SetFocus(wnd);
	USE_WNDDATA;
	wd->earlySelect = true;
}

static inline void
onCompletenessChanged(HWND wnd, bool allBlocks) {
	USE_WNDDATA;
	if (allBlocks) {
		InvalidateRect(wnd, NULL, FALSE);
	}
	else {
		invalidateItem(wd, wd->selection);
	}
}

static inline int
onGetNav(HWND wnd) {
	USE_WNDDATA;
	return wd->selection;
}

static inline void
onUpdateUiState(HWND wnd, WORD action, WORD flags) {
	USE_WNDDATA;
	if (GetFocus() == wnd && flags & UISF_HIDEFOCUS) invalidateItem(wd, wd->selection);
}

static inline bool
onDataChanged(HWND wnd, bool showAllFields) {
	USE_WNDDATA;
	if (!theCdt.blocks[wd->selection].heap) wd->selection = cdt_getFirstValidBlockIndex(&theCdt);
	InvalidateRect(wnd, NULL, TRUE);
	return true;
}

static inline void
initPopupMenuItems(HWND wnd, HMENU h, int item) {
	if (item == 0) EnableMenuItem(h, kShiftUp, MF_BYCOMMAND | MF_GRAYED);
	else if (item == cdt_kBlockCountMax - 1) EnableMenuItem(h, kShiftDown, MF_BYCOMMAND | MF_GRAYED);

	if (cdt_blockIsEmpty(theCdt.blocks + item)) {
		EnableMenuItem(h, 2, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(h, kEditCopy, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(h, kEditDelete, MF_BYCOMMAND | MF_GRAYED);
	}

	bool canPaste = false;
	if (OpenClipboard(wnd)) {
		UINT formats[] = { CF_UNICODETEXT, CF_HDROP };
		canPaste = 0 < GetPriorityClipboardFormat(formats, ARRAYSIZE(formats));
		CloseClipboard();
	}
	if (!canPaste) EnableMenuItem(h, kEditPaste, MF_BYCOMMAND | MF_GRAYED);
}

static inline WORD
popupMenu(WndData* wd, int x, int y) {
	POINT pt = { x, y };
	int item = itemFromScreenPt(wd, &pt);
	if (item < 0) return 0;
	if (wd->selection != item) selectItem(wd, item);

	HMENU menu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(kMenuNavPopup));
	if (!menu) return 0;
	HMENU popup = GetSubMenu(menu, 0);
	initPopupMenuItems(wd->wnd, popup, item);

	UINT flags = TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | sys_getPopupMenuAlignment();
	//dbg_printf(L"Before trackpopup menu %u\n", GetLastError());
	WORD cmd = (WORD)TrackPopupMenu(popup, flags, pt.x, pt.y, 0, wd->wnd, NULL);
	//dbg_printf(L"after track popup menu %u\n", GetLastError());
	DestroyMenu(menu);
	return cmd;
}

static inline void
handlePaste(HWND wnd) {
	if (!clipboard_open(wnd)) return;

	wchar_t* text = clipboard_getText(NULL);
	if (text) {
		clipboard_close();
		msg_sendCommand(GetParent(wnd), command_kOpenText, (LPARAM)text);
		HeapFree(GetProcessHeap(), 0, text);
		return;
	}
	HDROP drop = clipboard_getDrop();
	if (drop) {
		msg_sendCommand(GetParent(wnd), command_kOpenDrop, (LPARAM)drop);
	}
	clipboard_close();

}

static inline void
handleDelete(const WndData* wd) {
	if (IDYES != msgbox_show(wd->wnd, kTextPromptDelete, 0, MB_YESNO | MB_ICONEXCLAMATION)) return;
	cdt_clearBlock(&theCdt, wd->selection);
	msg_sendDataChanged(GetParent(wd->wnd), false);
}

static inline void
handleCopy(HWND wnd) {
	USE_WNDDATA;
	clipboard_putIndex(wnd, wd->selection);
}

static inline void
onContextMenu(HWND wnd, HWND ctxWnd, UINT x, UINT y) {
	USE_WNDDATA;
	switch (popupMenu(wd, x ,y)) {
	case kEditCopy:
		handleCopy(wnd);
		break;
	case kEditPaste:
		handlePaste(wnd);
		break;
	case kShiftUp:
		selectItem(wd, cdt_blockShiftUp(&theCdt, wd->selection));
		break;
	case kShiftDown:
		selectItem(wd, cdt_blockShiftDown(&theCdt, wd->selection));
		break;
	case kEditDelete:
		handleDelete(wd);
		break;
	case kCopyAsCue:
		msg_sendCopy(GetParent(wnd), format_kCue, wd->selection);
		break;
	case kCopyAsEac:
		msg_sendCopy(GetParent(wnd), format_kEac, wd->selection);
		break;
	}
}

static inline void
onDpiChangedAfterParent(HWND wnd) {
	InvalidateRect(wnd, NULL, TRUE);
}

static LRESULT WINAPI
wndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		HANDLE_MSG(wnd, WM_CREATE, onCreate);
		HANDLE_MSG(wnd, WM_DESTROY, onDestroy);
		HANDLE_MSG(wnd, WM_WINDOWPOSCHANGED, onWindowPosChanged);
		//HANDLE_MSG(wnd, WM_MOUSEMOVE, onMouseMove);
		//MSG_HANDLE_WM_MOUSELEAVE(wnd, onMouseLeave);
		HANDLE_MSG(wnd, WM_LBUTTONDOWN, onLButtonDown);
		HANDLE_MSG(wnd, WM_LBUTTONUP, onLButtonUp);
		HANDLE_MSG(wnd, WM_CONTEXTMENU, onContextMenu);
		HANDLE_MSG(wnd, WM_SETFOCUS, onSetFocus);
		HANDLE_MSG(wnd, WM_KILLFOCUS, onKillFocus);
		HANDLE_MSG(wnd, WM_KEYDOWN, onKey);
		HANDLE_MSG(wnd, WM_GETDLGCODE, onGetDlgCode);
		MSG_HANDLE_WM_DPICHANGED_AFTERPARENT(wnd, onDpiChangedAfterParent);
		MSG_HANDLE_GETNAV(wnd, onGetNav);
		MSG_HANDLE_DATACHANGED(wnd, onDataChanged);
		MSG_HANDLE_WM_UPDATEUISTATE(wnd, onUpdateUiState);
		MSG_HANDLE_COMPLETENESSCHANGED(wnd, onCompletenessChanged);
		//HANDLE_MSG(wnd, WM_COMMAND, onCommand);
		//HANDLE_MSG(wnd, WM_DRAWITEM, onDrawItem);
		//HANDLE_MSG(wnd, WM_MEASUREITEM, onMeasureItem);
		//HANDLE_MSG(wnd, WM_CTLCOLORLISTBOX, onCtlColorListBox);
		//MSG_HANDLE_NAV(wnd, onNav);
	case WM_WINDOWPOSCHANGING:
		onWindowPosChanging(wnd, (WINDOWPOS*)lParam);
		break;
	case WM_MOUSEACTIVATE:
		onMouseActivate(wnd);
		break;
	case WM_PAINT:
		onPaint(wnd);
		return 0;
	case WM_PRINTCLIENT:
		onPrintClient(wnd, (HDC)wParam);
		return 0;
	}
	return DefWindowProc(wnd, msg, wParam, lParam);
}

static inline bool
registerClass(void) {
	WNDCLASS wc = {
		.style = CS_PARENTDC,
		.lpfnWndProc = wndProc,
		.hInstance = GetModuleHandle(NULL),
		.hCursor = LoadCursor(NULL, IDC_ARROW),
		.hbrBackground = (HBRUSH)(COLOR_BTNHIGHLIGHT + 1),
		.lpszMenuName = NULL,
		.lpszClassName = kClsName,
	};
	return RegisterClass(&wc);
}

HWND
navwnd_create(HWND parent, intptr_t id) {
	if (!registerClass()) return NULL;

	WndData* wd = calloc(1, sizeof(WndData));
	if (!wd) return NULL;

	return CreateWindowEx(
		WS_EX_NOPARENTNOTIFY,
		kClsName, NULL,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP,
		0, 0, 0, 0,
		parent, (HMENU)id, GetModuleHandle(NULL), wd);
}
