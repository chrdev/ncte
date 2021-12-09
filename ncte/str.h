#pragma once

#include <Windows.h>
#include <strsafe.h>

#include <stdarg.h>
#include <string.h> // strncmp
#include <stdbool.h>

#include "mem.h"


typedef struct ResStr{
	const wchar_t* p;
	int c;
}ResStr;

const wchar_t* _cdecl
wcs_format(const wchar_t* format, ...);

const wchar_t* _cdecl
wcs_formatEx(int* len, const wchar_t* format, ...);

// Caution: return static pointer
// len can be NULL if not needed
const wchar_t* _cdecl
wcs_loadf(int* len, WORD id, ...);

// Caution: return static pointer
// len can be NULL if not needed
const wchar_t*
wcs_load(int* len, WORD id);

ResStr
resstr_load(WORD id);

// Return converted positive number or 0
int
resstr_loadNumber(WORD id);

// Because we are dealing whih short lists, a dumb comparison will do
static inline int
str_getIndexInList(const char* str, int strLen, const char* const* list, int count) {
	for (int i = 0; i < count; ++i) {
		if (!_strnicmp(str, list[i], strLen)) return i;
	}
	return -1;
}

static inline int
wcs_getIndexInList(const wchar_t* str, int strLen, const wchar_t* const* list, int count) {
	for (int i = 0; i < count; ++i) {
		if (!_wcsnicmp(str, list[i], strLen)) return i;
	}
	return -1;
}

// Returns successfully converted digits, or -1 on failure
static inline int
str_toNumberAsDec(int* result, const char* str, int limit) {
	enum { kHardLimit = 8 };
	if (limit > kHardLimit) limit = kHardLimit;
	int n = 0;
	int i = 0;
	for (; str[i] && i < limit; ++i) {
		if (str[i] == ' ' || str[i] == '\t' || str[i] == '\r' || str[i] == '\n') break;
		if (str[i] < '0' || str[i] > '9') return -1;
		n = 10 * n + str[i] - '0';
	}
	*result = n;
	return i;
}

// Returns successfully converted digits, or -1 on failure
static inline int
wcs_toNumberAsDec(int* result, const wchar_t* str, int limit) {
	enum { kHardLimit = 8 };
	if (limit > kHardLimit) limit = kHardLimit;
	int n = 0;
	int i = 0;
	for (; str[i] && i < limit; ++i) {
		if (str[i] == L' ' || str[i] == L'\t' || str[i] == L'\r' || str[i] == L'\n') break;
		if (str[i] < L'0' || str[i] > L'9') return -1;
		n = 10 * n + str[i] - L'0';
	}
	*result = n;
	return i;
}

// Returns successfully converted digits, or -1 on failure
// This function convert hex number like 0xFF or 0Xff or ffh or ffH.
static inline int
str_toNumberAsHex(int* result, const char* str, int limit) {
	enum { kHardLimit = 8 }; // We don't need to convert large dex in this app, so 8 is enough
	if (limit > kHardLimit) limit = kHardLimit;
	int n = 0;
	int i = 0;
	bool leading = true;
	for (; str[i] && i < limit; ++i) {
		char ch = str[i];
		if (ch == 'h' || ch == 'H' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') break;
		if (leading) {
			if (ch == '0') continue;
			if (ch == 'x' || ch == 'X') {
				leading = false;
				continue;
			}
		}

		if (ch >= 'A' && ch <= 'F') {
			ch += 0x20;
		}
		if (ch >= 'a' && ch <= 'f') {
			leading = false;
			n = (n << 4) | (ch - 'a' + 10);
		}
		else if (ch >= '0' && ch <= '9') {
			leading = false;
			n = (n << 4) | (ch - '0');
		}
		else {
			return -1;
		}
	}
	*result = n;
	return i;
}

// Returns successfully converted digits, or -1 on failure
// This function convert hex number like 0xFF or 0Xff or ffh or ffH.
static inline int
wcs_toNumberAsHex(int* result, const wchar_t* str, int limit) {
	enum { kHardLimit = 8 }; // We don't need to convert large dex in this app, so 8 is enough
	if (limit > kHardLimit) limit = kHardLimit;
	int n = 0;
	int i = 0;
	bool leading = true;
	for (; str[i] && i < limit; ++i) {
		wchar_t ch = str[i];
		if (ch == L'h' || ch == L'H' || ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') break;
		if (leading) {
			if (ch == L'0') continue;
			if (ch == L'x' || ch == L'X') {
				leading = false;
				continue;
			}
		}

		if (ch >= L'A' && ch <= L'F') {
			ch += 0x20;
		}
		if (ch >= L'a' && ch <= L'f') {
			leading = false;
			n = (n << 4) | (ch - L'a' + 10);
		}
		else if (ch >= L'0' && ch <= L'9') {
			leading = false;
			n = (n << 4) | (ch - L'0');
		}
		else {
			return -1;
		}
	}
	*result = n;
	return i;
}

static inline bool
str_isEmpty(const char* text) {
	return (!text || text[0] == '\0');
}

static inline bool
wcs_isEmpty(const wchar_t* text) {
	return (!text || text[0] == L'\0');
}

// Use HeapFree(GetProcessHeap(),...) to free the result
// len can be NULL
static inline wchar_t*
wcs_duplicate(const wchar_t* text, int* len) {
	int len_ = lstrlen(text);
	if (len) *len = len_;
	size_t cb = ((size_t)len_ + 1) * sizeof(wchar_t);
	wchar_t* result = HeapAlloc(GetProcessHeap(), 0, cb);
	if (!result) return NULL;
	CopyMemory(result, text, cb);
	return result;
}

static inline bool
wcs_copy(HANDLE heap, wchar_t** dst, const wchar_t* src) {
	assert(!*dst);
	if (wcs_isEmpty(src)) return true;
	size_t cb = (lstrlen(src) + 1) * sizeof(wchar_t);
	if (!mem_reserve(heap, dst, cb)) return false;
	CopyMemory(*dst, src, cb);
	return true;
}
