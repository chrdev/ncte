#pragma once

#include <Windows.h>
#include <shellapi.h>

#include <stdbool.h>
#include <assert.h>

#include "str.h"


static inline bool
clipboard_open(HWND wnd) {
	return OpenClipboard(wnd);
}

static inline bool
clipboard_close(void) {
	return CloseClipboard();
}

bool
clipboard_putText(HWND wnd, const wchar_t* text, size_t len);

// index will be regulated to its legal range before sent to clipboard
bool
clipboard_putIndex(HWND wnd, int index);

// Clipboard must be opend before call this function.
// len can be NULL
// If there is text, return the text, caller must call HeapFree(GetProcessHeap()...) after use.
// If there is no text, return NULL.
static inline wchar_t*
clipboard_getText(int* len) {
	HGLOBAL h = GetClipboardData(CF_UNICODETEXT);
	if (!h) return NULL;
	const wchar_t* data = GlobalLock(h);
	if (!data) return NULL;
	wchar_t* text = wcs_duplicate(data, len);
	GlobalUnlock(h);
	return text;
}

static inline HDROP
clipboard_getDrop(void) {
	return (HDROP)GetClipboardData(CF_HDROP);
}
