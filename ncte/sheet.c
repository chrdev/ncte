#include "sheet.h"

#include <assert.h>

#include "str.h"


typedef struct CodeNamePair {
	uint16_t code;
	const char* name;
	const wchar_t* wcsName;
}CodeNamePair;

static const CodeNamePair kSheetEncodings[] = {
	{ 0x00, "8859",   L"8859"  },
	{ 0x01, "ASCII",  L"ASCII" },
	{ 0x80, "MS-JIS", L"MS-JIS"},
};

// kSheetEncodings buddy
static const UINT kCodepages[] = {
	cdt_kCp88591,
	cdt_kCpAscii,
	cdt_kCpMsjis,
};

static const CodeNamePair kSheetLanguages[] = {
	{ 0x00, "Not Used",   L"Not Used"  },
	{ 0x09, "English",    L"English"   },
	{ 0x08, "German",     L"German"    },
	{ 0x0f, "French",     L"French"    },
	{ 0x0a, "Spanish",    L"Spanish"   },
	{ 0x15, "Italian",    L"Italian"   },
	{ 0x69, "Japanese",   L"Japanese"  },
	{ 0x65, "Korean",     L"Korean"    },
	{ 0x75, "Chinese",    L"Chinese"   },
	{ 0x21, "Portuguese", L"Portuguese"},
	{ 0x1d, "Dutch",      L"Dutch"     },
	{ 0x07, "Danish",     L"Danish"    },
	{ 0x28, "Swedish",    L"Swedish"   },
	{ 0x1e, "Norwegian",  L"Norwegian" },
	{ 0x27, "Finnish",    L"Finnish"   },
	{ 0x20, "Polish",     L"Polish"    },
	{ 0x56, "Russian",    L"Russian"   },
	{ 0x26, "Slovene",    L"Slovene"   },
	{ 0x1b, "Hungarian",  L"Hungarian" },
	{ 0x06, "Czech",      L"Czech"     },
	{ 0x70, "Greek",      L"Greek"     },
	{ 0x29, "Turkish",    L"Turkish"   },
};

static const CodeNamePair kSheetGenres[] = {
	{ 0x0000, "Not Used",               L"Not Used"              },
	{ 0x0001, "Not Defined",            L"Not Defined"           },
	{ 0x0002, "Adult Contemporary",     L"Adult Contemporary"    },
	{ 0x0003, "Alternative Rock",       L"Alternative Rock"      },
	{ 0x0004, "Childrens Music",        L"Childrens Music"       },
	{ 0x0005, "Classical",              L"Classical"             },
	{ 0x0006, "Contemporary Christian", L"Contemporary Christian"},
	{ 0x0007, "Country",                L"Country"               },
	{ 0x0008, "Dance",                  L"Dance"                 },
	{ 0x0009, "Easy Listening",         L"Easy Listening"        },
	{ 0x000a, "Erotic",                 L"Erotic"                },
	{ 0x000b, "Folk",                   L"Folk"                  },
	{ 0x000c, "Gospel",                 L"Gospel"                },
	{ 0x000d, "Hip Hop",                L"Hip Hop"               },
	{ 0x000e, "Jazz",                   L"Jazz"                  },
	{ 0x000f, "Latin",                  L"Latin"                 },
	{ 0x0010, "Musical",                L"Musical"               },
	{ 0x0011, "New Age",                L"New Age"               },
	{ 0x0012, "Opera",                  L"Opera"                 },
	{ 0x0013, "Operetta",               L"Operetta"              },
	{ 0x0014, "Pop Music",              L"Pop Music"             },
	{ 0x0015, "Rap",                    L"Rap"                   },
	{ 0x0016, "Reggae",                 L"Reggae"                },
	{ 0x0017, "Rock Music",             L"Rock Music"            },
	{ 0x0018, "Rhythm & Blues",         L"Rhythm & Blues"        },
	{ 0x0019, "Sound Effects",          L"Sound Effects"         },
	{ 0x001a, "Sound Track",            L"Sound Track"           },
	{ 0x001b, "Spoken Word",            L"Spoken Word"           },
	{ 0x001c, "World Music",            L"World Music"           },
};

static inline int
getIndexByCode(uint16_t code, const CodeNamePair* list, int count) {
	for (int i = 0; i < count; ++i) {
		if (list[i].code == code) return i;
	}
	return -1;
}

static inline int
getIndexByName(const char* name, int len, const CodeNamePair* list, int count) {
	for (int i = 0; i < count; ++i) {
		if (!_strnicmp(name, list[i].name, len)) return i;
	}
	return -1;
}

static const char* kFields[] = {
	"Album Message",
	"Album Title",
	"Arranger",
	"Artist Name",
	"Catalog Number",
	"Closed Information",
	"Composer",
	"First Track Number",
	"Genre Code",
	"Genre Information",
	"Language Code",
	"Last Track Number",
	"Songwriter",
	"Text Code",
	"Text Data Copy Protection",
	"UPC / EAN"
};

// kFields buddy
enum {
	kAlbumMessage,
	kAlbumTitle,
	kArranger,
	kArtistName,
	kCatalogNumber,
	kClosedInformation,
	kComposer,
	kFirstTrackNumber,
	kGenreCode,
	kGenreInformation,
	kLanguageCode,
	kLastTrackNumber,
	kSongwriter,
	kTextCode,
	kTextDataCopyProtection,
	kUpcEan,
	kFieldCount
};
static_assert(ARRAYSIZE(kFields) == kFieldCount, "Count of kFields should match kFieldCount.");

static const char kKeyIsrc[]  = "ISRC ";
static const char kKeyTrack[] = "Track ";

static const char* kTrackFields[] = {
	"Arranger",
	"Artist",
	"Composer",
	"Message",
	"Songwriter",
	"Title"
};

// kTrackFields buddy
enum {
	kTrackArranger,
	kTrackArtist,
	kTrackComposer,
	kTrackMessage,
	kTrackSongwriter,
	kTrackTitle,
	kTrackFieldCount,
};
static_assert(ARRAYSIZE(kTrackFields) == kTrackFieldCount, "Count of kTrackFields should match kTrackFieldCount.");

static inline bool
getIndexFromNameOrCode(int* result, const char* value, int len, const CodeNamePair* list, int count) {
	int index = getIndexByName(value, len, list, count);
	if (index >= 0) {
		*result = index;
		return true;
	}
	int n;
	int c = str_toNumberAsDec(&n, value, 3);
	if (c < 0) return false;
	index = getIndexByCode(n, list, count);
	if (index >= 0) {
		*result = index;
		return true;
	}
	return false;
}

static inline bool
parseTextCode(int* result, const char* value, int len) {
	return getIndexFromNameOrCode(result, value, len, kSheetEncodings, ARRAYSIZE(kSheetEncodings));
}

static inline bool
parseTrackNumber(uint8_t* result, const char* value, int len) {
	int n;
	if (str_toNumberAsDec(&n, value, 2) < 1) return false;
	if (n <= 0 || n > 99) return false;

	*result = n;
	return true;
}

static inline bool
parseTrack(cdt_Block* block, int trackFirst, UINT codepage, const char* key, int keyLen, const char* value, int valueLen) {
	cdt_Track* tracks = block->tracks;
	key += sizeof(kKeyTrack) - 1;
	keyLen -= sizeof(kKeyTrack) - 1;
	int trackNumber;
	int digits = str_toNumberAsDec(&trackNumber, key, 2);
	if (digits <= 0) return false;
	if (trackNumber <= 0 || trackNumber > 99) return false;

	key += digits + 1;
	keyLen -= digits + 1;
	wchar_t** text;
	switch (str_getIndexInList(key, keyLen, kTrackFields, kTrackFieldCount)) {
	case -1:
		return false;
	case kTrackTitle:
		text = &tracks[trackNumber - trackFirst].title;
		break;
	case kTrackArtist:
		text = &tracks[trackNumber - trackFirst].artist;
		break;
	case kTrackSongwriter:
		text = &tracks[trackNumber - trackFirst].songwriter;
		break;
	case kTrackComposer:
		text = &tracks[trackNumber - trackFirst].composer;
		break;
	case kTrackArranger:
		text = &tracks[trackNumber - trackFirst].arranger;
		break;
	case kTrackMessage:
		text = &tracks[trackNumber - trackFirst].message;
		break;
	default:
		assert(false);
		return false;
		break;
	}

	if (!cdt_setTextUsingCodepage(block, text, value, valueLen, codepage)) return false;
	return true;
}

static inline bool
parseIsrc(cdt_Block* block, int trackFirst, const char* key, int keyLen, const char* value, int valueLen) {
	cdt_Track* tracks = block->tracks;
	key += sizeof(kKeyIsrc) - 1;
	keyLen -= sizeof(kKeyIsrc) - 1;
	int trackNumber;
	int digits = str_toNumberAsDec(&trackNumber, key, 2);
	if (digits <= 0) return false;
	if (trackNumber <= 0 || trackNumber > 99) return false;
	wchar_t** text = &tracks[trackNumber - trackFirst].isrc;
	if (!cdt_setTextUsingCodepage(block, text, value, valueLen, cdt_kCpAscii)) return false;
	return true;
}

static inline bool
parseGenreCode(int* result, const char* value, int len) {
	return getIndexFromNameOrCode(result, value, len, kSheetGenres, ARRAYSIZE(kSheetGenres));
}

static inline bool
parseLanguageCode(int* result, const char* value, int len) {
	return getIndexFromNameOrCode(result, value, len, kSheetLanguages, ARRAYSIZE(kSheetLanguages));
}

static inline bool
parseCopyProtection(bool* result, const char* value, int len) {
	int n;
	if (!_strnicmp(value, "ON", len)) {
		*result = true;
	}
	else {
		int c = str_toNumberAsHex(&n, value, 4);
		*result = c > 0 && n > 0;
	}
	return true;
}

bool
sheet_read(Sheet sheet, Cdt* cdt) {
	const char* key;
	const char* value;
	int keyLen;
	int valueLen;

	int blockIndex = -1;
	cdt_Block* block = NULL;
	uint8_t trackFirst = 0, trackLast = 0;
	for (bool ok = cfgfile_read(sheet, &key, &keyLen, &value, &valueLen); ok; ok = cfgfile_read(sheet, &key, &keyLen, &value, &valueLen)) {
		if (!_strnicmp(key, kKeyTrack, sizeof(kKeyTrack) - 1)) {
			if (!block) return false;
			if (!parseTrack(block, cdt->trackFirst, cfgfile_getCodePage(sheet), key, keyLen, value, valueLen)) return false;
			continue;
		}
		else if (!_strnicmp(key, kKeyIsrc, sizeof(kKeyIsrc) - 1)) {
			if (!block) return false;
			if (!parseIsrc(block, cdt->trackFirst, key, keyLen, value, valueLen)) return false;
			continue;
		}
		switch (str_getIndexInList(key, keyLen, kFields, kFieldCount)) {
		case -1:
			break;
		case kTextCode:
			if (++blockIndex >= cdt_kBlockCountMax) return false;
			block = cdt->blocks + blockIndex;
			if (!parseTextCode(&block->album->encoding, value, valueLen)) return false;
			if (cdt_kCpUtf8 != cfgfile_getCodePage(sheet)) cfgfile_setCodepage(sheet, kCodepages[block->album->encoding]);
			break;
		case kFirstTrackNumber:
			if (!block) return false;
			if (!parseTrackNumber(&trackFirst, value, valueLen)) return false;
			break;
		case kLastTrackNumber:
			if (!block) return false;
			if (!parseTrackNumber(&trackLast, value, valueLen)) return false;
			if (!cdt_setTrackRange(cdt, trackFirst, trackLast)) return false;
			break;
		case kAlbumMessage:
			if (!block) return false;
			if (!cdt_setTextUsingCodepage(block, &block->album->message, value, valueLen, cfgfile_getCodePage(sheet))) return false;
			break;
		case kAlbumTitle:
			if (!block) return false;
			if (!cdt_setTextUsingCodepage(block, &block->album->title, value, valueLen, cfgfile_getCodePage(sheet))) return false;
			break;
		case kArranger:
			if (!block) return false;
			if (!cdt_setTextUsingCodepage(block, &block->album->arranger, value, valueLen, cfgfile_getCodePage(sheet))) return false;
			break;
		case kArtistName:
			if (!block) return false;
			if (!cdt_setTextUsingCodepage(block, &block->album->artist, value, valueLen, cfgfile_getCodePage(sheet))) return false;
			break;
		case kCatalogNumber:
			if (!block) return false;
			if (!cdt_setTextUsingCodepage(block, &block->album->catalog, value, valueLen, cdt_kCpAscii)) return false;
			break;
		case kClosedInformation:
			if (!block) return false;
			if (!cdt_setTextUsingCodepage(block, &block->album->closedInfo, value, valueLen, cdt_kCp88591)) return false;
			break;
		case kComposer:
			if (!block) return false;
			if (!cdt_setTextUsingCodepage(block, &block->album->composer, value, valueLen, cfgfile_getCodePage(sheet))) return false;
			break;
		case kGenreCode:
			if (!block) return false;
			if (!parseGenreCode(&block->album->genreCode, value, valueLen)) return false;
			break;
		case kGenreInformation:
			if (!block) return false;
			if (!cdt_setTextUsingCodepage(block, &block->album->genreExtra, value, valueLen, cdt_kCpAscii)) return false;
			break;
		case kLanguageCode:
			if (!block) return false;
			if (!parseLanguageCode(&block->album->language, value, valueLen)) return false;
			break;
		case kSongwriter:
			if (!block) return false;
			if (!cdt_setTextUsingCodepage(block, &block->album->songwriter, value, valueLen, cfgfile_getCodePage(sheet))) return false;
			break;
		case kTextDataCopyProtection:
			if (!block) return false;
			if (!parseCopyProtection(&block->album->copyProtected, value, valueLen)) return false;
			break;
		case kUpcEan:
			if (!block) return false;
			if (!cdt_setTextUsingCodepage(block, &block->album->upc, value, valueLen, cdt_kCpAscii)) return false;
			break;
		default:
			assert(false);
			return false;
			break;
		}
	}
	return true;
}


static inline int
getIndexByNameWcs(const wchar_t* name, int len, const CodeNamePair* list, int count) {
	for (int i = 0; i < count; ++i) {
		if (!_wcsnicmp(name, list[i].wcsName, len)) return i;
	}
	return -1;
}

static const wchar_t* kWcsFields[] = {
	L"Album Message",
	L"Album Title",
	L"Arranger",
	L"Artist Name",
	L"Catalog Number",
	L"Closed Information",
	L"Composer",
	L"First Track Number",
	L"Genre Code",
	L"Genre Information",
	L"Language Code",
	L"Last Track Number",
	L"Songwriter",
	L"Text Code",
	L"Text Data Copy Protection",
	L"UPC / EAN"
};
static_assert(ARRAYSIZE(kWcsFields) == kFieldCount, "Count of kWcsFields should match kFieldCount.");


static const wchar_t kWcsKeyIsrc[] = L"ISRC ";
static const wchar_t kWcsKeyTrack[] = L"Track ";

static const wchar_t* kWcsTrackFields[] = {
	L"Arranger",
	L"Artist",
	L"Composer",
	L"Message",
	L"Songwriter",
	L"Title"
};
static_assert(ARRAYSIZE(kWcsTrackFields) == kTrackFieldCount, "Count of kWcsTrackFields should match kTrackFieldCount.");

static inline bool
getIndexFromNameOrCodeWcs(int* result, const wchar_t* value, int len, const CodeNamePair* list, int count) {
	int index = getIndexByNameWcs(value, len, list, count);
	if (index >= 0) {
		*result = index;
		return true;
	}
	int n;
	int c = wcs_toNumberAsDec(&n, value, 3);
	if (c < 0) return false;
	index = getIndexByCode(n, list, count);
	if (index >= 0) {
		*result = index;
		return true;
	}
	return false;
}

static inline bool
parseTextCodeWcs(int* result, const wchar_t* value, int len) {
	return getIndexFromNameOrCodeWcs(result, value, len, kSheetEncodings, ARRAYSIZE(kSheetEncodings));
}

static inline bool
parseTrackNumberWcs(uint8_t* result, const wchar_t* value, int len) {
	int n;
	if (wcs_toNumberAsDec(&n, value, 2) < 1) return false;
	if (n <= 0 || n > 99) return false;

	*result = n;
	return true;
}

static inline bool
parseTrackWcs(cdt_Block* block, int trackFirst, const wchar_t* key, int keyLen, const wchar_t* value, int valueLen) {
	cdt_Track* tracks = block->tracks;
	key += ARRAYSIZE(kWcsKeyTrack) - 1;
	keyLen -= ARRAYSIZE(kWcsKeyTrack) - 1;
	int trackNumber;
	int digits = wcs_toNumberAsDec(&trackNumber, key, 2);
	if (digits <= 0) return false;
	if (trackNumber <= 0 || trackNumber > 99) return false;

	key += digits + 1;
	keyLen -= digits + 1;
	wchar_t** text;
	switch (wcs_getIndexInList(key, keyLen, kWcsTrackFields, kTrackFieldCount)) {
	case -1:
		return false;
	case kTrackTitle:
		text = &tracks[trackNumber - trackFirst].title;
		break;
	case kTrackArtist:
		text = &tracks[trackNumber - trackFirst].artist;
		break;
	case kTrackSongwriter:
		text = &tracks[trackNumber - trackFirst].songwriter;
		break;
	case kTrackComposer:
		text = &tracks[trackNumber - trackFirst].composer;
		break;
	case kTrackArranger:
		text = &tracks[trackNumber - trackFirst].arranger;
		break;
	case kTrackMessage:
		text = &tracks[trackNumber - trackFirst].message;
		break;
	default:
		assert(false);
		return false;
		break;
	}

	if (!cdt_setText(block, text, value, valueLen)) return false;
	return true;
}

static inline bool
parseIsrcWcs(cdt_Block* block, int trackFirst, const wchar_t* key, int keyLen, const wchar_t* value, int valueLen) {
	cdt_Track* tracks = block->tracks;
	key += ARRAYSIZE(kWcsKeyIsrc) - 1;
	keyLen -= ARRAYSIZE(kWcsKeyIsrc) - 1;
	int trackNumber;
	int digits = wcs_toNumberAsDec(&trackNumber, key, 2);
	if (digits <= 0) return false;
	if (trackNumber <= 0 || trackNumber > 99) return false;
	wchar_t** text = &tracks[trackNumber - trackFirst].isrc;
	if (!cdt_setText(block, text, value, valueLen)) return false;
	return true;
}

static inline bool
parseGenreCodeWcs(int* result, const wchar_t* value, int len) {
	return getIndexFromNameOrCodeWcs(result, value, len, kSheetGenres, ARRAYSIZE(kSheetGenres));
}

static inline bool
parseLanguageCodeWcs(int* result, const wchar_t* value, int len) {
	return getIndexFromNameOrCodeWcs(result, value, len, kSheetLanguages, ARRAYSIZE(kSheetLanguages));
}

static inline bool
parseCopyProtectionWcs(bool* result, const wchar_t* value, int len) {
	int n;
	if (!_wcsnicmp(value, L"ON", len)) {
		*result = true;
	}
	else {
		int c = wcs_toNumberAsHex(&n, value, 4);
		*result = c > 0 && n > 0;
	}
	return true;
}

bool
sheet_readWcs(Sheet sheet, Cdt* cdt) {
	const wchar_t* key;
	const wchar_t* value;
	int keyLen;
	int valueLen;

	int blockIndex = -1;
	cdt_Block* block = NULL;
	uint8_t trackFirst = 0, trackLast = 0;
	for (bool ok = cfgfile_readWcs(sheet, &key, &keyLen, &value, &valueLen); ok; ok = cfgfile_readWcs(sheet, &key, &keyLen, &value, &valueLen)) {
		if (!_wcsnicmp(key, kWcsKeyTrack, ARRAYSIZE(kWcsKeyTrack) - 1)) {
			if (!block) return false;
			if (!parseTrackWcs(block, cdt->trackFirst, key, keyLen, value, valueLen)) return false;
			continue;
		}
		else if (!_wcsnicmp(key, kWcsKeyIsrc, ARRAYSIZE(kWcsKeyIsrc) - 1)) {
			if (!block) return false;
			if (!parseIsrcWcs(block, cdt->trackFirst, key, keyLen, value, valueLen)) return false;
			continue;
		}
		switch (wcs_getIndexInList(key, keyLen, kWcsFields, kFieldCount)) {
		case -1:
			break;
		case kTextCode:
			if (++blockIndex >= cdt_kBlockCountMax) return false;
			block = cdt->blocks + blockIndex;
			if (!parseTextCodeWcs(&block->album->encoding, value, valueLen)) return false;
			if (cdt_kCpUtf8 != cfgfile_getCodePage(sheet)) cfgfile_setCodepage(sheet, kCodepages[block->album->encoding]);
			break;
		case kFirstTrackNumber:
			if (!block) return false;
			if (!parseTrackNumberWcs(&trackFirst, value, valueLen)) return false;
			break;
		case kLastTrackNumber:
			if (!block) return false;
			if (!parseTrackNumberWcs(&trackLast, value, valueLen)) return false;
			if (!cdt_setTrackRange(cdt, trackFirst, trackLast)) return false;
			break;
		case kAlbumMessage:
			if (!block) return false;
			if (!cdt_setText(block, &block->album->message, value, valueLen)) return false;
			break;
		case kAlbumTitle:
			if (!block) return false;
			if (!cdt_setText(block, &block->album->title, value, valueLen)) return false;
			break;
		case kArranger:
			if (!block) return false;
			if (!cdt_setText(block, &block->album->arranger, value, valueLen)) return false;
			break;
		case kArtistName:
			if (!block) return false;
			if (!cdt_setText(block, &block->album->artist, value, valueLen)) return false;
			break;
		case kCatalogNumber:
			if (!block) return false;
			if (!cdt_setText(block, &block->album->catalog, value, valueLen)) return false;
			break;
		case kClosedInformation:
			if (!block) return false;
			if (!cdt_setText(block, &block->album->closedInfo, value, valueLen)) return false;
			break;
		case kComposer:
			if (!block) return false;
			if (!cdt_setText(block, &block->album->composer, value, valueLen)) return false;
			break;
		case kGenreCode:
			if (!block) return false;
			if (!parseGenreCodeWcs(&block->album->genreCode, value, valueLen)) return false;
			break;
		case kGenreInformation:
			if (!block) return false;
			if (!cdt_setText(block, &block->album->genreExtra, value, valueLen)) return false;
			break;
		case kLanguageCode:
			if (!block) return false;
			if (!parseLanguageCodeWcs(&block->album->language, value, valueLen)) return false;
			break;
		case kSongwriter:
			if (!block) return false;
			if (!cdt_setText(block, &block->album->songwriter, value, valueLen)) return false;
			break;
		case kTextDataCopyProtection:
			if (!block) return false;
			if (!parseCopyProtectionWcs(&block->album->copyProtected, value, valueLen)) return false;
			break;
		case kUpcEan:
			if (!block) return false;
			if (!cdt_setText(block, &block->album->upc, value, valueLen)) return false;
			break;
		default:
			assert(false);
			return false;
			break;
		}
	}
	return true;
}
