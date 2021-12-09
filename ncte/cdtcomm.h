#pragma once

#include <Windows.h>

#include <stdbool.h>
#include <stdint.h>


enum {
	cdt_kEncodingIndex88591,
	cdt_kEncodingIndexAscii,
	cdt_kEncodingIndexMsjis,
};

enum {
	cdt_kCpMsjis = 60932, // customed codepage, use a number hopefully not taken already
	cdt_kCpUtf16be = 1201,
	cdt_kCp88591 = 28591,
	cdt_kCpAscii = 20127,
	cdt_kCpUtf8 = 65001,
};

// enum of completeness values
enum {
	cdt_kNone,
	cdt_kBlockHasAnyTitle = 0x01,
	cdt_kBlockHasAllTitles = 0x03,
};

enum {
	cdt_kCchUpc = 13,
	cdt_kCchEan = cdt_kCchUpc,
	cdt_kCchIsrc = 12,
	cdt_kCchToc = 8,
};

// Used as bool
enum {
	cdt_kSingleByte,
	cdt_kDoubleByte,
};

enum {
	cdt_kHeaderSize = 4,
	cdt_kPayloadSize = 12,
	cdt_kPayloadWcsSize = 6,
	cdt_kCrcSize = 2,
	cdt_kPackSize = 18,
	cdt_kSummarySize = cdt_kPackSize * 3,

	cdt_kTypeCountMax = 16,
	cdt_kBlockCountMax = 8,
	cdt_kBlockSizeMin = 4 * cdt_kPackSize, // 1 pack of 0x80, 3 packs of 0x8f, totally 4 packs
	cdt_kBlockSizeMax = 256 * cdt_kPackSize,
	cdt_kBinSizeMax = cdt_kBlockSizeMax * cdt_kBlockCountMax,

	//cdt_kCchDefaultBuffer = 168, // Suggested soft limit is 160+1(NUL), Let's use 168 though.
	cdt_kCchMaxStrBuffer = 253 * cdt_kPayloadSize, // Let's not bother to minus 1 which taken by track 1.
	cdt_kCchMaxWcsBuffer = cdt_kCchMaxStrBuffer / 2,
};

typedef struct cdt_Album {
	bool copyProtected; // 0x8F
	int encoding; // 0x8F. It's index rather than code or codepage
	int language; // 0x8F. It's index rather than code
	wchar_t* title;  // 0x80. Specified encoding. Mandatory.
	wchar_t* artist; // 0x81. Specified encoding. Should not be ommitted except for an omnibus compilation album
	wchar_t* songwriter; // 0x82. Specified encoding. Optional
	wchar_t* composer; // 0x83. Specified encoding. Optional
	wchar_t* arranger; // 0x84. Specified encoding. Optional
	wchar_t* message; // 0x85. Specified encoding. Optional
	wchar_t* catalog; // 0x86. ASCII. Optional
	int genreCode; // 0x87. Optional. It's index rather than code
	wchar_t* genreExtra; // 0x87. ASCII, Optional
	wchar_t* closedInfo; // 0x8D. 8859-1, Optional

	union {
		wchar_t* upc; // 0x8E. ASCII, Optional. UPC/EAN
		wchar_t* ean;
	};
}cdt_Album;

typedef struct cdt_Track {
	wchar_t* title;  // 0x80. Specified encoding. Mandatory.
	wchar_t* artist; // 0x81. Specified encoding. Optional
	wchar_t* songwriter; // 0x82, Specified encoding. Optional
	wchar_t* composer; // 0x83, Specified encoding. Optional
	wchar_t* arranger; // 0,84, Specified encoding. Optional
	wchar_t* message; // 0x85. Specified encoding. Optional
	wchar_t* isrc; // 0x8E. ASCII. Optional
}cdt_Track;

typedef struct cdt_Block {
	HANDLE heap;
	cdt_Album* album;
	cdt_Track* tracks;
	int trackFirst;
	int trackCount;
}cdt_Block;

typedef struct cdt_Time {
	int min;
	int sec;
	int frame;
}cdt_Time;

typedef struct Cdt {
	uint8_t trackFirst;
	uint8_t trackLast;
	cdt_Block blocks[cdt_kBlockCountMax];
	cdt_Time* toc; // (trackCount + 1) items in total, toc[0] is the start time of track1, toc[trackCount] is the total playing time
}Cdt;


typedef struct cdt_PackHeader {
	union {
		uint8_t h1;
		uint8_t type;
	};
	union {
		uint8_t h2;
		uint8_t trackNumber;
	};
	union {
		uint8_t h3;
		uint8_t packNumber;
	};
	union {
		uint8_t h4;
		struct {
			uint8_t prevCharCount : 4;
			uint8_t blockNumber : 3;
			uint8_t isMultiByte : 1;
		};
	};
}cdt_PackHeader;
static_assert(sizeof(cdt_PackHeader) == 4, "PackHeader should be 4 in size");

typedef struct cdt_Pack {
	cdt_PackHeader;
	union {
		uint8_t    payload[12];
		wchar_t payloadWcs[6];
	};
	uint16_t crc;
}cdt_Pack;
static_assert(sizeof(cdt_Pack) == 18, "Pack should be 18 in size");

typedef struct cdt_CodePair {
	uint16_t code;
	uint16_t name;
}cdt_CodePair;

const cdt_CodePair cdt_kEncodings[];
const cdt_CodePair cdt_kLanguages[];
const cdt_CodePair cdt_kGenres[];

const size_t cdt_kEncodingCount;
const size_t cdt_kLanguageCount;
const size_t cdt_kGenreCount;

const uint32_t cdt_kCodepages[];


static inline int
cdt_getEncodingIndex(uint8_t code) {
	for (int i = 0; i < (int)cdt_kEncodingCount; ++i) {
		if ((uint8_t)cdt_kEncodings[i].code == code) return i;
	}
	return -1;
}

static inline int
cdt_getLanguageIndex(uint8_t code) {
	// Small count of elements, no need for binary search
	for (int i = 0; i < (int)cdt_kLanguageCount; ++i) {
		if ((uint8_t)cdt_kLanguages[i].code == code) return i;
	}
	return -1;
}

static inline int
cdt_getGenreIndex(uint16_t code) {
	for (int i = 0; i < (int)cdt_kGenreCount; ++i) {
		if (cdt_kGenres[i].code == code) return i;
	}
	return -1;
}


static inline bool
cdt_timeIsEmpty(const cdt_Time* time) {
	if (time->min) return false;
	if (time->sec) return false;
	if (time->frame) return false;
	return true;
}

static inline bool
cdt_timeIsValid(const cdt_Time* time) {
	if (time->min < 0 || time->min > 83) return false;
	if (time->sec < 0 || time->sec > 59) return false;
	if (time->frame < 0 || time->frame > 74) return false;
	return true;
}

static inline bool
cdt_timeIsValidNonZero(const cdt_Time* time) {
	if (cdt_timeIsEmpty) return false;
	return cdt_timeIsValid(time);
}

// If a > b, return a positive number. If a < b, return a negative number. if a == b, return 0.
static inline int
cdt_timeCompare(const cdt_Time* a, const cdt_Time* b) {
	int r = a->min - b->min;
	if (r) return r;
	r = a->sec - b->sec;
	if (r) return r;
	return a->frame - b->frame;
}

static inline bool
cdt_tocIsEmpty(const cdt_Time* toc, int trackCount) {
	for (int i = 0; i <= trackCount; ++i) {
		if (!cdt_timeIsEmpty(toc + i)) return false;
	}
	return true;
}

static inline bool
cdt_tocIsValid(const cdt_Time* toc, int trackCount) {
	if (!cdt_timeIsValid(toc)) return false;
	for (int i = 0; i < trackCount; ++i) {
		const cdt_Time* a = toc + i;
		const cdt_Time* b = toc + i + 1;
		if (!cdt_timeIsValid(b)) return false;
		if (cdt_timeCompare(a, b) >= 0) return false;
	}
	return true;
}

static inline bool
cdt_tocIsValidNonZero(const cdt_Time* toc, int trackCount) {
	if (cdt_tocIsEmpty(toc, trackCount)) return false;
	return cdt_tocIsValid(toc, trackCount);
}
