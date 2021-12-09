#include "cdtenc.h"

#include <Windows.h>

#include <intrin.h> // _byteswap_ushort
#include <assert.h>

#include "cdtcomm.h"
#include "crc.h"
#include "debug.h"
#include "fields.h"
#include "err.h"
#include "str.h"
#include "charset.h"


typedef struct BlockSummary {
	union {
		uint8_t encoding;
		struct {
			uint8_t : 7;
			uint8_t isMultiByte : 1;
		};
	};
	uint8_t trackFirst;
	uint8_t trackLast;
	uint8_t copyProtection;
	uint8_t typePackCount[cdt_kTypeCountMax];
	uint8_t blockPackLast[cdt_kBlockCountMax];
	uint8_t blockLanguage[cdt_kBlockCountMax];
}BlockSummary;
static_assert(sizeof(BlockSummary) == 36, "BlockSummary should be 36 in size");

typedef char* (*EncTextProc)(HANDLE heap, const wchar_t* text, char** buffer);

typedef struct Conntext {
	HANDLE heap;
	BYTE* bin;
	int binLen;
	int packCount;

	cdt_Pack* pack;
	int payloadP;

	// Block contents
	BlockSummary summary;
	int summaryP[cdt_kBlockCountMax]; // first pack's index of each summary
	BYTE blockCount;
	EncTextProc encText;

	// cdt_Pack contents
	cdt_PackHeader header;

	//AlbumFields(albumFields);
	//TrackFields(trackFields);

	// Temporary memory objects
	union {
		char* strBuf;
		wchar_t* wcsBuf;
	};
	union {
		char* strBufP;
		wchar_t* wcsBufP;
	};
}Context;


static inline void
checkWcsNotEmpty(const wchar_t* text) {
	if (wcs_isEmpty(text)) RaiseException(kErrEmptyText, 0, 0, NULL);
}

static inline void
reserveStrBuffer(HANDLE heap, char** buffer, int cb) {
	if (!*buffer) {
		*buffer = HeapAlloc(heap, 0, cb);
	}
	else if (HeapSize(heap, 0, *buffer) < (size_t)cb) {
		*buffer = HeapReAlloc(heap, 0, *buffer, cb);
	}
}

static inline void
reserveWcsBuffer(HANDLE heap, wchar_t** buffer, int cch) {
	int cb = cch * sizeof(wchar_t);
	reserveStrBuffer(heap, (char**)buffer, cb);
}

static inline void
makeEmptyStrBuffer(HANDLE heap, char** buffer) {
	reserveStrBuffer(heap, buffer, 1);
	(*buffer)[0] = '\0';
}

static inline char*
wcsToStr(HANDLE heap, const wchar_t* text, char** buffer, UINT codePage) {
	if (wcs_isEmpty(text)) {
		makeEmptyStrBuffer(heap, buffer);
		return *buffer;
	}

	int cb = WideCharToMultiByte(codePage, 0, text, -1, NULL, 0, NULL, NULL);
	if (!cb) RaiseException(kErrBadTranscoding, 0, 0, NULL);
	reserveStrBuffer(heap, buffer, cb);
	cb = WideCharToMultiByte(codePage, 0, text, -1, *buffer, cb, NULL, NULL);
	assert(cb);
	return *buffer;
}

static char*
wcsTo88591(HANDLE heap, const wchar_t* text, char** buffer) {
	return wcsToStr(heap, text, buffer, cdt_kCp88591);
}

static char*
wcsToAscii(HANDLE heap, const wchar_t* text, char** buffer) {
	return wcsToStr(heap, text, buffer, cdt_kCpAscii);
}

static inline void
makeEmptyWcsBuffer(HANDLE heap, char** buffer) {
	wchar_t** wcs = (wchar_t**)buffer;
	reserveWcsBuffer(heap, wcs, 1);
	(*wcs)[0] = L'\0';
}

static char*
wcsToMsjis(HANDLE heap, const wchar_t* text, char** buffer) {
	if (wcs_isEmpty(text)) {
		makeEmptyWcsBuffer(heap, buffer);
		return *buffer;
	}

	int cch = lstrlen(text);
	reserveWcsBuffer(heap, (wchar_t**)buffer, cch + 1);
	if (!charset_utf16ToMsjis(text, cch, *(wchar_t**)buffer)) RaiseException(kErrBadTranscoding, 0, 0, NULL);

	return *buffer;
}

static char*
wcsToUtf16be(HANDLE heap, const wchar_t* text, char** buffer) {
	if (wcs_isEmpty(text)) {
		makeEmptyWcsBuffer(heap, buffer);
		return *buffer;
	}

	int cch = lstrlen(text) + 1;
	reserveWcsBuffer(heap, (wchar_t**)buffer, cch);

	wchar_t* wcs = (wchar_t*)*buffer;
	for (int i = 0; i < cch; ++i) {
		wcs[i] = _byteswap_ushort((WORD)text[i]);
	}
	return *buffer;
}

static const EncTextProc kEncTextProcs[] = {
	wcsTo88591,
	wcsToAscii,
	wcsToMsjis,
	wcsToUtf16be,
};


static inline void
fillCrc(cdt_Pack* pack) {
	WORD crc = crc_calc((BYTE*)pack, cdt_kHeaderSize + cdt_kPayloadSize);
	crc = _byteswap_ushort(crc);
	pack->crc = crc;
}

static inline void
beginPack(Context* ctx) {
	if (ctx->header.packNumber == 0xFF) err_raise(kErrPackCountOverflow);
	*(cdt_PackHeader*)ctx->pack = ctx->header;
}

static inline void
endPack(Context* ctx) {
	assert(ctx->payloadP);
	fillCrc(ctx->pack);
	ctx->payloadP = 0;
	++ctx->pack;
	++ctx->packCount;
	++ctx->header.packNumber;
}

static inline void
packBufferAsDoubleByte(Context* ctx) {
	ctx->header.prevCharCount = 0;
	for (ctx->wcsBufP = ctx->wcsBuf; /* blank */; ++ctx->wcsBufP) {
		if (!ctx->payloadP) {
			beginPack(ctx);
		}

		ctx->pack->payloadWcs[ctx->payloadP / 2] = *ctx->wcsBufP;
		ctx->payloadP += 2;

		if (ctx->payloadP == cdt_kPayloadSize) {
			if (ctx->header.prevCharCount > 0xC) ctx->header.prevCharCount = 0xF;
			else ctx->header.prevCharCount = (BYTE)(ctx->wcsBufP - ctx->wcsBuf + 1);
			endPack(ctx);
		}

		if (!*ctx->wcsBufP) break;
	}
	ctx->header.prevCharCount = (cdt_kPayloadSize - ctx->payloadP) / 2;
}

static inline void
packBufferAsSingleByte(Context* ctx) {
	ctx->header.prevCharCount = 0;
	for (ctx->strBufP = ctx->strBuf; /* blank */; ++ctx->strBufP) {
		if (!ctx->payloadP) {
			beginPack(ctx);
		}

		ctx->pack->payload[ctx->payloadP++] = *ctx->strBufP;

		if (ctx->payloadP == cdt_kPayloadSize) {
			if (ctx->header.prevCharCount) ctx->header.prevCharCount = 0xF;
			else ctx->header.prevCharCount = (BYTE)(ctx->strBufP - ctx->strBuf + 1);
			endPack(ctx);
		}

		if (!*ctx->strBufP) break;
	}
	ctx->header.prevCharCount = cdt_kPayloadSize - ctx->payloadP;
}

static inline void
packText(const wchar_t* text, Context* ctx) {
	ctx->encText(ctx->heap, text, &ctx->strBuf);
	if (ctx->summary.isMultiByte) packBufferAsDoubleByte(ctx);
	else packBufferAsSingleByte(ctx);
}

static inline void
packAscii(const wchar_t* text, Context* ctx) {
	wcsToAscii(ctx->heap, text, &ctx->strBuf);
	packBufferAsSingleByte(ctx);
}

static inline void
pack88591(const wchar_t* text, Context* ctx) {
	wcsTo88591(ctx->heap, text, &ctx->strBuf);
	packBufferAsSingleByte(ctx);
}

static inline void
beginType(Context* ctx, BYTE type) {
	cdt_PackHeader* hdr = &ctx->header;
	hdr->type = type;
	hdr->trackNumber = 0;
	ctx->summary.typePackCount[type - 0x80] = hdr->packNumber;
}

static inline void
endType(Context* ctx, BYTE type) {
	if (ctx->payloadP) {
		for (int i = ctx->payloadP; i < cdt_kPayloadSize; ++i) {
			ctx->pack->payload[i] = 0;
		}
		endPack(ctx);
	}
	ctx->header.prevCharCount = 0;

	cdt_PackHeader* hdr = &ctx->header;
	int typeIndex = type - 0x80;
	ctx->summary.typePackCount[typeIndex] = hdr->packNumber - ctx->summary.typePackCount[typeIndex];
}

// title
static inline void
enc80(const cdt_Block* block, Context* ctx) {
	beginType(ctx, 0x80);
	cdt_PackHeader* hdr = &ctx->header;

	checkWcsNotEmpty(block->album->title);
	packText(block->album->title, ctx);

	for (int i = ctx->summary.trackFirst; i <= ctx->summary.trackLast; ++i) {
		hdr->trackNumber = i;
		const wchar_t* text = block->tracks[i - ctx->summary.trackFirst].title;
		checkWcsNotEmpty(text);
		packText(text, ctx);
	}

	endType(ctx, 0x80);
}

// Artist
static inline void
enc81(const cdt_Block* block, Context* ctx) {
	if (!cdt_blockHasAnyArtist(block)) return;

	beginType(ctx, 0x81);
	cdt_PackHeader* hdr = &ctx->header;

	packText(block->album->artist, ctx);
	for (int i = ctx->summary.trackFirst; i <= ctx->summary.trackLast; ++i) {
		hdr->trackNumber = i;
		const wchar_t* text = block->tracks[i - ctx->summary.trackFirst].artist;
		packText(text, ctx);
	}

	endType(ctx, 0x81);
}

static inline void
enc82(const cdt_Block* block, Context* ctx) {
	if (!cdt_blockHasAnySongwriter(block)) return;

	beginType(ctx, 0x82);
	cdt_PackHeader* hdr = &ctx->header;

	packText(block->album->songwriter, ctx);
	for (int i = ctx->summary.trackFirst; i <= ctx->summary.trackLast; ++i) {
		hdr->trackNumber = i;
		const wchar_t* text = block->tracks[i - ctx->summary.trackFirst].songwriter;
		packText(text, ctx);
	}

	endType(ctx, 0x82);
}

static inline void
enc83(const cdt_Block* block, Context* ctx) {
	if (!cdt_blockHasAnyComposer(block)) return;

	beginType(ctx, 0x83);
	cdt_PackHeader* hdr = &ctx->header;

	packText(block->album->composer, ctx);
	for (int i = ctx->summary.trackFirst; i <= ctx->summary.trackLast; ++i) {
		hdr->trackNumber = i;
		const wchar_t* text = block->tracks[i - ctx->summary.trackFirst].composer;
		packText(text, ctx);
	}

	endType(ctx, 0x83);
}

static inline void
enc84(const cdt_Block* block, Context* ctx) {
	if (!cdt_blockHasAnyArranger(block)) return;

	beginType(ctx, 0x84);
	cdt_PackHeader* hdr = &ctx->header;

	packText(block->album->arranger, ctx);
	for (int i = ctx->summary.trackFirst; i <= ctx->summary.trackLast; ++i) {
		hdr->trackNumber = i;
		const wchar_t* text = block->tracks[i - ctx->summary.trackFirst].arranger;
		packText(text, ctx);
	}

	endType(ctx, 0x84);
}

//static inline bool
//has85(const cdt_Block* block, Context* ctx) {
//	if (!ctx->albumFields[albumFields_kMessage] && !ctx->trackFields[trackFields_kMessage]) return false;
//	if (!cdt_blockHasAnyMessage(block)) return false;
//	return true;
//}

static inline void
enc85(const cdt_Block* block, Context* ctx) {
	if (!cdt_blockHasAnyMessage(block)) return;

	beginType(ctx, 0x85);
	cdt_PackHeader* hdr = &ctx->header;

	packText(block->album->message, ctx);
	for (int i = ctx->summary.trackFirst; i <= ctx->summary.trackLast; ++i) {
		hdr->trackNumber = i;
		const wchar_t* text = block->tracks[i - ctx->summary.trackFirst].message;
		packText(text, ctx);
	}

	endType(ctx, 0x85);
}

static inline void
enc86(const cdt_Block* block, Context* ctx) {
	if (wcs_isEmpty(block->album->catalog)) return;

	beginType(ctx, 0x86);
	packAscii(block->album->catalog, ctx);
	endType(ctx, 0x86);
}

static inline void
enc87(const cdt_Block* block, Context* ctx) {
	if (!block->album->genreCode) return;
	beginType(ctx, 0x87);

	beginPack(ctx);
	*(WORD*)ctx->pack->payload = _byteswap_ushort((WORD)block->album->genreCode);
	ctx->payloadP = sizeof(WORD);

	if (wcs_isEmpty(block->album->genreExtra)) {
		endPack(ctx);
	}
	else {
		pack88591(block->album->genreExtra, ctx);
	}

	endType(ctx, 0x87);
}

static inline void
packTocForAlbum(const cdt_Time* toc, Context* ctx) {
	if (!cdt_timeIsValid(toc)) err_raise(kErrBadTimeRange);
	beginPack(ctx);
	const cdt_Time* time = toc + (ctx->summary.trackLast - ctx->summary.trackFirst + 1);
	uint8_t* payload = ctx->pack->payload;
	payload[0] = ctx->summary.trackFirst;
	payload[1] = ctx->summary.trackLast;
	payload[2] = 0;
	payload[3] = time->min;
	payload[4] = time->sec;
	payload[5] = time->frame;
	for (int i = 6; i < cdt_kPayloadSize; ++i) {
		payload[i] = 0;
	}
	ctx->payloadP = cdt_kPayloadSize;
	endPack(ctx);
}

static inline void
packTime(const cdt_Time* time, Context* ctx) {
	if (!cdt_timeIsValid(time)) err_raise(kErrBadTimeRange);

	if (!ctx->payloadP) beginPack(ctx);
	uint8_t* payload = ctx->pack->payload;

	if (ctx->payloadP >= cdt_kPayloadSize) err_raise(kErrBadTocAlignment);
	payload[ctx->payloadP++] = time->min;
	if (ctx->payloadP >= cdt_kPayloadSize) err_raise(kErrBadTocAlignment);
	payload[ctx->payloadP++] = time->sec;
	if (ctx->payloadP >= cdt_kPayloadSize) err_raise(kErrBadTocAlignment);
	payload[ctx->payloadP++] = time->frame;

	if (ctx->payloadP == cdt_kPayloadSize) {
		endPack(ctx);
	}
}

static inline void
packTocForTracks(const cdt_Time* toc, Context* ctx) {
	for (int i = ctx->summary.trackFirst; i <= ctx->summary.trackLast; ++i) {
		ctx->header.trackNumber = i;
		const cdt_Time* time = toc + i - ctx->summary.trackFirst;
		packTime(time, ctx);
	}
	if (ctx->payloadP) endPack(ctx);
}

// TOC
static inline void
enc88(const cdt_Time* toc, Context* ctx) {
	if (!cdt_tocIsValid(toc, ctx->summary.trackLast - ctx->summary.trackFirst + 1)) return;
	beginType(ctx, 0x88);
	packTocForAlbum(toc, ctx);
	packTocForTracks(toc, ctx);
	endType(ctx, 0x88);
}

// TOC for Promotion, demostration, etc
static inline void
enc89(const cdt_Block* block, Context* ctx) {
	
}
 
static inline void
enc8d(const cdt_Block* block, Context* ctx) {
	if (wcs_isEmpty(block->album->closedInfo)) return;

	beginType(ctx, 0x8D);
	pack88591(block->album->closedInfo, ctx);
	endType(ctx, 0x8D);
}

static inline void
enc8e(const cdt_Block* block, Context* ctx) {
	if (!cdt_blockHasAnyCode(block)) return;
	
	beginType(ctx, 0x8E);
	cdt_PackHeader* hdr = &ctx->header;

	packAscii(block->album->upc, ctx);
	for (int i = ctx->summary.trackFirst; i <= ctx->summary.trackLast; ++i) {
		hdr->trackNumber = i;
		const wchar_t* text = block->tracks[i - ctx->summary.trackFirst].isrc;
		packAscii(text, ctx);
	}

	endType(ctx, 0x8E);
}

// Block summary
static inline void
enc8f(const cdt_Block* block, Context* ctx) {
	cdt_PackHeader* hdr = &ctx->header;
	const BYTE* src = (const BYTE*)&ctx->summary;

	ctx->summary.blockPackLast[ctx->blockCount] = hdr->packNumber + 2;
	ctx->summary.typePackCount[0xF] = 3;

	hdr->type = 0x8F;
	for (BYTE i = 0; i < 3; ++i) {
		hdr->trackNumber = i;
		*(cdt_PackHeader*)ctx->pack = ctx->header;
		CopyMemory(ctx->pack->payload, src, cdt_kPayloadSize);
		fillCrc(ctx->pack);
		++ctx->pack;
		++ctx->packCount;
		++ctx->header.packNumber;
		src += cdt_kPayloadSize;
	}
}

static inline void
beginBlock(const cdt_Block* block, Context* ctx) {
	ctx->summary.encoding = (BYTE)cdt_kEncodings[block->album->encoding].code;
	ctx->summary.copyProtection = block->album->copyProtected ? 0x03 : 0x00;
	ctx->summary.blockLanguage[ctx->blockCount] = (BYTE)cdt_kLanguages[block->album->language].code;
	ctx->encText = kEncTextProcs[block->album->encoding];

	cdt_PackHeader* hdr = &ctx->header;
	hdr->packNumber = 0;
	hdr->prevCharCount = 0;
	hdr->blockNumber = ctx->blockCount;
	hdr->isMultiByte = ctx->summary.isMultiByte;
}

static inline void
endBlock(Context* ctx) {
	ctx->summaryP[ctx->blockCount] = ctx->packCount - 3;
	++ctx->blockCount;
}

static inline void
encBlock(const cdt_Block* block, const cdt_Time* toc, Context* ctx) {
	beginBlock(block, ctx);

	enc80(block, ctx);
	enc81(block, ctx);
	enc82(block, ctx);
	enc83(block, ctx);
	enc84(block, ctx);
	enc85(block, ctx);
	enc86(block, ctx);
	enc87(block, ctx);

	enc88(toc, ctx);
	enc89(block, ctx);

	enc8d(block, ctx);
	enc8e(block, ctx);
	enc8f(block, ctx);

	endBlock(ctx);
}

static inline void
copyBlockPackLast(const cdt_Pack* src, cdt_Pack* dst) {
	assert(src != dst);
	ptrdiff_t offset = cdt_kPackSize + cdt_kHeaderSize + 8;
	const uint32_t* srcP = (uint32_t*)((uint8_t*)src + offset);
	uint32_t* dstP = (uint32_t*)((uint8_t*)dst + offset);
	*dstP = *srcP;

	offset += sizeof(uint32_t) + cdt_kCrcSize + cdt_kHeaderSize;
	srcP = (uint32_t*)((uint8_t*)src + offset);
	dstP = (uint32_t*)((uint8_t*)dst + offset);
	*dstP = *srcP;
}

static inline void
copyBlockLanguage(const cdt_Pack* src, cdt_Pack* dst) {
	assert(src != dst);
	ptrdiff_t offset = cdt_kPackSize + cdt_kPackSize + cdt_kHeaderSize + 4;
	const uint64_t* srcP = (uint64_t*)((uint8_t*)src + offset);
	uint64_t* dstP = (uint64_t*)((uint8_t*)dst + offset);
	*dstP = *srcP;
}

// write blockPackLast and blockLanguagCode
static inline void
rewriteSummary(Context* ctx) {
	const cdt_Pack* src = (cdt_Pack*)(ctx->bin) + ctx->summaryP[ctx->blockCount - 1];
	for (int i = 0; i < ctx->blockCount - 1; ++i) {
		cdt_Pack* dst = (cdt_Pack*)(ctx->bin) + ctx->summaryP[i];
		copyBlockPackLast(src, dst);
		copyBlockLanguage(src, dst);
		fillCrc(dst + 1);
		fillCrc(dst + 2);
	}
}

bool
cdt_encode(const Cdt* cdt, CdtEncContext context) {
	Context* ctx = (Context*)context;
	ctx->binLen = 0;
	ctx->packCount = 0;
	ctx->blockCount = 0;
	ctx->pack = (cdt_Pack*)ctx->bin;

	ZeroMemory(&ctx->summary, sizeof(BlockSummary));
	ctx->summary.trackFirst = cdt->trackFirst;
	ctx->summary.trackLast = cdt->trackLast;

	for (int i = 0; i < cdt_kBlockCountMax; ++i) {
		const cdt_Block* block = cdt->blocks + i;
		if (!block->heap) continue;
		if (!cdt_blockHasFullTitle(block)) continue;
		__try {
			encBlock(block, cdt->toc, ctx);
		}
		__except (1) {
			return false;
		}
	}
	rewriteSummary(ctx);
	ctx->binLen = cdt_kPackSize * ctx->packCount;
	return true;
}

CdtEncContext
cdt_createEncContext(void) {
	HANDLE heap = HeapCreate(HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE, 0, 0);
	if (!heap) return false;

	Context* ctx = NULL;
	__try {
		ctx = HeapAlloc(heap, 0, sizeof(Context));
		ctx->bin = HeapAlloc(heap, 0, cdt_kBinSizeMax);
		ctx->strBuf = HeapAlloc(heap, 0, cdt_kCchMaxStrBuffer);
	}
	__except (1) {
		HeapDestroy(heap);
		return NULL;
	}

	ctx->heap = heap;
	//fields_setByMask(ctx->albumFields, albumFields_kEnd, albumMask);
	//fields_setByMask(ctx->trackFields, trackFields_kEnd, trackMask);
	return ctx;
}

void
cdt_destroyEncContext(CdtEncContext context) {
	assert(context);
	Context* ctx = (Context*)context;
	BOOL ok = HeapDestroy(ctx->heap);
	assert(ok);
}

bool
cdt_saveBin(const wchar_t* path, const CdtEncContext context) {
	Context* ctx = (Context*)context;
	if (!ctx->binLen) return false;
	
	HANDLE f = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE) return false;
	
	bool result = false;
	DWORD cb;
	BOOL ok = WriteFile(f, ctx->bin, (DWORD)ctx->binLen, &cb, NULL);
	if (!ok || cb != (DWORD)ctx->binLen) goto cleanup;

	const BYTE nul = 0;
	ok = WriteFile(f, &nul, 1, &cb, NULL);
	result = ok && cb == 1;
	
cleanup:;
	CloseHandle(f);
	return result;
}
