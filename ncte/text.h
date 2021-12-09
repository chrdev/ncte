#pragma once

#include <wchar.h>
#include <stdbool.h>
#include <stdint.h>


bool
text_append(wchar_t** buffer, size_t pos, const wchar_t* text, size_t len);

// If the character after the new line is not NUL, return pointer to the next line.
// If no new line, or the first character at next line is NUL, return NULL.
static inline const wchar_t*
text_advanceLine(const wchar_t* text) {
	const wchar_t* p = text;
	for (; *p; ++p) {
		switch (*p) {
		case L'\r':
			p += (*(p + 1) == L'\n') ? 2 : 1;
			return *p ? p : NULL;
		case L'\n':
			++p;
			return *p ? p : NULL;
		}
	}
	return NULL;
}

static inline const wchar_t*
text_advanceLines(const wchar_t* text, size_t count) {
	const wchar_t* p = text;
	for (size_t i = 0; i < count && p; ++i) {
		p = text_advanceLine(p);
	}
	return p;
}

// convert the first two characters to number
// If both characters are digits, return 0 to 99
// If either of the first two characters is not digit, return -1
int
text_parseUnsignedDigits2(const wchar_t* text);

// Find the first character past whitespaces (' ' and '\t'), return its pointer.
// If met EOL or NUL before any non-whitespace characters, return NULL.
const wchar_t*
text_skipSpacesInLine(const wchar_t* text);

// Caution: characters in expects must be in ascending order, otherwise the result is undefined.
// Search text for characters in expects array, if found any one, return the found index, count set to offset to text.
// If no expects found at current line, return -1.
// count parameter is in and out.
int
text_expectCharsInLine(const wchar_t* text, const wchar_t* expected, int* count);


static inline ptrdiff_t
text_distanceToEol(const wchar_t* text) {
	for (const wchar_t* p = text; ; ++p) {
		switch (*p) {
		case L'\r':
		case L'\n':
		case L'\0':
			return p - text;
		}
	}
	return -1;
}

static inline ptrdiff_t
text_distanceToCharInLine(const wchar_t* text, wchar_t ch) {
	for (const wchar_t* p = text;; ++p) {
		if (*p == ch) return p - text;
		switch (*p) {
		case L'\r':
		case L'\n':
		case L'\0':
			return -1;
		}
	}
	return -1;
}

static inline ptrdiff_t
text_distanceToNonSpaceInLine(const wchar_t* text) {
	for (const wchar_t* p = text;; ++p) {
		switch (*p) {
		case L' ':
		case L'\t':
			continue;
		case L'\r':
		case L'\n':
		case L'\0':
			return -1;
		default:
			return p - text;
		}
	}
	return -1;
}

// Note: return distance to text, not to begin
static inline ptrdiff_t
text_distanceToPrevNonSpace(const wchar_t* text, const wchar_t* begin) {
	for (const wchar_t* p = begin; p != text; --p) {
		switch (*p) {
		case L' ':
		case L'\t':
			continue;
		default:
			return p - text;
		}
	}
	return -1;
}
