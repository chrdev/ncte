#include "clipboard.h"

#include "cdtcomm.h" // cdt_kBlockCountMax


bool
clipboard_putText(HWND wnd, const wchar_t* text, size_t len) {
	size_t cb = (len + 1) * sizeof(wchar_t);
	HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, cb);
	if (!h) return false;
	wchar_t* buffer = GlobalLock(h);
	if (!buffer) {
		GlobalFree(h);
		return false;
	}
	CopyMemory(buffer, text, cb - sizeof(wchar_t));
	buffer[len] = L'\0';
	GlobalUnlock(h);

	if (!OpenClipboard(wnd)) {
		GlobalFree(h);
		return false;
	}
	bool result = false;
	if (!EmptyClipboard()) {
		GlobalFree(h);
		goto cleanup;
	}
	if (!SetClipboardData(CF_UNICODETEXT, h)) {
		GlobalFree(h);
		goto cleanup;
	}

	result = true;
cleanup:;
	CloseClipboard();
	return result;
}

bool
clipboard_putIndex(HWND wnd, int index) {
	if (index < 0) index = 0;
	if (index >= cdt_kBlockCountMax) index = cdt_kBlockCountMax - 1;
	wchar_t text = index + L'0';
	return clipboard_putText(wnd, &text, 1);
}
