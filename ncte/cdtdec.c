#include "cdtdec.h"

#include <Windows.h>

#include <stdlib.h>
#include <assert.h>

#include "cdtcomm.h"
#include "file.h"
#include "mem.h"


typedef struct PackedGenre {
	cdt_PackHeader header;
	uint16_t genre;
	char genreExtra;
}PackedGenre;

typedef struct PackedSummary {
	cdt_PackHeader header0;
	union {
		uint8_t encoding;
		struct {
			uint8_t : 7;
			uint8_t isMultiByte : 1;
		};
	};
	uint8_t trackFirst;
	uint8_t trackLast;
	uint8_t copyProtected;
	uint8_t packCount80;
	uint8_t packCount81;
	uint8_t packCount82;
	uint8_t packCount83;
	uint8_t packCount84;
	uint8_t packCount85;
	uint8_t packCount86;
	uint8_t packCount87;
	uint16_t crc0;

	cdt_PackHeader header1;
	uint8_t packCount88;
	uint8_t packCount89;
	uint8_t packCount8a_;
	uint8_t packCount8b_;
	uint8_t packCount8c_;
	uint8_t packCount8d;
	uint8_t packCount8e;
	uint8_t packCount8f;
	uint8_t blockPackLastA[cdt_kBlockCountMax / 2];
	uint16_t crc1;

	cdt_PackHeader header2;
	uint8_t blockPackLastB[cdt_kBlockCountMax / 2];
	uint8_t blockLanguage[cdt_kBlockCountMax];
	uint16_t crc2;
}PackedSummary;
static_assert(sizeof(PackedSummary) == 18 * 3, "PackedSummary should be 18 * 3 in size");

static inline int
getBlockTotal(const PackedSummary* summary) {
	int result = 0;
	const uint8_t* p;
	for (p = summary->blockPackLastA; p < (const uint8_t*)&summary->crc1; ++p) {
		if (*p) ++result;
		else return result;
	}
	for (p = summary->blockPackLastB; p < summary->blockLanguage; ++p) {
		if (*p) ++result;
		else return result;
	}
	return result;
}

typedef wchar_t*(*DecTextProc)(HANDLE heap, const char* rawText, wchar_t** buffer);

typedef struct Context {
	HANDLE   heap;
	union {
		char* strBuf;
		wchar_t* wcsBuf;
	};

	union {
		const uint8_t* bin;
		const cdt_Pack* pack;
	};
	ptrdiff_t cbRemain;
	int payloadP;
	UINT codepage;

	uint8_t blockTotal; // total block count read from summary
	uint8_t blockCount; // decoded count

	uint8_t trackIndex;
	uint8_t packCount;

	union {
		const PackedSummary* summary;
		const cdt_Pack* summaryPack0;
	};

	DecTextProc decText;
}Context;

static inline const PackedSummary*
retrieveNextSummary(Context* ctx) {
	ctx->summary = NULL;
	const uint8_t* end = ctx->bin + ctx->cbRemain - cdt_kSummarySize;
	for (const cdt_Pack* p = ctx->pack; (const uint8_t*)p < end; ++p) {
		if (p->type < 0x80) return NULL;
		if (p->type > 0x8F) return NULL;
		if (p->type == 0x8F) {
			ctx->summary = (const PackedSummary*)p;
			return ctx->summary;
		}
	}
	return NULL;
}

static inline void
advancePack(Context* ctx) {
	ctx->payloadP = 0;
	++ctx->pack;
	ctx->cbRemain -= cdt_kPackSize;
}

// Return count of bytes copied, including the terminating NUL.
static inline int
extractStr(Context* ctx) {
	// TODO: This function doesn't handle malformed bin and could be dangerous, do something
	char* p = ctx->strBuf;
	for (; p - ctx->strBuf < cdt_kCchMaxStrBuffer; ++p) {
		*p = ctx->pack->payload[ctx->payloadP];
		if (++ctx->payloadP == cdt_kPayloadSize) {
			advancePack(ctx);
		}
		if (!*p) break;
	}
	return (int)(p - ctx->strBuf) + 1;
}

// Return count of bytes copied, including the terminating NUL.
static inline int
extractWcs(Context* ctx) {
	// TODO: This function doesn't handle malformed bin and could be dangerous, do something
	wchar_t* p = ctx->wcsBuf;
	for (; p - ctx->wcsBuf < cdt_kCchMaxWcsBuffer; ++p) {
		*p = ctx->pack->payloadWcs[ctx->payloadP];
		if (++ctx->payloadP == cdt_kPayloadWcsSize) {
			advancePack(ctx);
		}
		if (!*p) break;
	}
	return (int)((char*)p - ctx->strBuf) + sizeof(wchar_t);
}

static inline int
extractText(Context* ctx) {
	if (ctx->summary->isMultiByte) return extractWcs(ctx);
	else return extractStr(ctx);
}

static inline int
extractTextsOfCurrentType(cdt_Block* block, size_t albumFieldOffset, size_t trackFieldOffset, Context* ctx) {
	int result = 0;
	ctx->payloadP = 0;
	int cb = extractText(ctx);
	if (!cb) return 0;
	wchar_t** field = (wchar_t**)((char*)block->album + albumFieldOffset);
	if (!cdt_setTextUsingCodepage(block, field , ctx->strBuf, cb, ctx->codepage)) return 0;
	for (int i = 0; i < block->trackCount; ++i) {
		cb = extractText(ctx);
		if (!cb) return result;
		field = (wchar_t**)((char*)(block->tracks + i) + trackFieldOffset);
		if (!cdt_setTextUsingCodepage(block, field, ctx->strBuf, cb, ctx->codepage)) return result;
		++result;
	}
	return result;
}

static inline bool
beginType(uint8_t type, Context* ctx) {
	return (ctx->pack->type == type);
}

static inline void
endType(Context* ctx) {
	if (ctx->payloadP) advancePack(ctx);
}

// Return count of decoded packs
static inline int
dec80(cdt_Block* block, Context* ctx) {
	if (!beginType(0x80, ctx)) return 0;
	int result = extractTextsOfCurrentType(block, offsetof(cdt_Album,title), offsetof(cdt_Track,title), ctx);
	endType(ctx);
	return result;
}

static inline int
dec81(cdt_Block* block, Context* ctx) {
	if (!beginType(0x81, ctx)) return 0;
	int result = extractTextsOfCurrentType(block, offsetof(cdt_Album, artist), offsetof(cdt_Track, artist), ctx);
	endType(ctx);
	return result;
}

static inline int
dec82(cdt_Block* block, Context* ctx) {
	if (!beginType(0x82, ctx)) return 0;
	int result = extractTextsOfCurrentType(block, offsetof(cdt_Album, songwriter), offsetof(cdt_Track, songwriter), ctx);
	endType(ctx);
	return result;
}

static inline int
dec83(cdt_Block* block, Context* ctx) {
	if (!beginType(0x83, ctx)) return 0;
	int result = extractTextsOfCurrentType(block, offsetof(cdt_Album, composer), offsetof(cdt_Track, composer), ctx);
	endType(ctx);
	return result;
}

static inline int
dec84(cdt_Block* block, Context* ctx) {
	if (!beginType(0x84, ctx)) return 0;
	int result = extractTextsOfCurrentType(block, offsetof(cdt_Album, arranger), offsetof(cdt_Track, arranger), ctx);
	endType(ctx);
	return result;
}

static inline int
dec85(cdt_Block* block, Context* ctx) {
	if (!beginType(0x85, ctx)) return 0;
	int result = extractTextsOfCurrentType(block, offsetof(cdt_Album, message), offsetof(cdt_Track, message), ctx);
	endType(ctx);
	return result;
}

static inline int
dec86(cdt_Block* block, Context* ctx) {
	if (!beginType(0x86, ctx)) return 0;
	int cb = extractStr(ctx);
	if (!cb) goto cleanup;
	if (!cdt_setTextUsingCodepage(block, &block->album->catalog, ctx->strBuf, cb, cdt_kCpAscii)) cb = 0;
cleanup:;
	endType(ctx);
	return cb;
}

// Return cbGenreExtra + 1
// If genreCode exists, but genreExtra not exists, return 1
// No any genre info return 0
static inline int
dec87(cdt_Block* block, Context* ctx) {
	if (!beginType(0x87, ctx)) return 0;
	uint16_t genre = *(uint16_t*)ctx->pack->payload;
	genre = _byteswap_ushort(genre);
	int index = cdt_getGenreIndex(genre);
	if (index < 0) index = 0;
	block->album->genreCode = index;

	ctx->payloadP = sizeof(uint16_t);
	int cb = extractStr(ctx);
	if (!cb) goto cleanup;
	if (!cdt_setTextUsingCodepage(block, &block->album->genreExtra, ctx->strBuf, cb, cdt_kCpAscii)) cb = 0;
	++cb;
cleanup:;
	endType(ctx);
	return cb;
}

static inline int
dec88(cdt_Time* toc, Context* ctx) {
	if (!beginType(0x88, ctx)) return 0;
	const trackCount = ctx->summary->trackLast - ctx->summary->trackFirst + 1;
	cdt_Time* time = toc + trackCount;
	const uint8_t* payload = ctx->pack->payload;
	time->min = payload[3];
	time->sec = payload[4];
	time->frame = payload[5];
	advancePack(ctx);
	for (int i = 0; i < trackCount; ++i) {
		time = toc + i;
		payload = ctx->pack->payload;
		time->min = payload[ctx->payloadP++];
		time->sec = payload[ctx->payloadP++];
		time->frame = payload[ctx->payloadP++];
		if (ctx->payloadP == cdt_kPayloadSize) advancePack(ctx);
	}
	endType(ctx);
	return 1;
}

static inline int
dec8d(cdt_Block* block, Context* ctx) {
	if (!beginType(0x8D, ctx)) return 0;
	int cb = extractStr(ctx);
	if (!cb) goto cleanup;
	if (!cdt_setTextUsingCodepage(block, &block->album->closedInfo, ctx->strBuf, cb, cdt_kCp88591)) cb = 0;
cleanup:;
	endType(ctx);
	return cb;
}

static inline int
extractTextOfCurrentTypeAsAscii(cdt_Block* block, size_t albumFieldOffset, size_t trackFieldOffset, Context* ctx) {
	int result = 0;
	ctx->payloadP = 0;
	int cb = extractStr(ctx);
	if (!cb) return 0;
	wchar_t** field = (wchar_t**)((char*)block->album + albumFieldOffset);
	if (!cdt_setTextUsingCodepage(block, field, ctx->strBuf, cb, cdt_kCpAscii)) return 0;
	for (int i = 0; i < block->trackCount; ++i) {
		cb = extractStr(ctx);
		if (!cb) return result;
		field = (wchar_t**)((char*)(block->tracks + i) + trackFieldOffset);
		if (!cdt_setTextUsingCodepage(block, field, ctx->strBuf, cb, cdt_kCpAscii)) return result;
		++result;
	}
	return result;
}

static inline int
dec8e(cdt_Block* block, Context* ctx) {
	if (!beginType(0x8E, ctx)) return 0;
	int result = extractTextOfCurrentTypeAsAscii(block, offsetof(cdt_Album, upc), offsetof(cdt_Track, isrc), ctx);
	endType(ctx);
	return result;
}

static inline void
beginBlock(cdt_Block* block, Context* ctx) {
	int index = cdt_getEncodingIndex(ctx->summary->encoding);
	if (index < 0) index = 0; // TODO: handle this
	block->album->encoding = index;
	block->album->copyProtected = ctx->summary->copyProtected;
	ctx->codepage = cdt_kCodepages[index];
}

static inline void
endBlock(Context* ctx) {
	const cdt_Pack* p = 3 + ctx->summaryPack0;
	ctx->cbRemain -= (uint8_t*)p - (uint8_t*)ctx->pack;
	assert(ctx->cbRemain >= 0);
	ctx->pack = p;
}

static inline bool
decBlock(cdt_Block* block, cdt_Time* toc, Context* ctx) {
	beginBlock(block, ctx);

	if (!dec80(block, ctx)) return false;
	dec81(block, ctx);
	dec82(block, ctx);
	dec83(block, ctx);
	dec84(block, ctx);
	dec85(block, ctx);
	dec86(block, ctx);
	dec87(block, ctx);
	dec88(toc, ctx);
	dec8d(block, ctx);
	dec8e(block, ctx);

	endBlock(ctx);
	return true;
}

static inline int
reserveBlocks(Cdt* cdt, const PackedSummary* summary) {
	if (!cdt_setTrackRange(cdt, summary->trackFirst, summary->trackLast)) return 0;
	int blockTotal = getBlockTotal(summary);
	for (int i = 1; i < blockTotal; ++i) {
		if (!cdt_reserveBlock(cdt, i)) return 0;
	}
	return blockTotal;
}

static inline void
setLanguages(Cdt* cdt, const Context* ctx) {
	for (int i = 0; i < ctx->blockTotal; ++i) {
		int index = cdt_getLanguageIndex(ctx->summary->blockLanguage[i]);
		// TODO: properly handle index == -1
		if (index < 0) index = 0;
		cdt->blocks[i].album->language = index;
	}
}

int
cdt_decode(const void* bin, int binSize, Cdt* cdt, CdtDecContext context) {
	if (binSize < cdt_kBlockSizeMin) return 0;
	Context* ctx = (Context*)context;
	ctx->pack = (cdt_Pack*)bin;
	ctx->cbRemain = binSize;

	if (!retrieveNextSummary(ctx)) return 0;
	ctx->blockTotal = reserveBlocks(cdt, ctx->summary);
	if (!ctx->blockTotal) return 0;
	setLanguages(cdt, ctx);

	ctx->blockCount = 0;
	for (; ctx->summary; retrieveNextSummary(ctx)) {
		if (!decBlock(cdt->blocks + ctx->blockCount, cdt->toc, ctx)) return ctx->blockCount;
		++ctx->blockCount;
	}
	return ctx->blockCount;
}

int
cdt_decodeFile(const wchar_t* path, Cdt* cdt, CdtDecContext ctx) {
	int size;
	const uint8_t* bin = file_openView(path, &size);
	if (!bin) return 0;
	int result = cdt_decode(bin, size, cdt, ctx);
	file_closeView(bin);
	return result;
}

CdtDecContext
cdt_createDecContext(void) {
	HANDLE heap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
	if (!heap) return NULL;
	Context* ctx = HeapAlloc(heap, 0, sizeof(Context));
	if (!ctx) {
		HeapDestroy(heap);
		return false;
	}
	ctx->strBuf = HeapAlloc(heap, 0, cdt_kCchMaxStrBuffer);
	if (!ctx->strBuf) {
		HeapDestroy(heap);
		return false;
	}
	ctx->heap = heap;
	return ctx;
}

void
cdt_destroyDecContext(CdtDecContext context) {
	assert(context);
	Context* ctx = (Context*)context;
	BOOL ok = HeapDestroy(ctx->heap);
	assert(ok);
}
