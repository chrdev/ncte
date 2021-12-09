#include "albumPanel.h"

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
#include "str.h"
#include "fields.h"
#include "thread.h"
#include "cdt.h"
#include "color.h"
#include "kbd.h"


static const Line kOptionalLines[] = {
	{ kIdSongwriterLabel, kIdSongwriter },
	{ kIdComposerLabel, kIdComposer },
	{ kIdArrangerLabel, kIdArranger },
	{ kIdCatalogLabel, kIdCatalog },
	{ kIdCodeLabel, kIdCode },
	{ kIdMessageLabel, kIdMessage },
	{ kIdClosedInfoLabel, kIdClosedInfo },
	{ kIdTocLabel, kIdToc },
};

// USERDATA USAGE
// 0x     00 00          00      00
//    bottomOptEditId          fields

enum {
	kSubclass = 1,
};

static inline void
handleBottomEditTab(HWND wnd) {
	HWND dlg = GetParent(wnd);

	if (kbd_isShiftDown()) {
		wnd_focusNextCtl(dlg, true);
	}
	else {
		sendGotoNext(dlg, goto_kFirst);
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
			handleBottomEditTab(wnd);
			return 0;
		}
		break;
	}
	return DefSubclassProc(wnd, msg, wParam, lParam);
}

static inline void
onWindowPosChanged(HWND wnd, const WINDOWPOS* wp) {
	//OutputDebugString(L"album panel pos changed\n");
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
initEncodings(HWND parent) {
	HWND wnd = GetDlgItem(parent, kIdEncoding);
	for (int i = 0; i < (int)cdt_kEncodingCount; ++i) {
		commctrl_comboAddString(wnd, cdt_getEncodingText(i, NULL));
	}
	commctrl_comboSelectItem(wnd, 0);
}

static inline void
initLanguages(HWND parent) {
	HWND wnd = GetDlgItem(parent, kIdLanguage);
	//commctrl_comboSetDroppedWidth(wnd, (int)(1.62f * wnd_getCx(wnd)));
	commctrl_comboInitStorage(wnd, cdt_kLanguageCount, cdt_kLanguageCount * sizeof(wchar_t) * 20);
	for (int i = 0; i < (int)cdt_kLanguageCount; ++i) {
		commctrl_comboAddString(wnd, cdt_getLanguageText(i, NULL));
	}
	commctrl_comboSelectItem(wnd, 0);
}

static inline void
initGenres(HWND parent) {
	HWND wnd = GetDlgItem(parent, kIdGenre);
	//commctrl_comboSetDroppedWidth(wnd, (int)(1.62f * wnd_getCx(wnd)));
	for (int i = 0; i < (int)cdt_kGenreCount; ++i) {
		commctrl_comboAddString(wnd, cdt_getGenreText(i, NULL));
	}
	commctrl_comboSelectItem(wnd, 0);
}

static inline void
initEdits(HWND dlg) {
	static const wchar_t kAscii[] = L"ASCII";
	static const wchar_t k8859[] = L"8859-1";
	static const wchar_t kTimeFormat[] = L"m:s:f";
	Edit_SetCueBannerTextFocused(GetDlgItem(dlg, kIdGenreExtra), kAscii, FALSE);
	Edit_SetCueBannerTextFocused(GetDlgItem(dlg, kIdCatalog), kAscii, FALSE);
	Edit_SetCueBannerTextFocused(GetDlgItem(dlg, kIdCode), kAscii, FALSE);
	Edit_SetCueBannerTextFocused(GetDlgItem(dlg, kIdClosedInfo), k8859, FALSE);
	Edit_SetCueBannerTextFocused(GetDlgItem(dlg, kIdToc), kTimeFormat, FALSE);

	commctrl_editSetLimitText(GetDlgItem(dlg, kIdCode), cdt_kCchUpc);
	commctrl_editSetLimitText(GetDlgItem(dlg, kIdToc), cdt_kCchToc);
}

static inline void
initBottomEdit(HWND dlg) {
	SetWindowSubclass(GetDlgItem(dlg, albumPanel_kIdDefBottom), editTabProc, kSubclass, 0);
}

static inline BOOL
onInitDialog(HWND dlg, HWND focusWnd, LPARAM lp) {
	SetWindowLongPtr(dlg, GWLP_USERDATA, 0xFF); // Set all optional fields visible

	wnd_setFont(GetDlgItem(dlg, kIdCopyProtection), theFonts.icon.handle, setFont_kNoRedraw);
	commctrl_createTooltip(dlg, kIdCopyProtection, kTextCopyProtection);

	initEncodings(dlg);
	initLanguages(dlg);
	initGenres(dlg);
	initEdits(dlg);

	initBottomEdit(dlg);

	return FALSE;
}

static inline void
setFields(HWND wnd, BYTE fields) {
	int oldBottomId = albumPanel_getBottomEditId(wnd);
	fields_set(wnd, fields, kRcLine4Y, kOptionalLines, _countof(kOptionalLines));
	int bottomId = albumPanel_getBottomEditId(wnd);
	if (bottomId == oldBottomId) {
		dlg_setResult(wnd, 1);
		return;
	}

	RemoveWindowSubclass(GetDlgItem(wnd, oldBottomId), editTabProc, kSubclass);
	SetWindowSubclass(GetDlgItem(wnd, bottomId), editTabProc, kSubclass, 0);
	dlg_setResult(wnd, 1);
}

static inline void
onGetFields(HWND wnd) {
	dlg_setResult(wnd, fields_retrieveFields(wnd));
}

static inline void
drawCopyProtectButton(HWND wnd, HDC dc, RECT* rc, UINT state) {
	// state dosen't carry CDIS_CHECKED.
	// If pushed, state carries CDIS_SELECTED, but we can't send BM_SETCHECK here because that triggers another custom draw.
	// So we handle the check toggling at WM_COMMAND.
	// BS_AUTOCHECKBOX lose the auto feature when we do custom draw.
	bool checked = commctrl_buttonChecked(wnd);
	bool hot = state & CDIS_HOT;
	bool focus = state & CDIS_FOCUS;

	UINT edge;
	COLORREF color;
	const wchar_t* text;
	if (checked) {
		edge = EDGE_ETCHED;
		color = kColorLocked;
		text = L"L";
	}
	else {
		edge = EDGE_RAISED;
		color = kColorUnlocked;
		text = L"U";
	}

	DrawEdge(dc, rc, edge, BF_RECT | BF_ADJUST);
	SetDCBrushColor(dc, color);
	FillRect(dc, rc, GetStockObject(DC_BRUSH));

	SelectObject(dc, theFonts.icon.handle);
	SetBkMode(dc, TRANSPARENT);
	DrawText(dc, text, 1, rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);

	if (focus) DrawFocusRect(dc, rc);
}

static inline void
onNotify(HWND wnd, NMHDR* hdr) {
	if (hdr->idFrom != kIdCopyProtection) {
		dlg_setResult(wnd, 0);
		return;
	}
	NMCUSTOMDRAW* cd = (NMCUSTOMDRAW*)hdr;
	switch (cd->dwDrawStage) {
	case CDDS_PREERASE:
		drawCopyProtectButton(hdr->hwndFrom, cd->hdc, &cd->rc, cd->uItemState);
		dlg_setResult(wnd, CDRF_SKIPDEFAULT);
		break;
	}
}

static inline void
handleTitleChanged(HWND panel, HWND ctl) {
	bool hadText = (bool)GetWindowLongPtr(ctl, GWLP_USERDATA);
	bool hasText = (bool)GetWindowTextLength(ctl);
	if (hasText == hadText) return;

	int blockIndex = msg_sendGetNav(GetParent(panel));
	cdt_Block* block = theCdt.blocks + blockIndex;
	cdt_Album* album = block->album;
	cdt_setTextFromCtl(block, &album->title, ctl);

	SetWindowLongPtr(ctl, GWLP_USERDATA, (LONG_PTR)hasText);
	msg_sendCompletenessChanged(GetParent(panel), false);
}

static inline void
handleEncodingChanged(HWND wnd, HWND ctl) {
	int encoding = commctrl_comboGetCurSel(ctl);
	if (encoding == CB_ERR) return;

	HWND parent = GetParent(wnd);
	int blockIndex = msg_sendGetNav(parent);
	cdt_Block* block = theCdt.blocks + blockIndex;
	block->album->encoding = encoding;
	if (cdt_blockRegulate(block)) {
		msg_sendDataChanged(parent, false);
	}

	msg_sendLanguageChanged(parent);
}

static inline void
handleLanguageChanged(HWND wnd, HWND ctl) {
	int language = commctrl_comboGetCurSel(ctl);
	if (language == CB_ERR) return;

	HWND parent = GetParent(wnd);
	int block = msg_sendGetNav(parent);
	theCdt.blocks[block].album->language = language;

	msg_sendLanguageChanged(parent);
}

static inline void
handleGenreChanged(HWND wnd, HWND ctl) {
	int v = (int)SendMessage(ctl, CB_GETCURSEL, 0, 0);
	int block = msg_sendGetNav(GetParent(wnd));
	theCdt.blocks[block].album->genreCode = v;
}

static inline void
handleCopyProtectionChanged(HWND wnd, HWND ctl) {
	int block = msg_sendGetNav(GetParent(wnd));
	bool checked = commctrl_buttonToggleCheck(ctl);
	theCdt.blocks[block].album->copyProtected = checked;
}

static inline void
handleEditKillFocus(HWND wnd, HWND ctl, int id) {
	int blockIndex = msg_sendGetNav(GetParent(wnd));
	cdt_Block* block = theCdt.blocks + blockIndex;
	cdt_Album* album = block->album;
	switch (id) {
	case kIdGenreExtra:
		cdt_setTextFromCtlUsingEncoding(block, &album->genreExtra, ctl, cdt_kEncodingIndexAscii);
		break;
	case kIdTitle:
		cdt_setTextFromCtl(block, &album->title, ctl);
		break;
	case kIdArtist:
		cdt_setTextFromCtl(block, &album->artist, ctl);
		break;		
	case kIdSongwriter:
		cdt_setTextFromCtl(block, &album->songwriter, ctl);
		break;
	case kIdComposer:
		cdt_setTextFromCtl(block, &album->composer, ctl);
		break;
	case kIdArranger:
		cdt_setTextFromCtl(block, &album->arranger, ctl);
		break;
	case kIdCatalog:
		cdt_setTextFromCtlUsingEncoding(block, &album->catalog, ctl, cdt_kEncodingIndexAscii);
		break;
	case kIdCode:
		cdt_setTextFromCtlUsingEncoding(block, &album->upc, ctl, cdt_kEncodingIndexAscii);
		break;
	case kIdMessage:
		cdt_setTextFromCtl(block, &album->message, ctl);
		break;
	case kIdClosedInfo:
		cdt_setTextFromCtlUsingEncoding(block, &album->closedInfo, ctl, cdt_kEncodingIndex88591);
		break;
	case kIdToc:
		cdt_timeFromCtl(theCdt.toc + block->trackCount, ctl);
		break;
	}
}

static inline void
onCommand(HWND wnd, int id, int code, HWND ctl) {
	switch (id) {
	case kIdCopyProtection:
		if (code == BN_CLICKED) handleCopyProtectionChanged(wnd, ctl);
		break;
	case kIdEncoding:
		if (code == CBN_SELCHANGE) handleEncodingChanged(wnd, ctl);
		break;
	case kIdLanguage:
		if (code == CBN_SELCHANGE) handleLanguageChanged(wnd, ctl);
		break;
	case kIdGenre:
		if (code == CBN_SELCHANGE) handleGenreChanged(wnd, ctl);
		break;
	case kIdTitle:
		if (code == EN_CHANGE) {
			handleTitleChanged(wnd, ctl);
			break;
		}
		// fall-through
	default:
		if (code == EN_KILLFOCUS) handleEditKillFocus(wnd, ctl, id);
		break;
	}
	dlg_setResult(wnd, 0);
}

static inline void
onDestroy(HWND wnd) {
	int id = fields_retrieveBottomOptEditId(wnd);
	RemoveWindowSubclass(GetDlgItem(wnd, id), editTabProc, kSubclass);
}

static inline void
onGetLayoutHint(HWND wnd, WORD srcId, WORD* data) {
	assert(data[0] == 4);
	RECT rc;
	GetWindowRect(GetDlgItem(wnd, kIdGenreLabel), &rc);
	data[1] = (WORD)rc.left;
	GetWindowRect(GetDlgItem(wnd, kIdGenre), &rc);
	data[2] = (WORD)rc.left;
	GetWindowRect(GetDlgItem(wnd, kIdGenreExtra), &rc);
	data[3] = (WORD)rc.left;

	dlg_setResult(wnd, (LONG_PTR)wnd);
}

static INT_PTR CALLBACK
dlgProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		HANDLE_MSG(wnd, WM_INITDIALOG, onInitDialog);
	case WM_WINDOWPOSCHANGED:
		onWindowPosChanged(wnd, (const WINDOWPOS*)lParam);
		return TRUE;
	case MSG_SETFIELDS:
		setFields(wnd, (BYTE)lParam);
		return TRUE;
	case MSG_GETFIELDS:
		onGetFields(wnd);
		return TRUE;
	case WM_NOTIFY:
		onNotify(wnd, (NMHDR*)lParam);
		return TRUE;
	case WM_COMMAND:
		onCommand(wnd, (int)LOWORD(wParam), (int)HIWORD(wParam), (HWND)lParam);
		return TRUE;
	case WM_DESTROY:
		onDestroy(wnd);
		break;
	case MSG_GOTONEXT:
		dlg_setResult(wnd, SendMessage(GetParent(wnd), msg, wParam, lParam));
		return TRUE;
	case MSG_GETLAYOUTHINT:
		onGetLayoutHint(wnd, (WORD)wParam, (WORD*)lParam);
		return TRUE;
	}
	return FALSE;
}

HWND
albumPanel_create(HWND parent) {
	HWND wnd = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(kDlgAlbum), parent, dlgProc);
	if (!wnd) return NULL;
	SetWindowLongPtr(wnd, GWLP_ID, kDlgAlbum);
	return wnd;
}

void
albumPanel_setFields(HWND wnd, const bool fields[albumFields_kEnd]) {
	setFields(wnd, fields_toByte(fields, albumFields_kEnd));
}

void
albumPanel_fill(HWND wnd, int blockIndex) {
	const cdt_Block* block = theCdt.blocks + blockIndex;
	const cdt_Album* album = block->album;

	CheckDlgButton(wnd, kIdCopyProtection, album->copyProtected);
	commctrl_dlgComboSetCurSel(wnd, kIdEncoding, album->encoding);
	commctrl_dlgComboSetCurSel(wnd, kIdLanguage, album->language);
	commctrl_dlgComboSetCurSel(wnd, kIdGenre, album->genreCode);
	SetDlgItemText(wnd, kIdGenreExtra, album->genreExtra);

	bool titleHasText = album->title && album->title[0];
	SetWindowLongPtr(GetDlgItem(wnd, kIdTitle), GWLP_USERDATA, (LONG_PTR)titleHasText);
	SetDlgItemText(wnd, kIdTitle, album->title);
	SetDlgItemText(wnd, kIdArtist, album->artist);
	SetDlgItemText(wnd, kIdSongwriter, album->songwriter);
	SetDlgItemText(wnd, kIdComposer, album->composer);
	SetDlgItemText(wnd, kIdArranger, album->arranger);
	SetDlgItemText(wnd, kIdCatalog, album->catalog);
	SetDlgItemText(wnd, kIdCode, album->upc);
	SetDlgItemText(wnd, kIdMessage, album->message);
	SetDlgItemText(wnd, kIdClosedInfo, album->closedInfo);
	SetDlgItemText(wnd, kIdToc, cdt_timeToText(theCdt.toc + block->trackCount));
}
