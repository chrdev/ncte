#include "format.h"

#include "text.h"
#include "cdt.h"


// If any text generated from the block, return text. Caller should call HeapFree(GetProcessHeap(),...) after use.
// If failed, return NULL
// len can be NULL if not needed
wchar_t*
eac_blockToText(const cdt_Block* block, const cdt_Time* toc, size_t* len) {
	enum { kInitSize = 2 };
	wchar_t* buffer = HeapAlloc(GetProcessHeap(), 0, kInitSize);
	if (!buffer) return NULL;

	size_t pos = 0;

	for (int i = 0; i < block->trackCount; ++i) {
		const wchar_t* t;
		size_t len_;

		t = block->tracks[i].artist;
		len_ = (size_t)lstrlen(t);
		if (len_) {
			if (!text_append(&buffer, pos, t, len_)) goto leave;
			pos += len_;

			t = L" / ";
			if (!text_append(&buffer, pos, t, 3)) goto leave;
			pos += 3;
		}

		t = block->tracks[i].title;
		len_ = (size_t)lstrlen(t);
		if (len_) {
			if (!text_append(&buffer, pos, t, len_)) goto leave;
			pos += len_;
		}

		t = L"\r\n";
		if (!text_append(&buffer, pos, t, 2)) goto leave;
		pos += 2;
	}
leave:;
	if (len) *len = pos;
	return buffer;
}

//bool
//eac_isMyKind(const wchar_t* text) {
//	// EAC V1.6
//	// If the first line contains ' - ', see the text as EAC clipboard data
//	for (const wchar_t* p = text; *p && *p != L'\r' && *p != L'\n'; ++p) {
//		if (*p == L'-' && p > text && *(p - 1) == L' ' && *(p + 1) == L' ') return true;
//	}
//	return false;
//}
//
//static bool
//parseTrackRange(const wchar_t* text, int* first, int* last) {
//	const wchar_t* p = text_advanceLines(text, 2);
//	if (!p) return false;
//	*first = text_parseUnsignedDigits2(p);
//	if (*first <= 0) return false;
//
//	p = text_advanceLine(p);
//	int prev = *first;
//	for (int last_ = text_parseUnsignedDigits2(p); last_ > prev; last_ = text_parseUnsignedDigits2(p)) {
//		prev = last_;
//		p = text_advanceLine(p);
//	}
//	*last = prev;
//	return true;
//}
//
//static bool
//parseAlbum(cdt_Block* block, const wchar_t* text) {
//	// line format:
//	// 'album artist - album title\r\n'
//	// If empty ' - \r\n'
//
//	ptrdiff_t sepPos = text_distanceToCharInLine(text, L'-');
//	if (sepPos <= 0) return false;
//
//	ptrdiff_t artistLen = text_distanceToPrevNonSpace(text, text + sepPos - 1) + 1;
//	if (artistLen > 0) {
//		if (!cdt_setText(block, &block->album->artist, text, (int)artistLen)) return false;
//	}
//
//	text += sepPos + 2;
//	ptrdiff_t titleBegin = text_distanceToNonSpaceInLine(text);
//	if (titleBegin >= 0) {
//		ptrdiff_t titleEnd = text_distanceToEol(text + titleBegin);
//		if (!cdt_setText(block, &block->album->title, text + titleBegin, (int)(titleEnd - titleBegin))) return false;
//	}
//	return true;
//}
//
//static bool
//parseTrack(cdt_Block* block, cdt_Time* toc, int index, const wchar_t* text) {
//
//
//	return true;
//}
//
//bool
//eac_parse(Cdt* cdt, const wchar_t* text) {
//	int first, last;
//	bool ok = parseTrackRange(text, &first, &last);
//	if (!ok) return false;
//	if (!cdt_setTrackRange(cdt, first, last)) return false;
//
//	cdt_Block* block = cdt->blocks;
//	const wchar_t* p =  text;
//	if (!parseAlbum(block, p)) return false;
//	p = text_advanceLines(p, 2);
//
//	for (int i = 0; i < block->trackCount; ++i) {
//		if (!parseTrack(block, cdt->toc, i, p)) return false;
//		p = text_advanceLine(p);
//	}
//	//cdt->toc[0] = 0;// some
//	return true;
//}
