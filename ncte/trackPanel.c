#include "trackPanel.h"

#include <windowsx.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "resdefs.h"
#include "wnddata.h"
#include "msg.h"
#include "wnd.h"
#include "font.h"
#include "commctrl.h"
#include "dpi.h"
#include "debug.h"
#include "cdt.h"
#include "str.h"
#include "thread.h"
#include "color.h"
#include "kbd.h"


enum {
	kSubclass = 1,
};

enum {
	kNext = 0x01,
	kPrev = 0x02,
};

static const Line kOptionalLines[] = {
	{ kIdTitleLabel, kIdTitle },
	{ kIdArtistLabel, kIdArtist },
	{ kIdSongwriterLabel, kIdSongwriter },
	{ kIdComposerLabel, kIdComposer },
	{ kIdArrangerLabel, kIdArranger },
	{ kIdCodeLabel, kIdCode },
	{ kIdMessageLabel, kIdMessage },
	{ kIdTocLabel, kIdToc },
};

// USERDATA USAGE
// 0x     00 00          00      00
//    bottomOptEditId          fields

static inline BYTE
getFields(HWND wnd) {
	return (BYTE)GetWindowLongPtr(wnd, GWLP_USERDATA);
}

static inline void
handleEditTab(HWND ctl, BYTE flags) {
	HWND panel = GetParent(ctl);
	if (kbd_isShiftDown()) {
		if (flags & kPrev) {
			if (sendGotoNext(panel, goto_kPrev)) {
				wnd_focusNextCtl(panel, true);
			}
			else {
				msg_sendFocusNextCtl(panel, kDlgTrack, true);
			}
		}
		else {
			wnd_focusNextCtl(panel, true);
		}
	}
	else {
		if (flags & kNext) {
			if (sendGotoNext(panel, goto_kNext)) {
				wnd_focusNextCtl(panel, false);
			}
			else {
				msg_sendFocusNextCtl(panel, kDlgTrack, false);
				SendMessage(GetParent(panel), WM_VSCROLL, SB_TOP, 0);
			}
		}
		else {
			wnd_focusNextCtl(panel, false);
		}
	}
}

static LRESULT CALLBACK
editTabProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR refData) {
	switch (msg) {
	case WM_GETDLGCODE:
		return DLGC_WANTTAB | DefSubclassProc(wnd, msg, wParam, lParam);
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_TAB:
			handleEditTab(wnd, (BYTE)refData);
			return 0;
		}
		break;
	}
	return DefSubclassProc(wnd, msg, wParam, lParam);
}

static inline bool
isVirtual(HWND wnd) {
	return kDlgMemTrack == (WORD)GetWindowLongPtr(wnd, GWLP_ID);
}

static inline void
saveTrackIndex(HWND panel, int index) {
	SetWindowLongPtr(panel, DWLP_USER, (LONG_PTR)index);
}

static inline int
retrieveTrackIndex(HWND panel) {
	return (int)GetWindowLongPtr(panel, DWLP_USER);
}

static inline void
onWindowPosChanged(HWND wnd, const WINDOWPOS* wp) {
	//OutputDebugString(L"track panel pos changed\n");
	dlg_setResult(wnd, 0);
	// blank
	//if ((wp->flags & SWP_NOSIZE) || wp->cx <= 0) return;
	//int x = getEditX();
	//int cx = wp->cx - x - dpi_scale(kMarginCx) * 3;
	//dbg_printf(L"pos changed: wp->cx: %d cx: %d\n", wp->cx, cx);
	//wnd_setCx(GetDlgItem(wnd, kIdTitle), cx);
	//wnd_setCx(GetDlgItem(wnd, kIdArtist), cx);
}

static inline void
onDrawItem(HWND wnd, int id, DRAWITEMSTRUCT* di) {
	HDC dc = di->hDC;
	SaveDC(dc);

	SetGraphicsMode(dc, GM_ADVANCED);

	SelectObject(dc, GetStockObject(NULL_PEN));
	SetDCBrushColor(dc, kColorTagBk);
	SelectObject(dc, GetStockObject(DC_BRUSH));
	RECT rc = { -kRcTLabelX - 6, -kRcLineH + 4, kRcTLabelX - 6, kRcLineH + 4 };
	OffsetRect(&rc, -2, -5);
	MapDialogRect(wnd, &rc);
	Pie(dc, rc.left, rc.top, rc.right, rc.bottom, (rc.left + rc.right) / 2, rc.bottom, rc.right, (rc.top + rc.bottom) / 2);

	SelectObject(dc, GetSysColorBrush(COLOR_BTNFACE));
	//InflateRect(&rc, -5, -2);
	SetRect(&rc, -21, -9, 21, 9);
	OffsetRect(&rc, -2, -5);
	MapDialogRect(wnd, &rc);
	Ellipse(dc, rc.left, rc.top, rc.right, rc.bottom);

	rc.left = 8;
	rc.top = 4;
	MapDialogRect(wnd, &rc);
	SetTextColor(dc, kColorTagText);
	SetBkMode(dc, TRANSPARENT);
	SetTextAlign(dc, TA_CENTER);
	SelectObject(dc, GetWindowFont(wnd));
	XFORM xf = {
		0.9659258f, -0.258819045f,
		0.258819045f, 0.9659258f,
		0.0f, 0.0f
	};
	SetWorldTransform(dc, &xf);
	int len;
	const wchar_t* text = wnd_getText(di->hwndItem, &len);
	ExtTextOut(dc, rc.left, rc.top, 0, NULL, text, len, NULL);

	RestoreDC(dc, -1);

	dlg_setResult(wnd, TRUE);
}

//static inline void
//onDrawItem(HWND wnd, int id, DRAWITEMSTRUCT* di) {
//	HDC dc = di->hDC;
//	SaveDC(dc);
//
//	SetGraphicsMode(dc, GM_ADVANCED);
//
//	SelectObject(dc, GetStockObject(NULL_PEN));
//	SetDCBrushColor(dc, RGB(90, 126, 143));
//	SelectObject(dc, GetStockObject(DC_BRUSH));
//	RECT rc = { -24, -18, 24, 18 };
//	MapDialogRect(wnd, &rc);
//	Pie(dc, rc.left, rc.top, rc.right, rc.bottom, (rc.left + rc.right) / 2, rc.bottom, rc.right, (rc.top + rc.bottom) / 2);
//
//	SelectObject(dc, GetSysColorBrush(COLOR_BTNFACE));
//	SetRect(&rc, -9, -9, 9, 9);
//	MapDialogRect(wnd, &rc);
//	Ellipse(dc, rc.left, rc.top, rc.right, rc.bottom);
//
//	rc.left = 0;
//	rc.top = 8;
//	MapDialogRect(wnd, &rc);
//	SetTextColor(dc, RGB(241, 241, 241));
//	SetBkMode(dc, TRANSPARENT);
//	SetTextAlign(dc, TA_CENTER);
//	SelectObject(dc, GetWindowFont(wnd));
//	XFORM xf = {
//		0.70710678f, -0.70710678f,
//		0.70710678f, 0.70710678f,
//		0.0f, 0.0f
//	};
//	SetWorldTransform(dc, &xf);
//	int len;
//	const wchar_t* text = wnd_getText(di->hwndItem, &len);
//	ExtTextOut(dc, rc.left, rc.top, 0, NULL, text, len, NULL);
//
//	RestoreDC(dc, -1);
//
//	dlg_setResult(wnd, TRUE);
//}

static inline HWND
getTopEdit(HWND panel) {
	BYTE fields = getFields(panel);
	assert(fields);
	int i;
	for (i = 0; i < 8; ++i) {
		BYTE mask = 1 << i;
		if (fields & mask) break;
	}
	return GetDlgItem(panel, kOptionalLines[i].editId);
}

static inline HWND
getBottomEdit(HWND panel) {
	BYTE fields = getFields(panel);
	assert(fields);
	int i;
	for (i = 7; i >= 0; --i) {
		BYTE mask = 1 << i;
		if (fields & mask) break;
	}
	return GetDlgItem(panel, kOptionalLines[i].editId);
}

static inline void
subclassEdgeEdits(HWND panel) {
	HWND topCtl = getTopEdit(panel);
	HWND bottomCtl = getBottomEdit(panel);
	assert(topCtl && bottomCtl);
	if (topCtl == bottomCtl) {
		SetWindowSubclass(topCtl, editTabProc, kSubclass, kPrev | kNext);
	}
	else {
		SetWindowSubclass(topCtl, editTabProc, kSubclass, kPrev);
		SetWindowSubclass(bottomCtl, editTabProc, kSubclass, kNext);
	}
}

static inline void
unsubclassEdgeEdits(HWND panel) {
	HWND topCtl = getTopEdit(panel);
	HWND bottomCtl = getBottomEdit(panel);
	assert(topCtl && bottomCtl);
	RemoveWindowSubclass(topCtl, editTabProc, kSubclass);
	if (topCtl != bottomCtl) {
		RemoveWindowSubclass(bottomCtl, editTabProc, kSubclass);
	}
}

static inline BOOL
onInitDialog(HWND wnd, HWND focusWnd, LPARAM lp) {
	SetWindowLongPtr(wnd, GWLP_USERDATA, 0xFF); // Set all optional fields visible

	if ((WORD)lp == kDlgMemTrack) return FALSE;

	subclassEdgeEdits(wnd);

	static const wchar_t kAscii[] = L"ASCII";
	static const wchar_t kTime[] = L"m:s:f";

	HWND ctl = GetDlgItem(wnd, kIdCode);
	commctrl_editSetLimitText(ctl, cdt_kCchIsrc);
	Edit_SetCueBannerTextFocused(ctl, kAscii, FALSE);

	ctl = GetDlgItem(wnd, kIdToc);
	commctrl_editSetLimitText(ctl, cdt_kCchToc);
	Edit_SetCueBannerTextFocused(ctl, kTime, FALSE);

	return FALSE;
}

static inline void
setFields(HWND wnd, BYTE fields) {
	if (!fields) {
		dlg_setResult(wnd, 0);
		return;
	}

	if (isVirtual(wnd)) {
		fields_set(wnd, fields, kRcLine1Y, kOptionalLines, _countof(kOptionalLines));
		dlg_setResult(wnd, 1);
		return;
	}

	unsubclassEdgeEdits(wnd);
	fields_set(wnd, fields, kRcLine1Y, kOptionalLines, _countof(kOptionalLines));
	subclassEdgeEdits(wnd);
	dlg_setResult(wnd, 1);
}

static inline void
onGetFields(HWND wnd) {
	dlg_setResult(wnd, fields_retrieveFields(wnd));
}

//static inline void
//onCommand(HWND wnd, int id, int code, HWND ctl) {
//	switch (code) {
//	case EN_KILLFOCUS:
//		//dbg_printf(L"  trackpanel id %d kill focus\n", id);
//		break;
//	}
//	dlg_setResult(wnd, 0);
//}

static inline void
onDestroy(HWND wnd) {
	if (isVirtual(wnd)) return;
	unsubclassEdgeEdits(wnd);
}

static inline void
handleTitleChanged(HWND panel, HWND ctl) {
	bool hadText = (bool)GetWindowLongPtr(ctl, GWLP_USERDATA);
	bool hasText = (bool)GetWindowTextLength(ctl);
	if (hasText == hadText) return;

	int blockIndex = msg_sendGetNav(GetParent(panel));
	int trackIndex = retrieveTrackIndex(panel);
	cdt_Block* block = theCdt.blocks + blockIndex;
	cdt_Track* track = block->tracks + trackIndex;
	cdt_setTextFromCtl(block, &track->title, ctl);

	SetWindowLongPtr(ctl, GWLP_USERDATA, (LONG_PTR)hasText);
	msg_sendCompletenessChanged(GetParent(panel), false);
}

static inline void
saveCtlText(HWND panel, HWND ctl, int id) {
	int blockIndex = msg_sendGetNav(GetParent(panel));
	int trackIndex = retrieveTrackIndex(panel);
	cdt_Block* block = theCdt.blocks + blockIndex;
	cdt_Track* track = block->tracks + trackIndex;
	switch (id) {
	case kIdTitle:
		cdt_setTextFromCtl(block, &track->title, ctl);
		break;
	case kIdArtist:
		cdt_setTextFromCtl(block, &track->artist, ctl);
		break;
	case kIdSongwriter:
		cdt_setTextFromCtl(block, &track->songwriter, ctl);
		break;
	case kIdComposer:
		cdt_setTextFromCtl(block, &track->composer, ctl);
		break;
	case kIdArranger:
		cdt_setTextFromCtl(block, &track->arranger, ctl);
		break;
	case kIdCode:
		cdt_setTextFromCtlUsingEncoding(block, &track->isrc, ctl, cdt_kEncodingIndexAscii);
		break;
	case kIdMessage:
		cdt_setTextFromCtl(block, &track->message, ctl);
		break;
	case kIdToc:
		cdt_timeFromCtl(theCdt.toc + trackIndex, ctl);
		break;
	}
}

//static inline void
//handleEditSetFocus(HWND panel, HWND ctl, int id) {
//	msg_sendShowRect(GetParent(panel), ctl);
//	//const trackIndex = retrieveTrackIndex(panel);
//	//if (trackIndex != theCdt.trackLast - theCdt.trackFirst) return;
//	//HWND bottomCtl = getBottomEdit(panel);
//	//if (ctl != bottomCtl) return;
//	//SendMessage(GetParent(panel), WM_VSCROLL, SB_BOTTOM, 0);
//}

static inline void
handleEditKillFocus(HWND panel, HWND ctl, int id) {
	saveCtlText(panel, ctl, id);
}

static inline void
onCommand(HWND wnd, int id, int code, HWND ctl) {
	switch (code) {
	case EN_CHANGE:
		if (id == kIdTitle) handleTitleChanged(wnd, ctl);
		break;
	//case EN_SETFOCUS:
	//	handleEditSetFocus(wnd, ctl, id);
	//	break;
	case EN_KILLFOCUS:
		handleEditKillFocus(wnd, ctl, id);
		break;
	}
	dlg_setResult(wnd, 0);
}

static INT_PTR CALLBACK
dlgProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		HANDLE_MSG(wnd, WM_INITDIALOG, onInitDialog);
	case WM_DESTROY:
		onDestroy(wnd);
		break;
	case WM_WINDOWPOSCHANGED:
		onWindowPosChanged(wnd, (const WINDOWPOS*)lParam);
		return TRUE;
	case WM_DRAWITEM:
		onDrawItem(wnd, (int)wParam, (DRAWITEMSTRUCT*)lParam);
		return TRUE;
	case MSG_SETFIELDS:
		setFields(wnd, (BYTE)lParam);
		return TRUE;
	case MSG_GETFIELDS:
		onGetFields(wnd);
		return TRUE;
	case MSG_GOTONEXT:
		dlg_setResult(wnd, SendMessage(GetParent(wnd), msg, wParam, lParam));
		return TRUE;
	case MSG_FOCUSNEXTCTL:
		SendMessage(GetParent(wnd), msg, wParam, lParam);
		return TRUE;
	case WM_COMMAND:
		if (isVirtual(wnd)) return TRUE;
		onCommand(wnd, (int)LOWORD(wParam), (int)HIWORD(wParam), (HWND)lParam);
		return TRUE;
	}
	return FALSE;
}

HWND
trackPanel_create(HWND parent, intptr_t id) {
	HWND wnd = CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(kDlgTrack), parent, dlgProc, id);
	if (!wnd) return NULL;
	SetWindowLongPtr(wnd, GWLP_ID, id);
	return wnd;
}

void
trackPanel_setFields(HWND wnd, const bool fields[trackFields_kEnd]) {
	setFields(wnd, fields_toByte(fields, trackFields_kEnd));
}

void
trackPanel_fill(HWND wnd, int blockIndex, int trackIndex) {
	saveTrackIndex(wnd, trackIndex);
	int trackNumber = trackIndex + theCdt.trackFirst;
	SetDlgItemInt(wnd, kIdTrackNumber, trackNumber, FALSE);

	const cdt_Track* track = theCdt.blocks[blockIndex].tracks + trackIndex;

	bool titleHasText = track->title && track->title[0];
	SetWindowLongPtr(GetDlgItem(wnd, kIdTitle), GWLP_USERDATA, (LONG_PTR)titleHasText);
	SetDlgItemText(wnd, kIdTitle, track->title);
	SetDlgItemText(wnd, kIdArtist, track->artist);
	SetDlgItemText(wnd, kIdSongwriter, track->songwriter);
	SetDlgItemText(wnd, kIdComposer, track->composer);
	SetDlgItemText(wnd, kIdArranger, track->arranger);
	SetDlgItemText(wnd, kIdCode, track->isrc);
	SetDlgItemText(wnd, kIdMessage, track->message);
	SetDlgItemText(wnd, kIdToc, cdt_timeToText(theCdt.toc + trackIndex));

	commctrl_dlgEditSetDirty(wnd, kIdTitle, false);
	commctrl_dlgEditSetDirty(wnd, kIdArtist, false);
	commctrl_dlgEditSetDirty(wnd, kIdSongwriter, false);
	commctrl_dlgEditSetDirty(wnd, kIdComposer, false);
	commctrl_dlgEditSetDirty(wnd, kIdArranger, false);
	commctrl_dlgEditSetDirty(wnd, kIdCode, false);
	commctrl_dlgEditSetDirty(wnd, kIdMessage, false);
	commctrl_dlgEditSetDirty(wnd, kIdToc, false);
}

void
trackPanel_saveCurrentCtl(HWND wnd) {
	HWND ctl = GetFocus();
	if (!IsChild(wnd, ctl)) return;
	if (!commctrl_editIsDirty(ctl)) return;
	int id = (int)GetWindowLongPtr(ctl, GWLP_ID);
	saveCtlText(wnd, ctl, id);
	//dbg_printf(L"trackPanel save current track for ctl %d\n", id);
}
