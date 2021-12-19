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

bool
wcsbuffer_append(wchar_t** buffer, const wchar_t* text, int len) {
	if (!text || !len) return true;
	size_t cchIndex = wcsbuffer_getCchIndex_(*buffer);
	size_t capacity = cchIndex - 1;
	size_t cch = (size_t)(*buffer)[cchIndex];
	if (len < 0) len = lstrlen(text);

	if (cch + (size_t)len > capacity) {
		size_t size = ((cch + (size_t)len) * 2 + 2) * sizeof(wchar_t);
		void* np = HeapReAlloc(GetProcessHeap(), 0, *buffer, size);
		if (!np) return false;
		*buffer = np;
		cchIndex = wcsbuffer_getCchIndex_(*buffer);
	}

	CopyMemory(*buffer + cch, text, (size_t)len * sizeof(wchar_t));
	cch += (size_t)len;
	(*buffer)[cch] = L'\0';
	(*buffer)[cchIndex] = (wchar_t)cch;
	return buffer;
}
