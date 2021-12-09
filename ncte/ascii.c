#include "charset.h"

#include <Windows.h>


int
charset_regulateAscii(wchar_t* text, int len) {
	static const wchar_t kDefaultChar = 0x7E;
	int result = 0;
	if (len < 0) len = lstrlen(text);
	const wchar_t* end = text + len;
	for (wchar_t* p = text; p < end; ++p) {
		if (*p < 0x20 || *p > 0x7E) {
			*p = kDefaultChar;
			++result;
		}
	}
	return result;
}
