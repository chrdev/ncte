#include "body.h"

#include <windowsx.h>

#include <stdbool.h>
#include <assert.h>

#include "msg.h"
#include "wnd.h"
#include "debug.h"
#include "albumPanel.h"
#include "trackPAnel.h"
#include "resdefs.h"
#include "wnddata.h"
#include "str.h"
#include "cdt.h"
#include "thread.h"
#include "sys.h"
#include "bit.h"


static const wchar_t kClsName[] = L"BODYWND";

typedef struct WndData {
	HWND wnd;

	int block;
	int hot; // track in edit
	
	int completeness;
	bool isTocVisible;
	
	int albumCy;
	int trackFirst;
	int trackLast;
	int trackCx;
	int trackCy;

	int wheelDelta;
	POINT lastMousePos;
}WndData;


//static inline void
//saveTrackRange(HWND wnd, int first, int last) {
//	ULONG_PTR v = (ULONG_PTR)MAKEWORD((BYTE)first, (BYTE)last);
//	SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR)(v << (ULONG_PTR)16));
//}
//
//static inline void
//retrieveTrackRange(HWND wnd, int* first, int* last) {
//	assert(first);
//	assert(last);
//	ULONG_PTR v = GetWindowLongPtr(wnd, GWLP_USERDATA);
//	v >>= 16;
//	*first = LOBYTE((WORD)v);
//	*last = HIBYTE((WORD)v);
//}
//
//static inline int
//retrieveTrackCount(HWND wnd) {
//	int first, last;
//	retrieveTrackRange(wnd, &first, &last);
//	assert(first > 0 && first <= last && last < 100);
//	return last - first + 1;
//}

static inline int
trackFromPt(const WndData* wd, int x, int y) {
	int result = -1;
	if (x >= wd->trackCx) return result;

	int pos = wnd_getScrollPosV(wd->wnd);
	y += pos;
	if (y < wd->albumCy) return result;
	result = (y - wd->albumCy) / wd->trackCy;
	if (result > wd->trackLast - wd->trackFirst) result = -1;
	return result;
}

static inline int
getTrackY(const WndData* wd, int index) {
	assert(index >= 0 && index <= wd->trackLast - wd->trackFirst);

	int y = wd->albumCy + index * wd->trackCy;
	y -= wnd_getScrollPosV(wd->wnd);
	return y;
}

static inline void
getTrackRect(const WndData* wd, int index, RECT* rc) {
	int y = getTrackY(wd, index);
	SetRect(rc, 0, y, wd->trackCx, y + wd->trackCy);
}

static inline bool
isTrackFullyVisible(const WndData* wd, int index) {
	RECT rc;
	GetClientRect(wd->wnd, &rc);
	int y = getTrackY(wd, index);
	return y >= rc.top && y + wd->trackCy <= rc.bottom;
}

// If album panel is invisible, returns false
// If album panel is fully visible, returns false
// Only, if album panel is partially visible, returns true
static inline bool
isAlbumPartiallyVisible(const WndData* wd) {
	int y = -wnd_getScrollPosV(wd->wnd);
	return y + wd->albumCy > 0;
}

static inline bool
createChildren(WndData* wd) {
	HWND wnd = albumPanel_create(wd->wnd);
	if (!wnd) return false;
	SendMessage(wd->wnd, MSG_SETFIELDS, fields_kAlbum, 0xFF);
	ShowWindow(wnd, SW_SHOWNA);

	wnd = trackPanel_create(wd->wnd, kDlgTrack);
	if (!wnd) return false;
	wd->trackCx = wnd_getCx(wnd);

	wnd = trackPanel_create(wd->wnd, kDlgMemTrack);
	if (!wnd) return false;
	SendMessage(wd->wnd, MSG_SETFIELDS, fields_kTrack, 0xFF);

	return true;
}

static inline BOOL
onCreate(HWND wnd, LPCREATESTRUCT cs) {
	STOR_WNDDATA;
	wd->wnd = wnd;
	wd->hot = -1;
	wd->trackFirst = 1;
	wd->trackLast = 1;
	wd->isTocVisible = true;

	if (!createChildren(wd)) return FALSE;
	msg_postLayoutChanged(GetParent(wnd), kDlgAlbum);

	return TRUE;
}

static inline void
onDestroy(HWND wnd) {
	USE_WNDDATA;
	free(wd);
}

//static inline void
//getVisibleTrackRange(const WndData* wd, int* begin, int* end) {
//	assert(wd->trackCy);
//	int pos = wnd_getScrollPosV(wd->wnd);
//	*begin = (pos - wd->albumCy) / wd->trackCy;
//	int viewCy = wnd_getClientCy(wd->wnd);
//	*end = *begin + (viewCy + 1) / wd->trackCy;
//	//int cy = wnd_getClientCy(wd->wnd) + wd->itemCy - 1;
//	//int count = cy / wd->itemCy;
//	//*begin = wnd_getVScrollPos(wd->wnd);
//	//if (count > wd->count - *begin) count = wd->count - *begin;
//	//*end = *begin + count;
//}

static inline int
getFullCy(const WndData* wd) {
	return wd->albumCy + wd->trackCy * (wd->trackLast - wd->trackFirst + 1);
}

static inline void
drawTrack(const WndData* wd, HDC dc, int index) {
	HWND child = GetDlgItem(wd->wnd, kDlgMemTrack);
	trackPanel_fill(child, wd->block, index);
	SendMessage(child, WM_PRINT, (WPARAM)dc, PRF_ERASEBKGND | PRF_CLIENT | PRF_NONCLIENT | PRF_CHILDREN);
}

static inline void
getTrackRectV(const WndData* wd, int index, RECT* rc) {
	rc->top = wd->albumCy + index * wd->trackCy;
	rc->bottom = rc->top + wd->trackCy;
}

static inline int
getVisibleTrackRange(const WndData* wd, const RECT* rect, int* begin, int* end) {
	int trackCount = wd->trackLast - wd->trackFirst + 1;
	assert(trackCount);
	RECT allTracksRect = { 0, wd->albumCy, wd->trackCx, wd->albumCy + wd->trackCy * trackCount };
	int pos = wnd_getScrollPosV(wd->wnd);
	RECT updateRc = *rect;
	OffsetRect(&updateRc, 0, pos);
	//dbg_printf(L"pos %d, trackCy %d, allTrackRect %d[%d], updateRc %d[%d], ", pos, wd->trackCy, allTracksRect.top, allTracksRect.bottom - allTracksRect.top, updateRc.top, updateRc.bottom - updateRc.top);
	RECT rc;
	if (!IntersectRect(&rc, &allTracksRect, &updateRc)) {
		*begin = *end = 0;
		//dbg_printf(L"no intersect (0 update)\n");
		return 0;
	}
	assert(rc.top >= wd->albumCy);

	//dbg_printf(L"intersect rc %d-%d[%d]\n", rc.top, rc.bottom, rc.bottom - rc.top);
	*begin = (rc.top - wd->albumCy) / wd->trackCy;
	int y = wd->albumCy + wd->trackCy * *begin - pos;
	for (*end = *begin + 1; (*end * wd->trackCy + wd->albumCy < rc.bottom) && *end < *begin + trackCount; ++(*end));

	return y;
}

static inline void
paint(HWND wnd, HDC dc, RECT* updateRc) {
	USE_WNDDATA;
	assert(wd->trackCy);

	int begin, end;
	int y = getVisibleTrackRange(wd, updateRc, &begin, &end);

	SaveDC(dc);
	//dbg_printf(L"   ### paint circel start %d - %d, RECT(%d)[%d]\n", begin + 1, end, updateRc->top, updateRc->bottom - updateRc->top);
	for (int i = begin; i < end; ++i, y += wd->trackCy) {
		if (i == wd->hot) continue;
		SetViewportOrgEx(dc, 0, y, NULL);
		//dbg_printf(L"draw #%d @ %d-%d\n", i + 1, y, y + wd->trackCy);
		drawTrack(wd, dc, i);
	}
	//OutputDebugString(L"###### paint circle ends\n");
	RestoreDC(dc, -1);
}

static inline void
onPaint(HWND wnd) {
	if (!GetUpdateRect(wnd, NULL, FALSE)) return;
	PAINTSTRUCT ps;
	HDC dc = BeginPaint(wnd, &ps);
	paint(wnd, dc, &ps.rcPaint);
	EndPaint(wnd, &ps);
}

static inline void
onPrintClient(HWND wnd, HDC dc, UINT opt) {
	RECT rc;
	GetClientRect(wnd, &rc);
	paint(wnd, dc, &rc);
}

static inline void
onWindowPosChanged(HWND wnd, const LPWINDOWPOS wp) {
	if (!wnd_sizeChanged(wp)) return;

	//int clientCx = wnd_getClientCx(wnd);
	//wnd_setCx(GetDlgItem(wnd, kDlgAlbum), clientCx);
	SCROLLINFO si = { sizeof(SCROLLINFO), SIF_POS };
	GetScrollInfo(wnd, SB_VERT, &si);
	int pos = si.nPos;

	si.fMask = SIF_PAGE;
	si.nPage = wnd_getClientCy(wnd);
	SetScrollInfo(wnd, SB_VERT, &si, TRUE);

	si.fMask = SIF_POS;
	GetScrollInfo(wnd, SB_VERT, &si);
	if (si.nPos != pos) {
		ScrollWindowEx(wnd, 0, pos - si.nPos, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE | SW_SCROLLCHILDREN);
	}
}

static inline void
updateScroll(const WndData* wd) {
	SCROLLINFO si = { sizeof(SCROLLINFO), SIF_POS };
	GetScrollInfo(wd->wnd, SB_VERT, &si);
	int pos = si.nPos;

	si.fMask = SIF_RANGE | SIF_PAGE;
	si.nMax = getFullCy(wd) - 1;
	si.nPage = wnd_getClientCy(wd->wnd);
	SetScrollInfo(wd->wnd, SB_VERT, &si, TRUE);

	si.fMask = SIF_POS;
	GetScrollInfo(wd->wnd, SB_VERT, &si);
	if (si.nPos != pos) {
		ScrollWindowEx(wd->wnd, 0, pos - si.nPos, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE | SW_SCROLLCHILDREN);
	}
	InvalidateRect(wd->wnd, NULL, TRUE);
}

static inline bool
isHotValid(const WndData* wd) {
	return wd->hot <= wd->trackLast - wd->trackFirst;
}

static inline void
hideTrackPanel(WndData* wd)
{
	if (!isHotValid(wd)) return;

	wd->hot = -1;
	HWND panel = GetDlgItem(wd->wnd, kDlgTrack);
	if (!IsWindowVisible(panel)) return;

	if (IsChild(panel, GetFocus())) {
		msg_sendFocusNextCtl(wd->wnd, kDlgTrack, true);
	}
	ShowWindow(panel, SW_HIDE);
}

static inline bool
onSetTrackRange(HWND wnd, int first, int last) {
	assert(first > 0 && first <= last && last < 100);
	if (!cdt_setTrackRange(&theCdt, first, last)) return false;
	if (!msg_postThreadSetTrackRange(theThreads.formStatusUpdater.id, first, last)) return false;

	USE_WNDDATA;
	wd->trackFirst = first;
	wd->trackLast = last;
	
	hideTrackPanel(wd);
	updateScroll(wd);
	
	return true;
}

static inline bool
isHidingAlbumToc(const WndData* wd, BYTE fields) {
	return wd->isTocVisible && !(fields & 1 << albumFields_kToc);
}

static inline bool
isAlbumTocVisible(BYTE fields) {
	return fields & 1 << albumFields_kToc;
}

static inline void
setTrackTocVisibility(WndData* wd) {
	HWND panel = GetDlgItem(wd->wnd, kDlgTrack);
	BYTE fields = msg_sendGetFields(panel, fields_kTrack);
	fields = bit_flip8(fields, trackFields_kToc);
	msg_sendSetFields(panel, fields_kTrack, fields);
	wd->trackCy = wnd_getCy(panel);
	msg_sendSetFields(GetDlgItem(wd->wnd, kDlgMemTrack), fields_kTrack, fields);
}

static inline bool
handleSetAlbumFields(WndData* wd, BYTE fields) {
	HWND trackPanel = GetDlgItem(wd->wnd, kDlgTrack);
	if (isHidingAlbumToc(wd, fields) && trackPanel_isOnlyTocVisible(trackPanel)) return false;

	const type = fields_kAlbum;
	HWND panel = GetDlgItem(wd->wnd, kDlgAlbum);
	bool result = msg_sendSetFields(panel, type, fields);
	wd->albumCy = wnd_getCy(panel);
	if (wd->isTocVisible != isAlbumTocVisible(fields)) {
		wd->isTocVisible = !wd->isTocVisible;
		setTrackTocVisibility(wd);
	}
	return result;
}

static inline bool
isTrackTocVisible(BYTE fields) {
	return fields & 1 << trackFields_kToc;
}

static inline void
setAlbumTocVisibility(WndData* wd) {
	HWND panel = GetDlgItem(wd->wnd, kDlgAlbum);
	BYTE fields = msg_sendGetFields(panel, fields_kAlbum);
	fields = bit_flip8(fields, albumFields_kToc);
	msg_sendSetFields(panel, fields_kAlbum, fields);
	wd->albumCy = wnd_getCy(panel);
}

static inline bool
handleSetTrackFields(WndData* wd, BYTE fields) {
	const type = fields_kTrack;

	HWND panel = GetDlgItem(wd->wnd, kDlgTrack);
	bool result = msg_sendSetFields(panel, type, fields);
	if (!result) return false;

	wd->trackCy = wnd_getCy(panel);
	msg_sendSetFields(GetDlgItem(wd->wnd, kDlgMemTrack), type, fields);
	if (wd->isTocVisible != isTrackTocVisible(fields)) {
		wd->isTocVisible = !wd->isTocVisible;
		setAlbumTocVisibility(wd);
	}
	return result;
}

static inline bool
onSetFields(HWND wnd, int type, BYTE fields) {
	USE_WNDDATA;
	bool result = false;

	hideTrackPanel(wd);
	
	switch (type) {
	case fields_kAlbum:
		result = handleSetAlbumFields(wd, fields);
		break;
	case fields_kTrack:
		result = handleSetTrackFields(wd, fields);
		break;
	}

	if (result) updateScroll(wd);
	return result;
}

static inline BYTE
onGetFields(HWND wnd, int type) {
	switch (type) {
	case fields_kAlbum:
		return msg_sendGetFields(GetDlgItem(wnd, kDlgAlbum), type);
	case fields_kTrack:
		return msg_sendGetFields(GetDlgItem(wnd, kDlgTrack), type);
	}
	return (BYTE)0;
}

static inline int
getScrollStep(HWND wnd) {
	RECT rc = { 0 };
	SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &rc.top, 0);
	rc.top *= kRcLineH;
	MapDialogRect(GetDlgItem(wnd, kDlgTrack), &rc);
	return rc.top;
}

static inline void
onVScroll(HWND wnd, HWND ctl, UINT code, int pos) {
	int step = getScrollStep(wnd);

	SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
	GetScrollInfo(wnd, SB_VERT, &si);
	pos = si.nPos;

	switch (code) {
	case SB_LINEUP:
		si.nPos -= step;
		break;
	case SB_LINEDOWN:
		si.nPos += step;
		break;
	case SB_PAGEUP:
		si.nPos -= si.nPage;
		break;
	case SB_PAGEDOWN:
		si.nPos += si.nPage;
		break;
	case SB_THUMBTRACK:
		si.nPos = si.nTrackPos;
		break;
	case SB_TOP:
		si.nPos = si.nMin;
		break;
	case SB_BOTTOM:
		si.nPos = si.nMax;
		break;
	}
	si.fMask = SIF_POS;
	SetScrollInfo(wnd, SB_VERT, &si, TRUE);
	GetScrollInfo(wnd, SB_VERT, &si);
	if (si.nPos != pos) {
		ScrollWindowEx(wnd, 0, pos - si.nPos, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE | SW_SCROLLCHILDREN);
	}
}

static inline void
scrollWheelDelta(const WndData* wd, int delta, bool pageMode) {
	if (abs(delta) < WHEEL_DELTA) return;
	delta /= WHEEL_DELTA;

	SCROLLINFO si = { sizeof(SCROLLINFO), SIF_POS | SIF_PAGE };
	GetScrollInfo(wd->wnd, SB_VERT, &si);
	int pos = si.nPos;

	delta *= pageMode ? si.nPage : getScrollStep(wd->wnd);
	si.fMask = SIF_POS;
	si.nPos -= delta;
	SetScrollInfo(wd->wnd, SB_VERT, &si, TRUE);
	GetScrollInfo(wd->wnd, SB_VERT, &si);
	if ( si.nPos != pos) {
		ScrollWindowEx(wd->wnd, 0, pos - si.nPos, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE | SW_SCROLLCHILDREN);
	}
}

static inline void
onMouseWheel(HWND wnd, int keys, int delta, int x, int y) {
	USE_WNDDATA;
	wd->wheelDelta += delta;
	scrollWheelDelta(wd, wd->wheelDelta, keys & MK_CONTROL);
	wd->wheelDelta %= WHEEL_DELTA;
}

static inline void
onSetFocus(HWND wnd, HWND oldFocus) {
	SetFocus(GetDlgItem(wnd, kDlgAlbum));
}

static inline void
invalidateTrack(const WndData* wd, int index) {
	RECT rc;
	getTrackRect(wd, index, &rc);
	InvalidateRect(wd->wnd, &rc, FALSE);
	//dbg_printf(L"invalidate track %d @%d [%d]\n", index + 1, rc.top, rc.bottom - rc.top);
}

// use client coordinates
static inline bool
validateMouseMove(WndData* wd, int x, int y) {
	POINT pt = { x ,y };
	bool result = false;
	if (sys_mayDrag(&wd->lastMousePos, &pt)) result = true;
	wd->lastMousePos.x = x;
	wd->lastMousePos.y = y;
	return result;
}

static inline void
editHotTrack(const WndData* wd) {
	assert(wd->hot >= 0 && wd->hot <= wd->trackLast - wd->trackFirst);
	HWND panel = GetDlgItem(wd->wnd, kDlgTrack);
	wnd_setRedraw(panel, false);
	trackPanel_fill(panel, wd->block, wd->hot);
	SetWindowPos(panel, NULL, 0, getTrackY(wd, wd->hot), 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_NOSIZE | SWP_SHOWWINDOW);
	wnd_setRedraw(panel, true);
	if (IsChild(panel, GetFocus())) return;
	wnd_focusThisCtl(panel, GetDlgItem(panel, kIdTitle));
}

static inline void
onMouseMove(HWND wnd, int x, int y, UINT flags) {
	USE_WNDDATA;
	if (!validateMouseMove(wd, x, y)) return;

	int index = trackFromPt(wd, x, y);
	if (wd->hot == index) return;
	if (index < 0) return;

	if (wd->hot >= 0) {
		trackPanel_saveCurrentCtl(GetDlgItem(wd->wnd, kDlgTrack));
		invalidateTrack(wd, wd->hot);
	}
	wd->hot = index;
	editHotTrack(wd);
}

enum ScrollType{
	kAbsolute,
	kRelative,
};

static inline void
scrollTo(const WndData* wd, int pos, enum ScrollType type) {
	SCROLLINFO si = { sizeof(SCROLLINFO), SIF_POS };
	GetScrollInfo(wd->wnd, SB_VERT, &si);
	int oldPos = si.nPos;

	if (type == kRelative) si.nPos += pos;
	else si.nPos = pos;
	SetScrollInfo(wd->wnd, SB_VERT, &si, TRUE);
	GetScrollInfo(wd->wnd, SB_VERT, &si);
	if (si.nPos != oldPos) {
		ScrollWindowEx(wd->wnd, 0, oldPos - si.nPos, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE | SW_SCROLLCHILDREN);
	}
}

static inline void
resetHotTest(WndData* wd) {
	GetCursorPos(&wd->lastMousePos);
	MapWindowPoints(NULL, wd->wnd, &wd->lastMousePos, 1);
}

// Track panel sent WM_GOTONEXT
static inline bool
onGotoNext(HWND wnd, int which) {
	USE_WNDDATA;
	resetHotTest(wd);
	HWND trackPanel = GetDlgItem(wd->wnd, kDlgTrack);

	switch (which) {
	case goto_kNext:
		if (wd->hot < wd->trackLast - wd->trackFirst) {
			trackPanel_saveCurrentCtl(trackPanel);
			invalidateTrack(wd, wd->hot);
			++wd->hot;
			editHotTrack(wd);
			if (!isTrackFullyVisible(wd, wd->hot)) {
				scrollTo(wd, wd->trackCy, kRelative);
			}
			return true;
		}
		break;
	case goto_kPrev:
		if (wd->hot > 0) {
			trackPanel_saveCurrentCtl(trackPanel);
			invalidateTrack(wd, wd->hot);
			--wd->hot;
			editHotTrack(wd);
			if (!isTrackFullyVisible(wd, wd->hot)) {
				scrollTo(wd, -wd->trackCy, kRelative);
			}
			if (isAlbumPartiallyVisible(wd)) {
				scrollTo(wd, 0, kAbsolute);
			}
			return true;
		}
		break;
	case goto_kFirst:
		if (wd->hot > 0) {
			invalidateTrack(wd, wd->hot);
		}
		wd->hot = 0;
		editHotTrack(wd);
		return true;
		break;
	}
	return false;
}

static inline void
onNav(HWND wnd, int index) {
	USE_WNDDATA;
	wd->block = index;
	wd->completeness = cdt_blockGetCompleteness(theCdt.blocks + index);

	albumPanel_fill(GetDlgItem(wnd, kDlgAlbum), index);
	if (wd->hot >= 0) {
		trackPanel_fill(GetDlgItem(wnd, kDlgTrack), index, wd->hot);
	}
	InvalidateRect(wnd, NULL, TRUE);
}

static inline int
onGetNav(HWND wnd) {
	USE_WNDDATA;
	return wd->block;
}

static inline void
onCompletenessChanged(HWND wnd, bool allBlocks) {
	USE_WNDDATA;
	int completeness = cdt_blockGetCompleteness(theCdt.blocks + wd->block);
	if (wd->completeness == completeness) return;
	wd->completeness = completeness;
	msg_sendCompletenessChanged(GetParent(wnd), false);
}

static inline void
onLanguageChanged(HWND wnd) {
	USE_WNDDATA;
	if (wd->completeness == cdt_kNone) return;
	msg_sendCompletenessChanged(GetParent(wnd), false);
}

static inline bool
onDataChanged(HWND wnd, bool showAllFields) {
	USE_WNDDATA;
	if (wd->hot >= 0) {
		ShowWindow(GetDlgItem(wnd, kDlgTrack), SW_HIDE);
		wd->hot = -1;
		resetHotTest(wd);
	}
	wd->trackFirst = theCdt.trackFirst;
	wd->trackLast = theCdt.trackLast;
	if (!theCdt.blocks[wd->block].heap) wd->block = cdt_getFirstValidBlockIndex(&theCdt);
	wd->completeness = cdt_blockGetCompleteness(theCdt.blocks + wd->block);

	updateScroll(wd);
	albumPanel_fill(GetDlgItem(wnd, kDlgAlbum), wd->block);

	return true;
}

static inline void
updateScrollForDpi(WndData* wd) {
	SCROLLINFO si = { sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE | SIF_POS };
	si.nMax = getFullCy(wd) - 1;
	si.nPage = wnd_getClientCy(wd->wnd);
	si.nPos = 0;
	SetScrollInfo(wd->wnd, SB_VERT, &si, FALSE);
}

static inline void
onDpiChangedAfterParent(HWND wnd) {
	USE_WNDDATA;
	HWND album = GetDlgItem(wnd, kDlgAlbum);
	HWND track = GetDlgItem(wnd, kDlgTrack);
	RECT rc = {
		0,
		albumPanel_getDialogH(album),
		kRcPanelW,
		trackPanel_getDialogH(track)
	};
	MapDialogRect(album, &rc); // album and track use the same font and therefore share the same dialog base units

	//dbg_printf(L"track cx %d -> %d\n", wd->trackCx, rc.right);
	wd->trackCx = rc.right;
	//dbg_printf(L"track cy %d -> %d\n", wd->trackCy, rc.bottom);
	wd->trackCy = rc.bottom;

	//dbg_printf(L"album cy %d -> %d\n", wd->albumCy, rc.top);
	wd->albumCy = rc.top;
	SetWindowPos(album, NULL, 0, 0, wd->trackCx, wd->albumCy, SWP_NOZORDER | SWP_NOSENDCHANGING | SWP_NOMOVE | SWP_NOCOPYBITS);

	SetWindowPos(track, NULL, 0, 0, wd->trackCx, wd->trackCy, SWP_NOZORDER | SWP_NOSENDCHANGING | SWP_NOMOVE | SWP_NOCOPYBITS | SWP_HIDEWINDOW);
	wd->hot = -1;
	track = GetDlgItem(wnd, kDlgMemTrack);
	SetWindowPos(track, NULL, 0, 0, wd->trackCx, wd->trackCy, SWP_NOZORDER | SWP_NOSENDCHANGING | SWP_NOMOVE | SWP_NOCOPYBITS);

	updateScrollForDpi(wd);
	//resetHotTest(wd);
	InvalidateRect(wnd, NULL, TRUE);
	msg_postLayoutChanged(GetParent(wnd), kDlgAlbum);
}

//static inline void
//onShowRect(HWND wnd, HWND ctl) {
//	RECT parentRc, childRc, rc;
//	GetWindowRect(ctl, &childRc);
//	MapWindowPoints(ctl, wnd, (POINT*)&childRc, 2);
//	GetClientRect(wnd, &parentRc);
//	const childCy = childRc.bottom - childRc.top;
//	parentRc.top += childCy;
//	parentRc.bottom -= childCy;
//	if (IntersectRect(&rc, &parentRc, &childRc)) return;
//
//	const y = (parentRc.top + parentRc.bottom) / 2 - childCy;
//	USE_WNDDATA;
//	scrollTo(wd, y, kAbsolute);
//}

static LRESULT WINAPI
wndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		HANDLE_MSG(wnd, WM_CREATE, onCreate);
		HANDLE_MSG(wnd, WM_DESTROY, onDestroy);
		HANDLE_MSG(wnd, WM_WINDOWPOSCHANGED, onWindowPosChanged);
		HANDLE_MSG(wnd, WM_PAINT, onPaint);
		HANDLE_MSG(wnd, WM_SETFOCUS, onSetFocus);
		HANDLE_MSG(wnd, WM_VSCROLL, onVScroll);
		HANDLE_MSG(wnd, WM_MOUSEMOVE, onMouseMove);
		MSG_HANDLE_WM_PRINTCLIENT(wnd, onPrintClient);
		MSG_HANDLE_WM_MOUSEWHEEL(wnd, onMouseWheel);
		MSG_HANDLE_WM_DPICHANGED_AFTERPARENT(wnd, onDpiChangedAfterParent);
		MSG_HANDLE_SETTRACKRANGE(wnd, onSetTrackRange);
		MSG_HANDLE_SETFIELDS(wnd, onSetFields);
		MSG_HANDLE_GETFIELDS(wnd, onGetFields);
		MSG_HANDLE_DATACHANGED(wnd, onDataChanged);
		MSG_HANDLE_COMPLETENESSCHANGED(wnd, onCompletenessChanged);
		MSG_HANDLE_LANGUAGECHANGED(wnd, onLanguageChanged);
		//HANDLE_MSG(wnd, WM_COMMAND, onCommand);
		//HANDLE_MSG(wnd, WM_DRAWITEM, onDrawItem);
		//HANDLE_MSG(wnd, WM_MEASUREITEM, onMeasureItem);
		//HANDLE_MSG(wnd, WM_CTLCOLORLISTBOX, onCtlColorListBox);
		MSG_HANDLE_NAV(wnd, onNav);
		MSG_HANDLE_GETNAV(wnd, onGetNav);
		MSG_HANDLE_GOTONEXT(wnd, onGotoNext);
		//MSG_HANDLE_SHOWRECT(wnd, onShowRect);
		MSG_FORWARD_FOCUSNEXTCTL(GetParent(wnd));
		MSG_FORWARD_GETLAYOUTHINT(GetDlgItem(wnd, kDlgAlbum));
	}
	return DefWindowProc(wnd, msg, wParam, lParam);
}

static inline bool
registerClass(void) {
	WNDCLASS wc = {
		.style = 0,
		.lpfnWndProc = wndProc,
		.hInstance = GetModuleHandle(NULL),
		.hCursor = LoadCursor(NULL, IDC_ARROW),
		.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
		.lpszMenuName = NULL,
		.lpszClassName = kClsName,
	};
	return RegisterClass(&wc);
}

HWND
body_create(HWND parent, intptr_t id) {
	if (!registerClass()) return NULL;

	WndData* wd = calloc(1, sizeof(WndData));
	if (!wd) return NULL;

	return CreateWindowEx(
		WS_EX_NOPARENTNOTIFY | WS_EX_CONTROLPARENT,
		kClsName, NULL,
		WS_VISIBLE | WS_CHILD | WS_VSCROLL,
		0, 0, 0, 0,
		parent, (HMENU)id, GetModuleHandle(NULL), wd);
}
