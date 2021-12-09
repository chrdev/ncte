#include "wnd.h"


const wchar_t*
wnd_getText(HWND wnd, int* len) {
	static wchar_t buffer[MAX_PATH];
	int len_ = GetWindowText(wnd, buffer, ARRAYSIZE(buffer));
	if (!len_) buffer[0] = L'\0';
	if (len) *len = len_;
	return buffer;
}
