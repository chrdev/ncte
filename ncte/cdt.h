#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "cdtcomm.h"
#include "str.h"

extern Cdt theCdt;

static inline const wchar_t*
cdt_getEncodingText(int index, int* len) {
	return wcs_loadf(len, cdt_kEncodings[index].name);
}

static inline const wchar_t*
cdt_getLanguageText(int index, int* len) {
	return wcs_loadf(len, cdt_kLanguages[index].name, cdt_kLanguages[index].code);
}

static inline const wchar_t*
cdt_getGenreText(int index, int* len) {
	return wcs_loadf(len, cdt_kGenres[index].name, cdt_kGenres[index].code);
}

// Caution: return pointer to static buffer
const wchar_t*
cdt_timeToText(const cdt_Time* time);

bool
cdt_timeFromText(cdt_Time* time, const wchar_t* text);

bool
cdt_timeFromCtl(cdt_Time* time, HWND ctl);

bool
cdt_init(Cdt* cdt);

void
cdt_uninit(Cdt* cdt);

static inline bool
cdt_reset(Cdt* cdt) {
	cdt_uninit(cdt);
	return cdt_init(cdt);
}

bool
cdt_setTrackRange(Cdt* cdt, int first, int last);

bool
cdt_reserveBlock(Cdt* cdt, int index);

bool
cdt_clearBlock(Cdt* cdt, int index);

bool
cdt_setTextFromCtlUsingEncoding(cdt_Block* block, wchar_t** field, HWND ctl, int encodingIndex);

static inline bool
cdt_setTextFromCtl(cdt_Block* block, wchar_t** field, HWND ctl) {
	return cdt_setTextFromCtlUsingEncoding(block, field, ctl, block->album->encoding);
}

bool
cdt_setTextUsingCodepage(cdt_Block* block, wchar_t** field, const char* text, int len, UINT codepage);

bool
cdt_setText(cdt_Block* block, wchar_t** field, const wchar_t* text, int len);

static inline int
cdt_getTrackCount(const Cdt* cdt) {
	int result = cdt->trackLast - cdt->trackFirst + 1;
	assert(result > 0);
	return result;
}

static inline int
cdt_getBlockCount(const Cdt* cdt) {
	int result = 0;
	for (int i = 0; i < cdt_kBlockCountMax; ++i) {
		if (cdt->blocks[i].heap) ++result;
	}
	return result;
}

// If album title and all tracks title exist, return true. Otherwise return false.
static inline bool
cdt_blockHasFullTitle(const cdt_Block* block) {
	if (wcs_isEmpty(block->album->title)) return false;
	for (int i = 0; i < block->trackCount; ++i) {
		if (wcs_isEmpty(block->tracks[i].title)) return false;
	}
	return true;
}

static inline bool
cdt_blockHasAlbumTitle(const cdt_Block* block) {
	return !wcs_isEmpty(block->album->title);
}

static inline bool
cdt_blockHasTrackTitle(const cdt_Block* block) {
	for (int i = 0; i < block->trackCount; ++i) {
		if (!wcs_isEmpty(block->tracks[i].title)) return true;
	}
	return false;
}

static inline bool
cdt_blockHasAnyTitle(const cdt_Block* block) {
	if (cdt_blockHasAlbumTitle(block)) return true;
	return cdt_blockHasTrackTitle(block);
}

// Return one of cdt_kNone, cdt_kBlockHasAnyTitle, cdt_kBlockHasAllTitles
static inline int
cdt_blockGetCompleteness(const cdt_Block* block) {
	if (!block->heap) return cdt_kNone;

	int count = 0;
	if (!wcs_isEmpty(block->album->title)) ++count;
	for (int i = 0; i < block->trackCount; ++i) {
		if (!wcs_isEmpty(block->tracks[i].title)) ++count;
	}
	if (!count) return cdt_kNone;
	else if (count == block->trackCount + 1) return cdt_kBlockHasAllTitles;
	return cdt_kBlockHasAnyTitle;
}

static inline bool
cdt_blockHasAlbumArtist(const cdt_Block* block) {
	return !wcs_isEmpty(block->album->artist);
}

static inline bool
cdt_blockHasTrackArtist(const cdt_Block* block) {
	for (int i = 0; i < block->trackCount; ++i) {
		if (!wcs_isEmpty(block->tracks[i].artist)) return true;
	}
	return false;
}

static inline bool
cdt_blockHasAnyArtist(const cdt_Block* block) {
	if (cdt_blockHasAlbumArtist) return true;
	return cdt_blockHasTrackArtist(block);
}

static inline bool
cdt_blockHasAlbumSongwriter(const cdt_Block* block) {
	return !wcs_isEmpty(block->album->songwriter);
}

static inline bool
cdt_blockHasTrackSongwriter(const cdt_Block* block) {
	for (int i = 0; i < block->trackCount; ++i) {
		if (!wcs_isEmpty(block->tracks[i].songwriter)) return true;
	}
	return false;
}

static inline bool
cdt_blockHasAnySongwriter(const cdt_Block* block) {
	if (cdt_blockHasAlbumSongwriter(block)) return true;
	return cdt_blockHasTrackSongwriter(block);
}

static inline bool
cdt_blockHasAlbumComposer(const cdt_Block* block) {
	return !wcs_isEmpty(block->album->composer);
}

static inline bool
cdt_blockHasTrackComposer(const cdt_Block* block) {
	for (int i = 0; i < block->trackCount; ++i) {
		if (!wcs_isEmpty(block->tracks[i].composer)) return true;
	}
	return false;
}

static inline bool
cdt_blockHasAnyComposer(const cdt_Block* block) {
	if (cdt_blockHasAlbumComposer(block)) return true;
	return cdt_blockHasTrackComposer(block);
}

static inline bool
cdt_blockHasAlbumArranger(const cdt_Block* block) {
	return !wcs_isEmpty(block->album->arranger);
}

static inline bool
cdt_blockHasTrackArranger(const cdt_Block* block) {
	for (int i = 0; i < block->trackCount; ++i) {
		if (!wcs_isEmpty(block->tracks[i].arranger)) return true;
	}
	return false;
}

static inline bool
cdt_blockHasAnyArranger(const cdt_Block* block) {
	if (cdt_blockHasAlbumArranger(block)) return true;
	return cdt_blockHasTrackArranger(block);
}

static inline bool
cdt_blockHasAlbumMessage(const cdt_Block* block) {
	return !wcs_isEmpty(block->album->message);
}

static inline bool
cdt_blockHasTrackMessage(const cdt_Block* block) {
	for (int i = 0; i < block->trackCount; ++i) {
		if (!wcs_isEmpty(block->tracks[i].message)) return true;
	}
	return false;
}

static inline bool
cdt_blockHasAnyMessage(const cdt_Block* block) {
	if (cdt_blockHasAlbumMessage(block)) return true;
	return cdt_blockHasTrackMessage(block);
}

static inline bool
cdt_blockHasAlbumUpc(const cdt_Block* block) {
	return !wcs_isEmpty(block->album->upc);
}

static inline bool
cdt_blockHasAlbumEan(const cdt_Block* block) {
	return cdt_blockHasAlbumUpc(block);
}

static inline bool
cdt_blockHasTrackIsrc(const cdt_Block* block) {
	for (int i = 0; i < block->trackCount; ++i) {
		if (!wcs_isEmpty(block->tracks[i].isrc)) return true;
	}
	return false;
}

static inline bool
cdt_blockHasAnyCode(const cdt_Block* block) {
	if (cdt_blockHasAlbumUpc(block)) return true;
	return cdt_blockHasTrackIsrc(block);
}

static inline bool
cdt_blockHasAlbumCatalog(const cdt_Block* block) {
	return !wcs_isEmpty(block->album->catalog);
}

static inline bool
cdt_blockHasAlbumClosedInfo(const cdt_Block* block) {
	return !wcs_isEmpty(block->album->closedInfo);
}

bool
cdt_blockIsEmpty(const cdt_Block* block);

static inline void
swapBlock(cdt_Block* a, cdt_Block* b) {
	cdt_Block t = *a;
	*a = *b;
	*b = t;
}

// Return the new index
static inline int
cdt_blockShiftUp(Cdt* cdt, int index) {
	if (index <= 0 || index >= cdt_kBlockCountMax) return index;
	int target = index - 1;
	for (int i = target; i >= 0; --i) {
		if (!cdt_blockIsEmpty(cdt->blocks + i)) continue;
		target = i;
		break;
	}
	swapBlock(cdt->blocks + index, cdt->blocks + target);
	return target;
}

// Return the new index
static inline int
cdt_blockShiftDown(Cdt* cdt, int index) {
	if (index >= cdt_kBlockCountMax - 1 || index < 0) return index;
	int target = index + 1;
	for (int i = target; i < cdt_kBlockCountMax; ++i) {
		if (!cdt_blockIsEmpty(cdt->blocks + i)) continue;
		target = i;
		break;
	}
	swapBlock(cdt->blocks + index, cdt->blocks + target);
	return target;
}

static inline int
cdt_getFirstValidBlockIndex(const Cdt* cdt) {
	for (int i = 0; i < cdt_kBlockCountMax; ++i) {
		if (cdt->blocks[i].heap) return i;
	}
	return -1;
}

static inline bool
cdt_canSave(const Cdt* cdt) {
	int count = 0;
	for (int i = 0; i < cdt_kBlockCountMax; ++i) {
		if (!cdt->blocks[i].heap) continue;
		switch (cdt_blockGetCompleteness(cdt->blocks + i)) {
		case cdt_kNone:
			continue;
		case cdt_kBlockHasAnyTitle:
			return false;
		case cdt_kBlockHasAllTitles:
			++count;
			break;
		}
	}
	return count > 0;
}

// All or some block of src will be moved to dst.
// Anchor is the index of block from where to overwrite dst.
// Return the count of moved blocks
int
cdt_transfer(Cdt* dst, int dstIndex, Cdt* src);

int
cdt_blockRegulate(cdt_Block* block);

bool
cdt_copyBlock(Cdt* cdt, int srcIndex, int dstIndex);

BYTE
cdt_getAlbumFields(const Cdt* cdt);

static inline BYTE
cdt_getAlbumHiddenFields(const Cdt* cdt, BYTE visibleFields) {
	BYTE filledFields = cdt_getAlbumFields(cdt);
	return (visibleFields ^ filledFields) & filledFields;
}

BYTE
cdt_getTrackFields(const Cdt* cdt);

static inline BYTE
cdt_getTrackHiddenFields(const Cdt* cdt, BYTE visibleFields) {
	BYTE filledFields = cdt_getTrackFields(cdt);
	return (visibleFields ^ filledFields) & filledFields;
}

typedef BYTE(*cdt_getFieldsFunc)(const Cdt* cdt);

typedef BYTE(*cdt_getHiddenFieldsFunc)(const Cdt* cdt, BYTE visibleFields);

static inline bool
cdt_hasAnyToc(const Cdt* cdt) {
	const trackCount = cdt->trackLast - cdt->trackFirst + 1;
	return cdt_tocIsValidNonZero(cdt->toc, trackCount);
}

