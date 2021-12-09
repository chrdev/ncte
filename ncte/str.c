#include "str.h"

static wchar_t theBuffer_[100];
static wchar_t theResBuffer_[100];

const wchar_t* _cdecl
wcs_format(const wchar_t* format, ...) {
	va_list ap;
	va_start(ap, format);
	StringCchVPrintfEx(theBuffer_, ARRAYSIZE(theBuffer_), NULL, NULL, STRSAFE_IGNORE_NULLS, format, ap);
	va_end(ap);
	return theBuffer_;
}

const wchar_t* _cdecl
wcs_formatEx(int* len, const wchar_t* format, ...) {
	size_t cch;
	va_list ap;
	va_start(ap, format);
	StringCchVPrintfEx(theBuffer_, ARRAYSIZE(theBuffer_), NULL, &cch, STRSAFE_IGNORE_NULLS, format, ap);
	va_end(ap);
	if (len) *len = (int)(ARRAYSIZE(theBuffer_) - cch);
	return theBuffer_;
}

// len can be NULL
const wchar_t* _cdecl
wcs_loadf(int* len, WORD id, ...) {
	int resLen = LoadString(GetModuleHandle(NULL), id, theResBuffer_, ARRAYSIZE(theResBuffer_));
	if (!resLen) return NULL;
	va_list ap;
	va_start(ap, id);
	size_t remaining;
	StringCchVPrintfEx(theBuffer_, ARRAYSIZE(theBuffer_), NULL, &remaining, STRSAFE_IGNORE_NULLS, theResBuffer_, ap);
	va_end(ap);
	if (len) *len = (int)(ARRAYSIZE(theBuffer_) - remaining);
	return theBuffer_;
}

const wchar_t*
wcs_load(int* len, WORD id) {
	int resLen = LoadString(GetModuleHandle(NULL), id, theResBuffer_, ARRAYSIZE(theResBuffer_));
	if (len) *len = resLen;
	return resLen ? theResBuffer_ : NULL;
}

ResStr
resstr_load(WORD id) {
	ResStr str;
	str.c = LoadString(GetModuleHandle(NULL), id, (wchar_t*)&str.p, 0);
	if (!str.c)  str.p = NULL;
	return str;
}

// Return converted positive number, or -1 on failure
static inline int
resstr_toNumber(const ResStr* resstr) {
	enum { kHardLimit = 8 };
	int limit = resstr->c;
	if (limit > kHardLimit) limit = kHardLimit;
	int n = 0;
	const wchar_t* str = resstr->p;
	for (int i = 0; i < limit; ++i) {
		if (str[i] < L'0' || str[i] > L'9') return -1;
		n = 10 * n + str[i] - L'0';
	}
	return n;
}

int
resstr_loadNumber(WORD id) {
	ResStr str = resstr_load(id);
	if (!str.p) return 0;
	int result = resstr_toNumber(&str);
	return result > 0 ? result : 0;
}
