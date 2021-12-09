#include "text.h"

#include <Windows.h>


bool
text_append(wchar_t** buffer, size_t pos, const wchar_t* text, size_t len) {
	size_t size = HeapSize(GetProcessHeap(), 0, *buffer);
	size_t required = (pos + len + 1) * sizeof(wchar_t);
	if (size < required) {
		size = required * 2;
		wchar_t* p = HeapReAlloc(GetProcessHeap(), 0, *buffer, size);
		if (!p) return false;
		*buffer = p;
	}
	CopyMemory(*buffer + pos, text, len * sizeof(wchar_t));
	(*buffer)[pos + len] = L'\0';
	return true;
}

//const wchar_t*
//text_skipLines(const wchar_t* text, size_t count) {
//	if (!count) return text;
//	size_t lineCount = 0;
//	const wchar_t* p = text;
//	for (; *p; ++p) {
//		switch (*p) {
//		case L'\r':
//			if (*(p + 1) == L'\n') continue;
//			// fall-through
//		case L'\n':
//			if (++lineCount == count) return *(p + 1) ? p + 1 : NULL;
//			break;
//		}
//	}
//	return NULL;
//}

int
text_parseUnsignedDigits2(const wchar_t* text) {
	if (!text) return -1;
	int result = 0;
	int digit = *text++ - L'0';
	if (digit >= 0 && digit <= 9) result = digit;
	else return -1;
	digit = *text - L'0';
	if (digit >= 0 && digit <= 9) return result * 10 + digit;
	else return -1;
}

const wchar_t*
text_skipSpacesInLine(const wchar_t* text) {
	for (; ; ++text) {
		switch (*text) {
		case L' ':
		case L'\t':
			continue;
		case L'\r':
		case L'\n':
		case L'\0':
			return NULL;
		default:
			return text;
		}
	}
	return NULL;
}

int
text_expectCharsInLine(const wchar_t* text, const wchar_t* expects, int* count) {
	return -1;
}
