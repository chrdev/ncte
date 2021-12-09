#pragma once

#include <Windows.h>

#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>


typedef struct Font {
	HFONT handle;
	int height;
	int aveCharWidth;
}Font;

typedef struct Fonts {
	Font normal;
	Font bold;
	Font smallNormal;
	Font smallBold;
	Font menu;
	Font icon;
}Fonts;
static_assert(sizeof(Fonts) / sizeof(Font) == 6, "Font count not correct");

extern Fonts theFonts;

bool
font_init(void);

void
font_uninit(void);

bool
font_refresh(void);

static inline SIZE
font_getTextExtent(HFONT font, const wchar_t* text) {
	SIZE size = { 0 };
	HDC dc = CreateCompatibleDC(NULL);
	SelectObject(dc, font);
	GetTextExtentPoint32(dc, text, lstrlen(text), &size);
	DeleteDC(dc);
	return size;
}

static inline int
font_getTextCx(HFONT font, const wchar_t* text) {
	SIZE size = font_getTextExtent(font, text);
	return size.cx;
}
