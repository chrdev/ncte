#include "font.h"
#include "dpi.h"
#include "resdefs.h"


static const wchar_t kFontName[] = L"Tahoma";
static const wchar_t kIconName[] = L"NCTE Icons";


enum {
	kFontSize = 10,
	kIconSize = 12,
	kSmallSize = kFontSize - 2,
};

Fonts theFonts = { 0 };

static HFONT
createFontIndirect(Font* font, const LOGFONT* lf) {
	assert(font);
	assert(lf);
	assert(!font->handle);

	HDC dc = CreateCompatibleDC(NULL);
	if (!dc) return NULL;

	font->handle = CreateFontIndirect(lf);
	if (!font->handle) {
		DeleteDC(dc);
		return NULL;
	}

	SelectObject(dc, font->handle);
	TEXTMETRIC tm;
	GetTextMetrics(dc, &tm);
	DeleteDC(dc);
	font->height = tm.tmHeight;
	font->aveCharWidth = tm.tmAveCharWidth;

	return font->handle;
}

static HFONT
createFont(Font* font, const wchar_t* name, LONG size, LONG weight) {
	assert(font);
	assert(name);
	assert(!font->handle);

	HDC dc = CreateCompatibleDC(NULL);
	if (!dc) return NULL;

	font->handle = CreateFont(
		-MulDiv(size, theDpi.dpi, 72),
		//-MulDiv(size, GetDeviceCaps(dc, LOGPIXELSY), 72),
		0, 0, 0, weight, 0, 0, 0, DEFAULT_CHARSET,
		0, 0, 0, 0,
		name);
	if (!font->handle) {
		DeleteDC(dc);
		return NULL;
	}

	SelectObject(dc, font->handle);
	TEXTMETRIC tm;
	GetTextMetrics(dc, &tm);
	DeleteDC(dc);
	font->height = tm.tmHeight;
	font->aveCharWidth = tm.tmAveCharWidth;

	return font->handle;
}

static inline void
deleteFont(Font* font) {
	assert(font);
	assert(font->handle);

	DeleteObject(font->handle);
	font->handle = NULL;
}


static inline HFONT
createMenuFont(Font* font) {
	NONCLIENTMETRICS m = { sizeof(m) };
	SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(m), &m, 0, theDpi.dpi);
	return createFontIndirect(font, &m.lfMenuFont);
}

static inline HANDLE
loadIconFont(void) {
	HRSRC res = FindResource(NULL, MAKEINTRESOURCE(kRcIconFont), MAKEINTRESOURCE(kRtFont));
	if (!res) return NULL;
	DWORD size = SizeofResource(NULL, res);
	HGLOBAL h = LoadResource(NULL, res);
	if (!h) return NULL;
	void* view = LockResource(h);
	if (!view) return NULL;
	DWORD count;
	return AddFontMemResourceEx(view, size, NULL, &count);
}

bool
font_init(void) {
	static HANDLE ttf = NULL;
	if (!ttf) {
		ttf = loadIconFont();
		if (!ttf) return false;
	}

	bool result = true;
	HFONT font;
	font = createFont(&theFonts.normal, kFontName, kFontSize, FW_NORMAL);
	if (!font) result = false;
	font = createFont(&theFonts.bold, kFontName, kFontSize, FW_BOLD);
	if (!font) result = false;
	font = createFont(&theFonts.smallNormal, kFontName, kSmallSize, FW_NORMAL);
	if (!font) result = false;
	font = createFont(&theFonts.smallBold, kFontName, kSmallSize, FW_BOLD);
	if (!font) result = false;
	font = createMenuFont(&theFonts.menu);
	if (!font) result = false;
	font = createFont(&theFonts.icon, kIconName, kIconSize, FW_NORMAL);
	if (!font) result = false;
	return result;
}

void
font_uninit(void) {
	Font* font = (Font*)&theFonts;
	int count = sizeof(Fonts) / sizeof(Font);
	for (int i = 0; i < count; ++i) {
		deleteFont(font++);
	}
}

bool
font_refresh(void) {
	font_uninit();
	return font_init();
}
