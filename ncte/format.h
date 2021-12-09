#pragma once

#include <Windows.h>

#include <stdbool.h>
#include <assert.h>

#include "cdtcomm.h"


enum {
	format_kUnknown,
	format_kIndex,
	format_kSheet,
	format_kEac,
	format_kCue,
};


static inline bool
format_isIndex(const wchar_t* text) {
	if (!*text) return false;
	if (text[1]) return false;
	int index = *text - L'0';
	if (index >= 0 && index < cdt_kBlockCountMax) return true;
	return false;
}

// Make sure format is format_kIndex before call this function.
static inline int
format_getIndex(const wchar_t* text) {
	return *text - L'0';
}


static inline bool
sheet_isMyKind(const wchar_t* text) {
	return false;
}

wchar_t*
eac_blockToText(const cdt_Block* block, const cdt_Time* toc, size_t* len);

//bool
//eac_isMyKind(const wchar_t* text);


//bool
//eac_parse(Cdt* cdt, const wchar_t* text);


static inline int
format_getFormat(const wchar_t* text) {
	if (!text) return format_kUnknown;
	if (format_isIndex(text)) return format_kIndex;
	if (sheet_isMyKind(text)) return format_kSheet;
	//if (eac_isMyKind(text)) return format_kEac;
	return format_kUnknown;
}

typedef wchar_t*(*BlockToTextFunc)(const cdt_Block* block, const cdt_Time* toc, size_t* len);

bool
format_copy(HWND wnd, const cdt_Block* block, const cdt_Time* toc, BlockToTextFunc blockToTextFunc);
