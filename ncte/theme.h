#pragma once

#include <Windows.h>
#include <uxtheme.h>
#pragma comment(lib, "UxTheme.lib")
#include <vsstyle.h>

#include <stdbool.h>
#include <assert.h>


void
theme_drawCheckbox(HTHEME theme, HDC dc, RECT* rc, bool checked, bool hot);

void
theme_drawMenuItem(HTHEME theme, HDC dc, RECT* rc, bool hot);

void
theme_drawText(HTHEME theme, HDC dc, RECT* rc, const wchar_t* text, int cch, bool checked, bool hot);
