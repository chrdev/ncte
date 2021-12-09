#include "cfgfile.h"

#include <assert.h>

#include "err.h"


enum {
	kFileSizeMin = 4,
	kFileSizeMax = 100000,
};


struct CfgFile_ {
	bool isWcs; // If true, also means reading from pure memory, not file.

	HANDLE f;
	union {
		const BYTE* base;
		const wchar_t* wcsBase;
	};
	union {
		const BYTE* eof;
		const wchar_t* wcsEof;
	};
	union {
		const BYTE* p;
		const char* key; // We don't allow leading space for a key, so key is always the same as p
		const wchar_t* wcsP;
		const wchar_t* wcsKey;
	};

	// tokenize() sets key and the following 3 variables
	size_t keyLen;
	union {
		const char* value; // pointer to a char between (key + keyLen + splitterLen) and eol;
		const wchar_t* wcsValue;
	};
	size_t valueLen;

	// findEol() sets eol and lineNext
	union {
		const BYTE* eol; // pointer to the last charater just before 0x0D or 0x0A, plus 1
		const wchar_t* wcsEol;
	};
	union {
		const BYTE* lineNext; // pointer to the first character just after lineEnd and any 0x0D or 0x0A
		const wchar_t* wcsLineNext;
	};

	UINT codepage;
};

bool
writeBom(HANDLE f) {
	static const char kBom[] = { 0xEF, 0xBB, 0xBF };
	DWORD cb;
	return WriteFile(f, kBom, sizeof(kBom), &cb, NULL) && cb == sizeof(kBom);
}

bool
cfgfile_openForWriting(CfgFile* file, const wchar_t* path) {
	HANDLE f = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE) return false;
	if (!writeBom(f)) {
		CloseHandle(f);
		return false;
	}

	struct CfgFile_* cf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct CfgFile_));
	if (!cf) {
		CloseHandle(f);
		return false;
	}
	cf->f = f;
	*file = cf;

	return true;
}

static inline void
readUtf8Bom(struct CfgFile_* cf) {
	if (*cf->p++ != 0xEF) goto leave;
	if (*cf->p++ != 0xBB) goto leave;
	if (*cf->p   != 0xBF) goto leave;
	cf->codepage = 65001;
leave:;
	cf->p = cf->base;
}

static inline void
skipLeadingNewLinesWcs(struct CfgFile_* cf) {
	for (; (*cf->wcsP == L'\r' || *cf->wcsP == L'\n') && cf->wcsP < cf->wcsEof; ++cf->wcsP);
}

static inline void
skipLeadingNewLines(struct CfgFile_* cf) {
	for (; (*cf->p == '\r' || *cf->p == '\n') && cf->p < cf->eof; ++cf->p);
}

bool
cfgfile_openForReading(CfgFile* file, const wchar_t* path) {
	HANDLE f = INVALID_HANDLE_VALUE;
	BYTE* base = NULL;
	struct CfgFile_* cf = NULL;
	__try {
		f = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (f == INVALID_HANDLE_VALUE) err_raise(kErrFileBadOpen);
		LARGE_INTEGER li;
		if (!GetFileSizeEx(f, &li) || li.QuadPart < kFileSizeMin || li.QuadPart > kFileSizeMax) err_raise(kErrFileBadSize);
		HANDLE map = CreateFileMapping(f, NULL, PAGE_READONLY, 0, 0, NULL);
		if (!map) err_raise(kErrFileBadMapping);
		base = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
		CloseHandle(map);
		if (!base) err_raise(kErrFileBadMapView);
		cf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, sizeof(struct CfgFile_));
		cf->f = f;
		cf->base = base;
		cf->p = base;
		cf->eof = base + li.LowPart;
		readUtf8Bom(cf);
		skipLeadingNewLines(cf);
	}
	__except (1) {
		if (base) UnmapViewOfFile(base);
		if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
		return false;
	}

	*file = cf;
	return true;
}

void
cfgfile_close(CfgFile file) {
	struct CfgFile_* cf = (struct CfgFile_*)file;
	if (!cf->isWcs) {
		if (cf->base) UnmapViewOfFile(cf->base);
		CloseHandle(cf->f);
	}
	HeapFree(GetProcessHeap(), 0, cf);
}

static inline bool
textEmpty(const wchar_t* text) {
	return (!text || text[0] == L'\0');
}

bool
cfgfile_write(CfgFile file, const wchar_t* key, const wchar_t* value, int valueWidth) {
	
	return true;
}

static inline void
findEol(struct CfgFile_* cf) {
	assert(!cf->isWcs);
	bool found = false;
	const BYTE* p = cf->p;
	for (; p < cf->eof; ++p) {
		switch (*p) {
		case 0xD:
		case 0xA:
			if (!found) {
				found = true;
				cf->eol = p;
			}
			else {
				cf->lineNext = p + 1;
			}
			break;
		default:
			if (found) {
				cf->lineNext = p;
				goto leave;
			}
			break;
		}
	}
leave:;
	if (!found) {
		cf->eol = cf->eof;
		cf->lineNext = cf->eof;
	}
}

static inline void
findEolWcs(struct CfgFile_* cf) {
	assert(cf->isWcs);
	bool found = false;
	const wchar_t* p = cf->wcsP;
	for (; p < cf->wcsEof; ++p) {
		switch (*p) {
		case L'\r':
		case L'\n':
			if (!found) {
				found = true;
				cf->wcsEol = p;
			}
			else {
				cf->wcsLineNext = p + 1;
			}
			break;
		default:
			if (found) {
				cf->wcsLineNext = p;
				goto leave;
			}
			break;
		}
	}
leave:;
	if (!found) {
		cf->wcsEol = cf->wcsEof;
		cf->wcsLineNext = cf->wcsEof;
	}
}

//static inline bool
//isAlphabet(const char* c) {
//	if (c < 'A') return false;
//	if (c > 'Z' && c < 'a') return false;
//	if (c > 'z') return false;
//	return true;
//}

static inline bool
isAlphaNumeric(const char c) {
	if (c < '0') return false;
	if (c > '9' && c < 'A') return false;
	if (c > 'Z' && c < 'a') return false;
	if (c > 'z') return false;
	return true;
}

static inline bool
isAlphaNumericWc(const wchar_t c) {
	if (c < L'0') return false;
	if (c > L'9' && c < L'A') return false;
	if (c > L'Z' && c < L'a') return false;
	if (c > L'z') return false;
	return true;
}

static inline const char*
findSplitter(const char* begin, const char* end) {
	const char* result = NULL;
	for (const char* p = begin; p < end; ++p) {
		if (*p != '=') continue;
		result = p;
	}
	return result;
}

static inline const wchar_t*
findSplitterWcs(const wchar_t* begin, const wchar_t* end) {
	const wchar_t* result = NULL;
	for (const wchar_t* p = begin; p < end; ++p) {
		if (*p != L'=') continue;
		result = p;
	}
	return result;
}

static inline const char*
findFirstNonWhiteSpace(const char* begin, const char* end, int step) {
	assert(step == 1 || step == -1);
	const char* result = NULL;
	for (const char* p = begin; p != end; p += step) {
		if (*p == 0x20) continue;
		if (*p == 0x09) continue;
		result = p;
		break;
	}
	return result;
}

static inline const wchar_t*
findFirstNonWhiteSpaceWcs(const wchar_t* begin, const wchar_t* end, int step) {
	assert(step == 1 || step == -1);
	const wchar_t* result = NULL;
	for (const wchar_t* p = begin; p != end; p += step) {
		if (*p == L' ') continue;
		if (*p == L'\t') continue;
		result = p;
		break;
	}
	return result;
}

static inline bool
tokenize(struct CfgFile_* cf) {
	assert(!cf->isWcs);
	if (!isAlphaNumeric(*cf->p)) return false;
	const char* splitter = findSplitter(cf->p + 1, cf->eol - 1);
	if (!splitter) return false;
	const char* keyLast = findFirstNonWhiteSpace(splitter - 1, cf->p - 1, -1);
	if (!keyLast) return false;
	const char* valueFirst = findFirstNonWhiteSpace(splitter + 1, cf->eol, 1);
	if (!valueFirst) return false;
	const char* valueLast = findFirstNonWhiteSpace(cf->eol - 1, splitter, -1);
	assert(valueFirst <= valueLast);

	cf->keyLen = keyLast - cf->key + 1;
	cf->value = valueFirst;
	cf->valueLen = valueLast - valueFirst + 1;

	return true;
}

static inline bool
tokenizeWcs(struct CfgFile_* cf) {
	assert(cf->isWcs);
	if (!isAlphaNumericWc(*cf->wcsP)) return false;
	const wchar_t* splitter = findSplitterWcs(cf->wcsP + 1, cf->wcsEol - 1);
	if (!splitter) return false;
	const wchar_t* keyLast = findFirstNonWhiteSpaceWcs(splitter - 1, cf->wcsP - 1, -1);
	if (!keyLast) return false;
	const wchar_t* valueFirst = findFirstNonWhiteSpaceWcs(splitter + 1, cf->wcsEol, 1);
	if (!valueFirst) return false;
	const wchar_t* valueLast = findFirstNonWhiteSpaceWcs(cf->wcsEol - 1, splitter, -1);
	assert(valueFirst <= valueLast);

	cf->keyLen = keyLast - cf->wcsKey + 1;
	cf->wcsValue = valueFirst;
	cf->valueLen = valueLast - valueFirst + 1;

	return true;
}

//static inline bool
//reserveTextBuffer(wchar_t** buffer, size_t cch) {
//	size_t cb = sizeof(wchar_t) * cch;
//	if (!*buffer) {
//		*buffer = HeapAlloc(GetProcessHeap(), 0, cb);
//		if (!*buffer) return false;
//	}
//	else if (HeapSize(GetProcessHeap(), 0, *buffer) < cb) {
//		wchar_t* newBuf = HeapReAlloc(GetProcessHeap(), 0, *buffer, cb);
//		if (newBuf) *buffer = newBuf;
//		else return false;
//	}
//	return true;
//}
//
//static inline int
//textFromCodepage(wchar_t** buffer, const char* str, int len, UINT codepage) {
//	int cch = MultiByteToWideChar(codepage, 0, str, len, NULL, 0);
//	if (!cch) return 0;
//	if (!reserveTextBuffer(buffer, cch + 1)) return 0;
//	cch = MultiByteToWideChar(codepage, 0, str, len, *buffer, cch + 1);
//	(*buffer)[cch] = L'\0';
//	return cch;
//}

//static inline bool
//transcode(struct CfgFile_* cf) {
//	if (!textFromCodepage(&cf->key, cf->rawKey, (int)cf->rawKeyLen, cf->codepage)) return false;
//	if (!textFromCodepage(&cf->value, cf->rawValue, (int)cf->rawValueLen, cf->codepage)) return false;
//	return true;
//}

bool
cfgfile_read(CfgFile file, const char** key, int* keyLen, const char** value, int* valueLen) {
	struct CfgFile_* cf = (struct CfgFile_*)file;
	for (;; cf->p = cf->lineNext) {
		findEol(cf);
		if (cf->p == cf->eol) return false;
		if (tokenize(cf)) break;
	}
	*key = cf->key;
	*keyLen = (int)cf->keyLen;
	*value = cf->value;
	*valueLen = (int)cf->valueLen;
	cf->p = cf->lineNext;
	return true;
}

bool
cfgfile_readWcs(CfgFile file, const wchar_t** key, int* keyLen, const wchar_t** value, int* valueLen) {
	struct CfgFile_* cf = (struct CfgFile_*)file;
	for (;; cf->wcsP = cf->wcsLineNext) {
		findEolWcs(cf);
		if (cf->wcsP == cf->wcsEol) return false;
		if (tokenizeWcs(cf)) break;
	}
	*key = cf->wcsKey;
	*keyLen = (int)cf->keyLen;
	*value = cf->wcsValue;
	*valueLen = (int)cf->valueLen;
	cf->wcsP = cf->wcsLineNext;
	return true;
}

void
cfgfile_setCodepage(CfgFile file, UINT codepage) {
	struct CfgFile_* cf = (struct CfgFile_*)file;
	cf->codepage = codepage;
}

UINT
cfgfile_getCodePage(CfgFile file) {
	struct CfgFile_* cf = (struct CfgFile_*)file;
	return cf->codepage;
}

CfgFile
cfgfile_fromWcs(const wchar_t* text, int len) {
	if (len < kFileSizeMin || len > kFileSizeMax) return NULL;

	struct CfgFile_* cf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct CfgFile_));
	if (!cf) return NULL;

	cf->isWcs = true;
	cf->wcsBase = text;
	cf->wcsP = text;
	cf->wcsEof = text + (size_t)len;
	skipLeadingNewLinesWcs(cf);
	return cf;
}

bool
cfgfile_isWcs(CfgFile file) {
	struct CfgFile_* cf = (struct CfgFile_*)file;
	return cf->isWcs;
}
