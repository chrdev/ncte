#pragma once

#include <Windows.h>

#include <stdbool.h>


enum {
	filepath_kCch = MAX_PATH,
};

extern wchar_t thePath[];


static inline bool
filepath_hasExt(const wchar_t* path, const wchar_t* ext) {
	int extLen = lstrlen(ext);
	int pathLen = lstrlen(path);
	if (extLen > pathLen - 5) return false;
	return !lstrcmpi(path + pathLen - extLen, ext);
}
