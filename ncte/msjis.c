#include "charset.h"

#include <Windows.h>

#include <stdlib.h> // _countof
#include <search.h> // bsearch
#include <intrin.h> // _byteswap_ushort

#include "resdefs.h"


typedef struct WCharPair {
	wchar_t src;
	wchar_t dst;
}WCharPair;

typedef int(__cdecl* CompareFunc)(const void*, const void*);

static int __cdecl
compareWchar(const WCharPair* a, const WCharPair* b) {
	return a->src - b->src;
}

static inline WCharPair*
loadWCharMap(WORD id, DWORD* count) {
	HRSRC res = FindResource(NULL, MAKEINTRESOURCE(id), RT_RCDATA);
	if (!res) return NULL;
	*count = SizeofResource(NULL, res) / sizeof(WCharPair);
	HGLOBAL h = LoadResource(NULL, res);
	if (!h) return NULL;
	return LockResource(h);
}

int
charset_regulateMsjis(wchar_t* text, int len) {
	static const wchar_t kDefaultChar = 0x9981;
	static const WCharPair* kMap = NULL;
	static DWORD count = 0;
	if (!kMap) {
		kMap = loadWCharMap(kRcMsjisSubstitutes, &count);
		if (!kMap) return 0;
	}

	int result = 0;
	if (len < 0) len = lstrlen(text);
	const wchar_t* end = text + len;
	for (wchar_t* p = text; p < end; ++p) {
		WCharPair* pair = bsearch(p, kMap, count, sizeof(WCharPair), (CompareFunc)compareWchar);
		if (pair) {
			++result;
			*p = pair->dst;
		}
	}
	return result;
}

bool
charset_utf16ToMsjis(const wchar_t* src, int len, wchar_t* dst) {
	static const WCharPair* map = NULL;
	static DWORD count = 0;
	if (!map) {
		map = loadWCharMap(kRcUtf16ToMsjis, &count);
		if (!map) return false;
	}

	const wchar_t* end = src + len;
	for (; src < end; ++src, ++dst) {
		WCharPair* pair = bsearch(src, map, count, sizeof(WCharPair), (CompareFunc)compareWchar);
		if (pair) {
			*dst = _byteswap_ushort(pair->dst);
		}
		else {
			*dst = L'\0';
			return false;
		}
	}
	*dst = L'\0';
	return true;
}

bool
charset_MsjisToUtf16(const wchar_t* src, int len, wchar_t* dst) {
	static const WCharPair* map = NULL;
	static DWORD count = 0;
	if (!map) {
		map = loadWCharMap(kRcMsjisToUtf16, &count);
		if (!map) return false;
	}

	for (; src < src + len; ++src, ++dst) {
		wchar_t ch = _byteswap_ushort(*src);
		WCharPair* pair = bsearch(&ch, map, count, sizeof(WCharPair), (CompareFunc)compareWchar);
		if (pair) {
			*dst = pair->dst;
		}
		else {
			*dst = L'\0';
			return false;
		}
	}
	*dst = L'\0';
	return true;
}
