#pragma once

#include <wchar.h>
#include <stdbool.h>


#include "cdtcomm.h"


typedef int(*charset_RegulateFunc)(wchar_t* text, int len);

// Return count of illegal characters found and regulated.
// If all characters are legal, return 0.
int
charset_regulateAscii(wchar_t* text, int len);

// Return count of illegal characters found and regulated.
// If all characters are legal, return 0.
int
charset_regulate88591(wchar_t* text, int len);

// Return count of illegal characters found and regulated.
// If all characters are legal, return 0.
int
charset_regulateMsjis(wchar_t* text, int len);


static inline int
charset_regulate(unsigned char encoding, wchar_t* text, int len) {
	// as per index of cdt_kEncodings
	switch (encoding) {
	case cdt_kEncodingIndex88591:
		return charset_regulate88591(text, len);
	case cdt_kEncodingIndexAscii:
		return charset_regulateAscii(text, len);
	case cdt_kEncodingIndexMsjis:
		return charset_regulateMsjis(text, len);
	}
	return 0;
}

// Caution: sizeof dst is assumed to the at least as large as src.
bool
charset_utf16ToMsjis(const wchar_t* src, int len, wchar_t* dst);

// Caution: sizeof dst is assumed to the at least as large as src.
bool
charset_MsjisToUtf16(const wchar_t* src, int len, wchar_t* dst);
