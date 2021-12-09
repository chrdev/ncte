#include "cdt.h"


#include <intrin.h> // _byteswap_ushort
#include <assert.h>

#include "cdtcomm.h"
#include "mem.h" // mem_reserve
#include "charset.h"
#include "wnd.h"
#include "fields.h"


Cdt theCdt;

bool
cdt_init(Cdt* cdt) {
	ZeroMemory(cdt, sizeof(Cdt));
	cdt->trackFirst = 1;
	cdt->trackLast = 1;

	size_t tocSize = 2 * sizeof(cdt_Time); // 1 for album, 1 for track0, total 2
	cdt->toc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, tocSize);
	if (!cdt->toc) return false;

	return cdt_reserveBlock(cdt, 0);
}

void
cdt_uninit(Cdt* cdt) {
	for (int i = 0; i < cdt_kBlockCountMax; ++i) {
		cdt_Block* block = cdt->blocks + i;
		if (!block->heap) continue;
		BOOL ok = HeapDestroy(block->heap);
		assert(ok);
	}
	HeapFree(GetProcessHeap(), 0, cdt->toc);
}

static inline bool
reserveTracks(cdt_Block* block, int count) {
	assert(block->tracks);
	HANDLE heap = block->heap;
	size_t size = sizeof(cdt_Track) * count;
	if (HeapSize(heap, 0, block->tracks) < size) {
		cdt_Track* tracks = HeapReAlloc(heap, HEAP_ZERO_MEMORY, block->tracks, size);
		if (!tracks) return false;
		block->tracks = tracks;
	}
	return true;
}

static inline bool
reserveToc(Cdt* cdt, int trackCount) {
	size_t tocSize = (trackCount + 1) * sizeof(cdt_Time);
	if (HeapSize(GetProcessHeap(), 0, cdt->toc) < tocSize) {
		cdt_Time* toc = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cdt->toc, tocSize);
		if (!toc) return false;
		cdt->toc = toc;
	}
	return true;
}

static inline bool
reserveBlocks(Cdt* cdt, int trackFirst, int trackCount) {
	for (int i = 0; i < cdt_kBlockCountMax; ++i) {
		cdt_Block* block = cdt->blocks + i;
		if (!block->heap) continue;
		if (!reserveTracks(block, trackCount)) return false;
	}
	for (int i = 0; i < cdt_kBlockCountMax; ++i) {
		cdt_Block* block = cdt->blocks + i;
		if (!block->heap) continue;
		block->trackFirst = trackFirst;
		block->trackCount = trackCount;
	}
	return true;
}

bool
cdt_setTrackRange(Cdt* cdt, int first, int last) {
	if (first <= 0 || first > last || last > 99) return false;
	if (cdt->trackFirst == first && cdt->trackLast == last) return true;

	int trackCount = last - first + 1;

	if (!reserveToc(cdt, trackCount)) return false;
	if (!reserveBlocks(cdt, first, trackCount)) return false;

	cdt->trackFirst = first;
	cdt->trackLast = last;
	return true;
}

static inline bool
blockInit(cdt_Block* block, int trackFirst, int trackCount) {
	assert(!block->heap);
	block->heap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
	if (!block->heap) {
		return false;
	}
	block->album = HeapAlloc(block->heap, HEAP_ZERO_MEMORY, sizeof(cdt_Album));
	if (!block->album) {
		HeapDestroy(block->heap), block->heap = NULL;
		return false;
	}
	block->tracks = HeapAlloc(block->heap, HEAP_ZERO_MEMORY, sizeof(cdt_Track) * trackCount);
	if (!block->tracks) {
		HeapDestroy(block->heap), block->heap = NULL;
		return false;
	}
	block->trackFirst = trackFirst;
	block->trackCount = trackCount;
	return true;
}

bool
cdt_reserveBlock(Cdt* cdt, int index) {
	if (index >= cdt_kBlockCountMax || index < 0) return false;
	if (cdt->blocks[index].heap) return true;
	cdt_Block* block = cdt->blocks + index;
	if (!blockInit(block, cdt->trackFirst, cdt_getTrackCount(cdt))) return false;
	return true;
}

bool
cdt_clearBlock(Cdt* cdt, int index) {
	cdt_Block* block = cdt->blocks + index;
	assert(block->heap);
	HeapDestroy(block->heap);
	block->heap = NULL;
	return blockInit(block, cdt->trackFirst, cdt_getTrackCount(cdt));
}

bool
cdt_setTextFromCtlUsingEncoding(cdt_Block* block, wchar_t** field, HWND ctl, int encodingIndex) {
	assert(block->heap);
	int cch = GetWindowTextLength(ctl);
	if (!cch) {
		if (*field) (*field)[0] = L'\0';
		return true;
	}

	int cb = (cch + 1) * sizeof(wchar_t);
	if (!mem_reserve(block->heap, field, cb)) return false;
	GetWindowText(ctl, *field, cch + 1);
	if (charset_regulate(encodingIndex, *field, cch)) {
		SetWindowText(ctl, *field);
	}
	return true;
}

static inline int
getWcsBufferCch(const char* text, int len, UINT codepage) {
	if (codepage == cdt_kCpUtf16be || codepage == cdt_kCpMsjis) {
		return (len + 1) / sizeof(wchar_t);
	}
	else {
		return MultiByteToWideChar(codepage, 0, text, len, NULL, 0);
	}
}

static int
transcode(wchar_t* buffer, int cch, const char* text, int len, UINT codepage) {
	if (codepage == cdt_kCpUtf16be) {
		assert(cch * sizeof(wchar_t) == len);
		wchar_t* end = buffer + cch;
		for (const wchar_t* src = (const wchar_t*)text; buffer < end;) {
			*buffer++ = _byteswap_ushort(*src++);
		}
		return cch;
	}
	else if (codepage == cdt_kCpMsjis) {
		assert(cch * sizeof(wchar_t) >= len);
		const wchar_t* src = (const wchar_t*)text;
		charset_MsjisToUtf16(src, cch, buffer);
		return cch;
	}
	else {
		return MultiByteToWideChar(codepage, 0, text, len, buffer, cch);
	}
}

bool
cdt_setTextUsingCodepage(cdt_Block* block, wchar_t** field, const char* text, int len, UINT codepage) {
	assert(block->heap);
	if (!len) {
		if (*field) (*field)[0] = L'\0';
		return true;
	}

	int cch = getWcsBufferCch(text, len, codepage);
	if (!cch) return false;
	int cb = (cch + 1) * sizeof(wchar_t);
	if (!mem_reserve(block->heap, field, cb)) return false;
	cch = transcode(*field, cch, text, len, codepage);
	(*field)[cch] = L'\0';
	return cch;
}

bool
cdt_setText(cdt_Block* block, wchar_t** field, const wchar_t* text, int len) {
	assert(block->heap);
	if (len <= 0) {
		if (*field) (*field)[0] = L'\0';
		return true;
	}

	int cb = (len + 1) * sizeof(wchar_t);
	if (!mem_reserve(block->heap, field, cb)) return false;
	CopyMemory(*field, text, cb - sizeof(wchar_t));
	(*field)[len] = L'\0';
	return true;
}

bool
cdt_blockIsEmpty(const cdt_Block* block) {
	if (!block->heap) return true;

	if (block->album->copyProtected) return false;
	if (block->album->encoding) return false;
	if (block->album->language) return false;
	if (block->album->genreCode) return false;
	if (!wcs_isEmpty(block->album->genreExtra)) return false;
	if (!wcs_isEmpty(block->album->catalog)) return false;
	if (!wcs_isEmpty(block->album->closedInfo)) return false;

	if (cdt_blockHasAnyTitle(block)) return false;
	if (cdt_blockHasAnyArtist(block)) return false;
	if (cdt_blockHasAnySongwriter(block)) return false;
	if (cdt_blockHasAnyComposer(block)) return false;
	if (cdt_blockHasAnyArranger(block)) return false;
	if (cdt_blockHasAnyCode(block)) return false;
	if (cdt_blockHasAnyMessage(block)) return false;
	return true;
}

static inline void
timeNumToText(int num, wchar_t** text) {
	wchar_t* p = *text;
	if (num > 9) {
		*p++ = num / 10 + L'0';
		*p++ = num % 10 + L'0';
	}
	else {
		*p++ = L'0';
		*p++ = num + L'0';
	}
	*text = p;
}

const wchar_t*
cdt_timeToText(const cdt_Time* time) {
	static wchar_t buffer[9]; // text eg. 32:12:45

	if (!cdt_timeIsValid(time)) return NULL;

	wchar_t* p = buffer;
	timeNumToText(time->min, &p);
	*p++ = L':';
	timeNumToText(time->sec, &p);
	*p++ = L':';
	timeNumToText(time->frame, &p);
	*p = L'\0';
	return buffer;
}

static inline bool
parseTimeNumText(int* num, const wchar_t** text) {
	const wchar_t* p = *text;
	if (*p < L'0' || *p > L'9') return false;
	int n = *p++ - L'0';
	if (*p < L'0' || *p > L'9') {
		*num = n;
	}
	else {
		*num = n * 10 + *p++ - L'0';
	}
	*text = p + 1;
	return true;
}

bool
cdt_timeFromText(cdt_Time* time, const wchar_t* p) {
	if (!parseTimeNumText(&time->min, &p)) return false;
	if (!parseTimeNumText(&time->sec, &p)) return false;
	if (!parseTimeNumText(&time->frame, &p)) return false;
	return true;
}

bool
cdt_timeFromCtl(cdt_Time* time,HWND ctl) {
	int cch;
	const wchar_t* t = wnd_getText(ctl, &cch);
	if (cch <= 0 || cch > 8) return false;
	return cdt_timeFromText(time, t);
}

static inline void
transferToc(cdt_Time* dst, const cdt_Time* src, int trackCount) {
	++trackCount;
	for (int i = 0; i < trackCount; ++i) {
		if (cdt_timeIsEmpty(src + i)) continue;
		dst[i] = src[i];
	}
}

int
cdt_transfer(Cdt* dst, int dstIndex, Cdt* src) {
	if (!cdt_setTrackRange(src, dst->trackFirst, dst->trackLast)) return 0;

	int result = 0;
	for (int srcIndex = 0; srcIndex < cdt_kBlockCountMax && dstIndex < cdt_kBlockCountMax; ++srcIndex) {
		cdt_Block* srcBlock = src->blocks + srcIndex;
		if (!srcBlock->heap) continue;

		cdt_Block* dstBlock = dst->blocks + dstIndex;
		if (dstBlock->heap) {
			HeapDestroy(dstBlock->heap);
		}
		*dstBlock = *srcBlock;
		srcBlock->heap = NULL;

		++dstIndex;
		++result;
	}
	if (!result) return 0;

	transferToc(dst->toc, src->toc, dst->trackLast - dst->trackFirst + 1);

	return result;
}


static inline charset_RegulateFunc
getRegulateFunc(int encoding) {
	switch (encoding) {
	case cdt_kEncodingIndex88591:
		return charset_regulate88591;
	case cdt_kEncodingIndexAscii:
		return charset_regulateAscii;
	case cdt_kEncodingIndexMsjis:
		return charset_regulateMsjis;
	}
	return NULL;
}

static inline int
regulateTitle(cdt_Block* block) {
	wchar_t* t = block->album->title;
}

int
cdt_blockRegulate(cdt_Block* block) {
	charset_RegulateFunc regulate = getRegulateFunc(block->album->encoding);
	if (!regulate) return 0;

	int result = 0;
	cdt_Album* album = block->album;
	result += regulate(album->title, -1);
	result += regulate(album->artist, -1);
	result += regulate(album->songwriter, -1);
	result += regulate(album->composer, -1);
	result += regulate(album->arranger, -1);
	result += regulate(album->message, -1);
	result += charset_regulateAscii(album->genreExtra, -1);
	result += charset_regulateAscii(album->catalog, -1);
	result += charset_regulateAscii(album->upc, -1);
	result += charset_regulate88591(album->closedInfo, -1);

	for (int i = 0; i < block->trackCount; ++i) {
		cdt_Track* track = block->tracks + i;
		result += regulate(track->title, -1);
		result += regulate(track->artist, -1);
		result += regulate(track->songwriter, -1);
		result += regulate(track->composer, -1);
		result += regulate(track->arranger, -1);
		result += regulate(track->message, -1);
		result += charset_regulateAscii(track->isrc, -1);
	}

	return result;
}

static inline bool
albumCopy(HANDLE heap, cdt_Album* dst, const cdt_Album* src) {
	if (!wcs_copy(heap, &dst->title, src->title)) return false;
	if (!wcs_copy(heap, &dst->artist, src->artist)) return false;
	if (!wcs_copy(heap, &dst->songwriter, src->songwriter)) return false;
	if (!wcs_copy(heap, &dst->composer, src->composer)) return false;
	if (!wcs_copy(heap, &dst->arranger, src->arranger)) return false;
	if (!wcs_copy(heap, &dst->message, src->message)) return false;
	if (!wcs_copy(heap, &dst->catalog, src->catalog)) return false;
	if (!wcs_copy(heap, &dst->genreExtra, src->genreExtra)) return false;
	if (!wcs_copy(heap, &dst->closedInfo, src->closedInfo)) return false;
	if (!wcs_copy(heap, &dst->upc, src->upc)) return false;

	dst->copyProtected = src->copyProtected;
	dst->encoding = src->encoding;
	dst->language = src->language;
	dst->genreCode = src->genreCode;

	return true;
}

static inline bool
trackCopy(HANDLE heap, cdt_Track* dst, const cdt_Track* src) {
	if (!wcs_copy(heap, &dst->title, src->title)) return false;
	if (!wcs_copy(heap, &dst->artist, src->artist)) return false;
	if (!wcs_copy(heap, &dst->songwriter, src->songwriter)) return false;
	if (!wcs_copy(heap, &dst->composer, src->composer)) return false;
	if (!wcs_copy(heap, &dst->arranger, src->arranger)) return false;
	if (!wcs_copy(heap, &dst->message, src->message)) return false;
	if (!wcs_copy(heap, &dst->isrc, src->isrc)) return false;

	return true;
}

// dst will be overwritten
static inline bool
blockCopy(cdt_Block* dst, const cdt_Block* src) {
	if (cdt_blockIsEmpty(src)) return false;

	if (dst->heap) {
		HeapDestroy(dst->heap);
		dst->heap = NULL;
	}
	if (!blockInit(dst, src->trackFirst, src->trackCount)) return false;
	bool ok = false;
	if (!albumCopy(dst->heap, dst->album, src->album)) goto cleanup;
	for (int i = 0; i < src->trackCount; ++i) {
		if (!trackCopy(dst->heap, dst->tracks + i, src->tracks + i)) goto cleanup;
	}

	ok = true;
cleanup:;
	if (!ok) {
		HeapDestroy(dst->heap);
		dst->heap = NULL;
	}
	return ok;
}

bool
cdt_copyBlock(Cdt* cdt, int srcIndex, int dstIndex) {
	assert(srcIndex >= 0 && srcIndex < cdt_kBlockCountMax);
	assert(dstIndex >= 0 && dstIndex < cdt_kBlockCountMax);
	if (srcIndex == dstIndex) return false;
	return blockCopy(cdt->blocks + dstIndex, cdt->blocks + srcIndex);
}

static BYTE
blockGetAlbumFields(const cdt_Block* block) {
	BYTE result = 0;
	if (cdt_blockHasAlbumSongwriter(block)) result |= albumFields_kSongwriterMask;
	if (cdt_blockHasAlbumComposer(block)) result |= albumFields_kComposerMask;
	if (cdt_blockHasAlbumArranger(block)) result |= albumFields_kArrangerMask;
	if (cdt_blockHasAlbumUpc(block)) result |= albumFields_kCodeMask;
	if (cdt_blockHasAlbumMessage(block)) result |= albumFields_kMessageMask;
	if (cdt_blockHasAlbumCatalog(block)) result |= albumFields_kCatalogMask;
	if (cdt_blockHasAlbumClosedInfo(block)) result |= albumFields_kClosedInfoMask;
	return result;
}

BYTE
cdt_getAlbumFields(const Cdt* cdt) {
	BYTE result = 0;
	for (int i = 0; i < cdt_kBlockCountMax; ++i) {
		const cdt_Block* block = cdt->blocks + i;
		if (!block->heap) continue;

		BYTE fields = blockGetAlbumFields(block);
		result |= fields;
	}
	if (cdt_hasAnyToc(cdt)) result |= albumFields_kTocMask;
	return result;
}

static BYTE
blockGetTrackFields(const cdt_Block* block) {
	BYTE result = 0;
	if (cdt_blockHasTrackTitle(block)) result |= trackFields_kTitleMask;
	if (cdt_blockHasTrackArtist(block)) result |= trackFields_kArtistMask;
	if (cdt_blockHasTrackSongwriter(block)) result |= trackFields_kSongwriterMask;
	if (cdt_blockHasTrackComposer(block)) result |= trackFields_kComposerMask;
	if (cdt_blockHasTrackArranger(block)) result |= trackFields_kArrangerMask;
	if (cdt_blockHasTrackIsrc(block)) result |= trackFields_kCodeMask;
	if (cdt_blockHasTrackMessage(block)) result |= trackFields_kMessageMask;
	return result;
}

BYTE
cdt_getTrackFields(const Cdt* cdt) {
	BYTE result = 0;
	for (int i = 0; i < cdt_kBlockCountMax; ++i) {
		const cdt_Block* block = cdt->blocks + i;
		if (!block->heap) continue;

		BYTE fields = blockGetTrackFields(block);
		result |= fields;
	}
	if (cdt_hasAnyToc(cdt)) result |= trackFields_kTocMask;
	return result ? result : trackFields_kTitleMask;
}
