#pragma once

#include <Windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>


typedef struct Line {
	WORD labelId;
	WORD editId;
}Line;

// Optional fields of album and track

enum {
	albumFields_kBegin,
	albumFields_kSongwriter = albumFields_kBegin,
	albumFields_kComposer,
	albumFields_kArranger,
	albumFields_kCatalog,
	albumFields_kCode,
	albumFields_kMessage,
	albumFields_kClosedInfo,
	albumFields_kToc,
	albumFields_kEnd,
};

static const BYTE albumFields_kSongwriterMask = 1 << albumFields_kSongwriter;
static const BYTE albumFields_kComposerMask = 1 << albumFields_kComposer;
static const BYTE albumFields_kArrangerMask = 1 << albumFields_kArranger;
static const BYTE albumFields_kCodeMask = 1 << albumFields_kCode;
static const BYTE albumFields_kMessageMask = 1 << albumFields_kMessage;
static const BYTE albumFields_kCatalogMask = 1 << albumFields_kCatalog;
static const BYTE albumFields_kClosedInfoMask = 1 << albumFields_kClosedInfo;
static const BYTE albumFields_kTocMask = 1 << albumFields_kToc;

enum {
	trackFields_kBegin,
	trackFields_kTitle = trackFields_kBegin,
	trackFields_kArtist,
	trackFields_kSongwriter,
	trackFields_kComposer,
	trackFields_kArranger,
	trackFields_kCode,
	trackFields_kMessage,
	trackFields_kToc,
	trackFields_kEnd,
};

static const BYTE trackFields_kTitleMask      = 1 << trackFields_kTitle;
static const BYTE trackFields_kArtistMask     = 1 << trackFields_kArtist;
static const BYTE trackFields_kSongwriterMask = 1 << trackFields_kSongwriter;
static const BYTE trackFields_kComposerMask   = 1 << trackFields_kComposer;
static const BYTE trackFields_kArrangerMask   = 1 << trackFields_kArranger;
static const BYTE trackFields_kCodeMask       = 1 << trackFields_kCode;
static const BYTE trackFields_kMessageMask    = 1 << trackFields_kMessage;
static const BYTE trackFields_kTocMask        = 1 << trackFields_kToc;


//#define AlbumFields(x) \
//	bool x[albumFields_kEnd - albumFields_kBegin]
//
//#define TrackFields(x) \
//	bool x[trackFields_kEnd - trackFields_kBegin]

static inline uint8_t
fields_toByte(const bool* const fields, int fieldCount) {
	assert(fieldCount <= 8);
	uint8_t result = 0;
	for (int i = 0; i < fieldCount; ++i) {
		if (fields[i]) result |= 1 << i;
	}
	return result;
}

static inline void
fields_setByMask(bool* fields, int fieldCount, uint8_t v) {
	assert(fieldCount <= 8);
	for (int i = 0; i < fieldCount; ++i) {
		fields[i] = v & (1 << i);
	}
}

static inline BYTE
fields_retrieveFields(HWND wnd) {
	LONG_PTR v = (int)GetWindowLongPtr(wnd, GWLP_USERDATA);
	return LOBYTE(v);
}

static inline int
fields_retrieveBottomOptEditId(HWND wnd) {
	int v = (int)GetWindowLongPtr(wnd, GWLP_USERDATA);
	return v >> 16;
}

void
fields_set(HWND wnd, BYTE fields, int dlgTopOptLineY, const Line* const lines, int lineCount);
