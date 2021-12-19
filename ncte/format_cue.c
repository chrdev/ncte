#include "format.h"

#include "str.h"


static const wchar_t kNewLine[] = L"\r\n";
static const wchar_t kTextNewLine[] = L"\"\r\n";
static const wchar_t kCatalog[] = L"CATALOG ";
static const wchar_t kCDTextFile[] = L"CDTEXTFILE \"\"\r\n";
static const wchar_t kFile[] = L"FILE \"\" WAVE\r\n";
static const wchar_t kPerformer[] = L"PERFORMER \"";
static const wchar_t kSongwriter[] = L"SONGWRITER \"";
static const wchar_t kTitle[] = L"TITLE \"";

static const wchar_t kTrack[] = L"TRACK ";
static const wchar_t kAudioNewLine[] = L" AUDIO\r\n";
static const wchar_t kIndent[] = L"\t";
static const wchar_t kIndex[] = L"INDEX 01 ";
static const wchar_t kIsrc[] = L"ISRC ";

bool
cue_isMyKind(const wchar_t* text) {
	return false;
}

static inline bool
encCatalog(wchar_t** buffer, const wchar_t* text) {
	if (wcs_isEmpty(text)) return true;
	if (!wcsbuffer_append(buffer, kCatalog, ARRAYSIZE(kCatalog) - 1)) return false;
	if (!wcsbuffer_append(buffer, text, -1)) return false;
	if (!wcsbuffer_append(buffer, kNewLine, ARRAYSIZE(kNewLine) - 1)) return false;
	return true;
}

// placeholder
static inline bool
encCDTextFile(wchar_t** buffer) {
	return wcsbuffer_append(buffer, kCDTextFile, ARRAYSIZE(kCDTextFile) - 1);
}

// placeholder
static inline bool
encFile(wchar_t** buffer) {
	return wcsbuffer_append(buffer, kFile, ARRAYSIZE(kFile) - 1);
}

static inline bool
encIndent(wchar_t** buffer) {
	return wcsbuffer_append(buffer, kIndent, ARRAYSIZE(kIndent) - 1);
}

static inline bool
encArtist(wchar_t** buffer, const wchar_t* text, bool indent) {
	if (wcs_isEmpty(text)) return true;
	if (indent && !encIndent(buffer)) return false;
	if (!wcsbuffer_append(buffer, kPerformer, ARRAYSIZE(kPerformer) - 1)) return false;
	if (!wcsbuffer_append(buffer, text, -1)) return false;
	if (!wcsbuffer_append(buffer, kTextNewLine, ARRAYSIZE(kTextNewLine) - 1)) return false;
	return true;
}

static inline bool
encSongwriter(wchar_t** buffer, const wchar_t* text, bool indent) {
	if (wcs_isEmpty(text)) return true;
	if (indent && !encIndent(buffer)) return false;
	if (!wcsbuffer_append(buffer, kSongwriter, ARRAYSIZE(kSongwriter) - 1)) return false;
	if (!wcsbuffer_append(buffer, text, -1)) return false;
	if (!wcsbuffer_append(buffer, kTextNewLine, ARRAYSIZE(kTextNewLine) - 1)) return false;
	return true;
}

static inline bool
encTitle(wchar_t** buffer, const wchar_t* text, bool indent) {
	if (wcs_isEmpty(text)) return true;
	if (indent && !encIndent(buffer)) return false;
	if (!wcsbuffer_append(buffer, kTitle, ARRAYSIZE(kTitle) - 1)) return false;
	if (!wcsbuffer_append(buffer, text, -1)) return false;
	if (!wcsbuffer_append(buffer, kTextNewLine, ARRAYSIZE(kTextNewLine) - 1)) return false;
	return true;
}

// Convert as much as we can from the block, ignores errors.
static inline bool
encAlbum(wchar_t** buffer, const cdt_Album* album) {
	if (!encCatalog(buffer, album->upc)) return false;
	if (!encCDTextFile(buffer)) return false;
	if (!encFile(buffer)) return false;
	if (!encArtist(buffer, album->artist, false)) return false;
	if (!encSongwriter(buffer, album->songwriter, false)) return false;
	if (!encTitle(buffer, album->title, false)) return false;
	return true;
}

static inline bool
encTrackHeader(wchar_t** buffer, int trackNumber) {
	if (!wcsbuffer_append(buffer, kTrack, ARRAYSIZE(kTrack) - 1)) return false;
	int len;
	const wchar_t*t = wcs_formatEx(&len, L"%d", trackNumber);
	if (!wcsbuffer_append(buffer, t, len)) return false;
	if (!wcsbuffer_append(buffer, kAudioNewLine, ARRAYSIZE(kAudioNewLine) - 1)) return false;
	return true;
}

static inline bool
encIndex(wchar_t** buffer, const cdt_Time* time) {
	if (!encIndent(buffer)) return false;
	if (!wcsbuffer_append(buffer, kIndex, ARRAYSIZE(kIndex) - 1)) return false;
	int len;
	const wchar_t* t = wcs_formatEx(&len, L"%02u:%02u:%02u", time->min, time->sec, time->frame);
	if (!wcsbuffer_append(buffer, t, len)) return false;
	if (!wcsbuffer_append(buffer, kNewLine, ARRAYSIZE(kNewLine) - 1)) return false;
	return true;
}

static inline bool
encIsrc(wchar_t** buffer, const wchar_t* text) {
	if (wcs_isEmpty(text)) return true;
	if (!encIndent(buffer)) return false;
	if (!wcsbuffer_append(buffer, kIsrc, ARRAYSIZE(kIsrc) - 1)) return false;
	if (!wcsbuffer_append(buffer, text, -1)) return false;
	if (!wcsbuffer_append(buffer, kNewLine, ARRAYSIZE(kNewLine) - 1)) return false;
	return true;
}

// Convert as much as we can from the track, ignores errors.
static inline bool
encTrack(wchar_t** buffer, int trackNumber, const cdt_Track* track, const cdt_Time* time) {
	if (!encTrackHeader(buffer, trackNumber)) return false;
	if (!encIndex(buffer, time)) return false;
	if (!encIsrc(buffer, track->isrc)) return false;
	if (!encArtist(buffer, track->artist, true)) return false;
	if (!encSongwriter(buffer, track->songwriter, true)) return false;
	if (!encTitle(buffer, track->title, true)) return false;
	return true;
}

wchar_t*
cue_blockToText(const cdt_Block* block, const cdt_Time* toc, size_t* len) {
	enum { kInitSize = 1024 };
	wchar_t* buffer = wcsbuffer_init(kInitSize);
	if (!buffer) return NULL;

	if (!encAlbum(&buffer, block->album)) goto leave;

	for (int i = 0; i < block->trackCount; ++i) {
		int trackNumber = block->trackFirst + i;
		if (!encTrack(&buffer, trackNumber, block->tracks + i, toc + i)) break;
	}
leave:;
	int cch = wcsbuffer_getCch(buffer);
	if (!cch) {
		HeapFree(GetProcessHeap(), 0, buffer);
		return NULL;
	}
	if (len) *len = cch;
	return buffer;
}
