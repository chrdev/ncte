#pragma once

#include <Windows.h> // WM_APP
#include <stdbool.h>


enum {
	command_kBegin,
	command_kNew = command_kBegin,
	command_kOpenFile,
	command_kSave,
	command_kOpenDrop, // LPARAM: HDROP
	command_kOpenText, // LPARAM: const wchar_t*
	command_kEnd,
};

enum Fields{
	fields_kAlbum,
	fields_kTrack,
};

enum {
	goto_kNext,
	goto_kPrev,
	goto_kFirst,
};

enum {
	MSG_NAV = WM_APP,
	// wParam: nav index, 0-based
	// lParam: not used

	MSG_GETNAV,
	// wParam: not used
	// lParam: not used
	// return: int, 0-based nav page index

	MSG_COMMAND,
	// wParam: enum command_k*
	// lParam: depends on wParam, none except commented, see command_k*

	MSG_SETFIELDS,
	// wParam: enum Fields, fields_kAlbum or fields_kTrack
	// lParam: BYTE fields, 
	// return: bool, whether the fields actually changed.

	MSG_GETFIELDS,
	// wParam: enum Fields, fields_kAlbum or fields_kTrack
	// lParam: not used
	// return: BYTE, result

	MSG_GETFILLEDFIELDS,
	// wParam: enum Fields, fields_kAlbum or fields_kTrack
	// lParam: not used
	// return: BYTE, result

	MSG_SETTRACKRANGE,
	// wParam: MAKEWORD(trackFirst, trackLast)
	// lParam: not used
	// return: if successful true, if failed false.

	MSG_LANGUAGECHANGED,
	// wParam: MAKEWPARAM(encoding, language)
	//         If encoding is -1, encoding should be ignored.
	//         If language is -1, language should be ignored.
	// lParam: not used
	// return: none / ignored

	MSG_GOTONEXT,
	// TrackPanel sends this message to parent to ask for editing next item
	// AlbumPanel sends this message to parent to ask to edit the first track
	// wParam: enum goto_k*
	// lParam: not used
	// return: bool, true if moved to the next or prev item according to wParam, false if there is no next (or prev) item to move to

	MSG_FOCUSNEXTCTL,
	// A Dialog or control container window send this message to its parent to let the focus leave itself,
	//         and move to the next control, which is in another dialog or container window.
	// wParam: int, id of dialog or container window
	// lParam: bool, if false, focus next, if true, focus prev
	// return: none / ignored

	MSG_COMPLETENESSCHANGED,
	// Album dialog and track dialog send tihs message to their parent to notify the form completeness status has changed
	// This message will be rounted to the nav control. Nav should call cdt_blockGetCompleteness() and use the return value to
	//         redraw itself to reflect the new completeness status.
	// wParam: bool. If true, nav should check all blocks for new status. If false, only check the current block.
	//         When track range is changed, this is true.
	// lParam: Not used
	// return: none / ignored

	MSG_DATACHANGED,
	// wParam: bool, show all fields
	// lParam: not used
	// return: bool, if the receiver successfully updated its state, returns true, otherwise false

	MSG_GETLAYOUTHINT,
	// Sent by a child to its parent to query the hint to layout.
	// wParam: WORD, id of source control
	// lParam: WORD array, the first element is the count of elements, including the first
	// return: HWND, who provided the hint, can be used to call MapWindowPoints()

	MSG_LAYOUTCHANGED,
	// Sent by a child to its parent to notify the child's layout has changed.
	// The parent may route this memasge to an other child, that other child can send MSG_GETLAYOUTHINT to layout itself accordingly
	// wParam: WORD, srcId
	// lParam: not used
	// return: none / ignored

	MSG_COPY,
	// wParam: enum format_k*, see format.h
	// lParam: not used
	// return: none / ignored

	//MSG_SHOWRECT,
	// A child send this message to its parent to ask to show this child
	// wParam: HWND of child
	// lParam: not used
	// return none / ignored
};


// void onCompletenessChanged(HWND wnd, bool allBlocks)
#define MSG_HANDLE_COMPLETENESSCHANGED(wnd, fn) \
	case MSG_COMPLETENESSCHANGED: return (fn)((wnd), (bool)wParam), 0
#define MSG_FORWARD_COMPLETENESSCHANGED(wnd) \
	case MSG_COMPLETENESSCHANGED: return SendMessage((wnd), msg, wParam, lParam)
static inline void
msg_sendCompletenessChanged(HWND wnd, bool allBlocks) {
	SendMessage(wnd, MSG_COMPLETENESSCHANGED, (WPARAM)allBlocks, 0);
}


// void onGetNav(HWND wnd)
#define MSG_HANDLE_GETNAV(wnd, fn) \
	case MSG_GETNAV: return (fn)((wnd))
#define MSG_FORWARD_GETNAV(wnd) \
	case MSG_GETNAV: return SendMessage((wnd), msg, wParam, lParam)
static inline int
msg_sendGetNav(HWND wnd) {
	return (int)SendMessage(wnd, MSG_GETNAV, 0, 0);
}


// void onNav(HWND wnd, int index)
#define MSG_HANDLE_NAV(wnd, fn) \
	case MSG_NAV: return (fn)((wnd), (int)wParam), 0
#define MSG_FORWARD_NAV(wnd) \
	case MSG_NAV: return SendMessage((wnd), msg, wParam, lParam)
static inline void
msg_sendNav(HWND wnd, int index) {
	SendMessage(wnd, MSG_NAV, index, 0);
}

// void onCommand(HWND wnd, int command)
#define MSG_HANDLE_COMMAND(wnd, fn) \
	case MSG_COMMAND: return (fn)((wnd), (int)wParam, lParam), 0
#define MSG_FORWARD_COMMAND(wnd) \
	case MSG_COMMAND: return SendMessage((wnd), msg, wParam, lParam)
static inline void
msg_sendCommand(HWND wnd, int cmd, LPARAM lp) {
	SendMessage(wnd, MSG_COMMAND, cmd, lp);
}


// bool onSetFields(HWND wnd, int type, BYTE fields)

#define MSG_HANDLE_SETFIELDS(wnd, fn) \
	case MSG_SETFIELDS: return (fn)((wnd), (int)wParam, (BYTE)lParam)

#define MSG_FORWARD_SETFIELDS(wnd) \
	case MSG_SETFIELDS: return SendMessage((wnd), msg, wParam, lParam)

static inline bool
msg_sendSetFields(HWND wnd, int fieldsClass, BYTE fields) {
	return SendMessage(wnd, MSG_SETFIELDS, (WPARAM)fieldsClass, (LPARAM)fields);
}


// BYTE onGetFields(HWND wnd, int fieldsClass)

#define MSG_HANDLE_GETFIELDS(wnd, fn) \
	case MSG_GETFIELDS: return (fn)((wnd), (int)wParam)

#define MSG_FORWARD_GETFIELDS(wnd) \
	case MSG_GETFIELDS: return SendMessage((wnd), MSG_GETFIELDS, wParam, lParam)

static inline BYTE
msg_sendGetFields(HWND wnd, int fieldsClass) {
	return (BYTE)SendMessage(wnd, MSG_GETFIELDS, (WPARAM)fieldsClass, 0);
}


// BYTE onGetFilledFields(HWND wnd, int fieldsClass)

static inline BYTE
msg_sendGetFilledFields(HWND wnd, int fieldsClass) {
	return (BYTE)SendMessage(wnd, MSG_GETFILLEDFIELDS, (WPARAM)fieldsClass, 0);
}


// void onSetTrackRange(int first, int last)
#define MSG_THREAD_HANDLE_SETTRACKRANGE(fn) \
	case MSG_SETTRACKRANGE: (fn)((int)(LOBYTE(msg.wParam)), (int)(HIBYTE(msg.wParam))); break
// bool onSetTrackRange(HWND wnd, int first, int last)
#define MSG_HANDLE_SETTRACKRANGE(wnd, fn) \
	case MSG_SETTRACKRANGE: return (fn)((wnd), (int)(LOBYTE(wParam)), (int)(HIBYTE(wParam)))
#define MSG_FORWARD_SETTRACKRANGE(wnd) \
	case MSG_SETTRACKRANGE: return SendMessage((wnd), MSG_SETTRACKRANGE, wParam, lParam)
static inline bool
msg_postThreadSetTrackRange(DWORD id, int first, int last) {
	return PostThreadMessage(id, MSG_SETTRACKRANGE, (WPARAM)MAKEWORD((BYTE)first, (BYTE)last), 0);
}
static inline bool
msg_sendSetTrackRange(HWND wnd, int first, int last) {
	return (bool)SendMessage(wnd, MSG_SETTRACKRANGE, (WPARAM)MAKEWORD((BYTE)first, (BYTE)last), 0);
}

// void onLanguageChanged(int encoding, int language)
#define MSG_HANDLE_LANGUAGECHANGED(wnd, fn) \
	case MSG_LANGUAGECHANGED: return (fn)((wnd)), 0
//#define MSG_HANDLE_SETLANGUAGE(wnd, fn) \
//	case MSG_SETLANGUAGE: return (fn)((wnd), LOWORD(wParam), HIWORD(wParam)), 0
//#define MSG_FORWARD_SETLANGUAGE(wnd) \
//	case MSG_SETLANGUAGE: return SendMessage(wnd, msg, wParam, lParam)
static inline void
msg_sendLanguageChanged(HWND wnd) {
	SendMessage(wnd, MSG_LANGUAGECHANGED, 0, 0);
}

// bool onGotoNext(HWND wnd, int which)
#define MSG_HANDLE_GOTONEXT(wnd, fn) \
	case MSG_GOTONEXT: return (fn)((wnd), (int)wParam)
#define MSG_FORWARD_GOTONEXT(wnd) \
	case MSG_GOTONEXT: return SendMessage((wnd), MSG_GOTONEXT, wParam, lParam)

static inline bool
sendGotoNext(HWND wnd, int which) {
	return (bool)SendMessage(wnd, MSG_GOTONEXT, (WPARAM)which, 0);
}

// void focusNextCtl(HWND wnd, int id, bool prev)
#define MSG_HANDLE_FOCUSNEXTCTL(wnd, fn) \
	case MSG_FOCUSNEXTCTL: return (fn)((wnd), (int)wParam, (bool)lParam), 0
#define MSG_FORWARD_FOCUSNEXTCTL(wnd) \
	case MSG_FOCUSNEXTCTL: return SendMessage((wnd), MSG_FOCUSNEXTCTL, wParam, lParam)

// Assume focus is leaving container window with dlgId, find the next (or prev) ctl and set focus
static inline void
msg_sendFocusNextCtl(HWND wnd, int idCurrentParent, bool prev) {
	SendMessage(wnd, MSG_FOCUSNEXTCTL, (WPARAM)idCurrentParent, (LPARAM)prev);
}


// bool onDataChanged(HWND wnd, bool showAllFields)

#define MSG_HANDLE_DATACHANGED(wnd, fn) \
	case MSG_DATACHANGED: return (fn)((wnd), (bool)wParam)

static inline bool
msg_sendDataChanged(HWND wnd, bool showAllFields) {
	return (bool)SendMessage(wnd, MSG_DATACHANGED, (WPARAM)showAllFields, 0);
}


// HWND onGetLayoutHint(HWND wnd, WORD srcId, WORD* data)
#define MSG_HANDLE_GETLAYOUTHINT(wnd, fn) \
	case MSG_GETLAYOUTHINT: return (fn)((wnd), (WORD)wParam, (WORD*)lParam)
#define MSG_FORWARD_GETLAYOUTHINT(wnd) \
	case MSG_GETLAYOUTHINT: return SendMessage((wnd), msg, wParam, lParam)
static inline HWND
msg_sendGetLayoutHint(HWND wnd, WORD srcId, WORD* data) {
	return (HWND)SendMessage(wnd, MSG_GETLAYOUTHINT, (WPARAM)srcId, (LPARAM)data);
}

// void onLayoutChanged(HWND wnd, WORD srcId)
#define MSG_HANDLE_LAYOUTCHANGED(wnd, fn) \
	case MSG_LAYOUTCHANGED: return (fn)((wnd), (WPARAM)srcId), 0
#define MSG_FORWARD_LAYOUTCHANGED(wnd) \
	case MSG_LAYOUTCHANGED: return SendMessage((wnd), msg, wParam, lParam);
static inline void
msg_sendLayoutChanged(HWND wnd, WORD srcId) {
	SendMessage(wnd, MSG_LAYOUTCHANGED, (WPARAM)srcId, 0);
}
static inline void
msg_postLayoutChanged(HWND wnd, WORD srcId) {
	PostMessage(wnd, MSG_LAYOUTCHANGED, (WPARAM)srcId, 0);
}

// void onCopy(HWND wnd, int format, int blockIndex)
#define MSG_HANDLE_COPY(wnd, fn) \
	case MSG_COPY: return (fn)((wnd), (int)wParam, (int)lParam), 0
static inline void
msg_sendCopy(HWND wnd, int format, int blockIndex) {
	SendMessage(wnd, MSG_COPY, (WPARAM)format, (LPARAM)blockIndex);
}

// void onShowRect(HWND wnd, HWND child)
//#define MSG_HANDLE_SHOWRECT(wnd, fn) \
//	case MSG_SHOWRECT: return (fn)((wnd), (HWND)wParam), 0
//static inline void
//msg_sendShowRect(HWND wnd, HWND child) {
//	SendMessage(wnd, MSG_SHOWRECT, (WPARAM)child, 0);
//}

//// void onFormStatusChanged(HWND wnd, int index, int status, int encoding, int language)
//#define MSG_HANDLE_FORMSTATUSCHANGED(wnd, fn) \
//	case MSG_FORMSTATUSCHANGED: return (fn)((wnd), LOWORD(wParam), HIWORD(wParam), LOWORD(lParam), HIWORD(lParam)), 0
//#define MSG_FORWARD_FORMSTATUSCHANGED(wnd) \
//	case MSG_FORMSTATUSCHANGED: return SendMessage((wnd), msg, wParam, lParam)
//
//static inline bool
//msg_postThreadFormStatusChanged(DWORD id, int index, int status, int encoding, int language) {
//	return PostThreadMessage(id, MSG_FORMSTATUSCHANGED, MAKEWPARAM(index, status), MAKELPARAM(encoding, language));
//}
//
//static inline void
//msg_sendFormStatusChanged(HWND wnd, int index, int status, int encoding, int language) {
//	SendMessage(wnd, MSG_FORMSTATUSCHANGED, MAKEWPARAM(index, status), MAKELPARAM(encoding, language));
//}


// void onDpiChanged(HWND wnd, int dpi, const RECT* rc)
#define MSG_HANDLE_WM_DPICHANGED(wnd, fn) \
	case WM_DPICHANGED: return (fn)((wnd), LOWORD(wParam), (RECT*)lParam), 0

// void onDpiChangedAfterParent(HWND wnd)
#define MSG_HANDLE_WM_DPICHANGED_AFTERPARENT(wnd, fn) \
	case WM_DPICHANGED_AFTERPARENT: return (fn)((wnd)), 0


// void onMouseLeave(HWND wnd)
#define MSG_HANDLE_WM_MOUSELEAVE(wnd, fn) \
	case WM_MOUSELEAVE: return (fn)((wnd)), 0

// LRESULT onNotify(HWND wnd, int id, LPNMHDR hdr)
#define MSG_HANDLE_WM_NOTIFY(wnd, fn) \
	case WM_NOTIFY: return (fn)((wnd), (int)wParam, (LPNMHDR)lParam)

// void onMouseWheel(HWND wnd, int keys, int delta, int x, int y)
#define MSG_HANDLE_WM_MOUSEWHEEL(wnd, fn) \
	case WM_MOUSEWHEEL: return (fn)((wnd), GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), 0

// void onPrintClient(HWND wnd, HDC dc, UINT opt)
#define MSG_HANDLE_WM_PRINTCLIENT(wnd, fn) \
	case WM_PRINTCLIENT: return (fn)((wnd), (HDC)wParam, (UINT)lParam), 0

// void onThemeChanged(HWND wnd)
#define MSG_HANDLE_WM_THEMECHANGED(wnd, fn) \
	case WM_THEMECHANGED: return (fn)((wnd)), 0

// void onUpdateUiState(HWND wnd, WORD action, WORD flags)
#define MSG_HANDLE_WM_UPDATEUISTATE(wnd, fn) \
	case WM_UPDATEUISTATE: return (fn)((wnd), LOWORD(wParam), HIWORD(wParam)), 0

