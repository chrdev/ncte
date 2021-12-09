#pragma once

#include <Windows.h>

#include <stdint.h>


static inline const uint8_t*
file_openView(const wchar_t* path, int* size) {
	enum { kFileSizeMax = 0x7FFFFFFF };
	const uint8_t* p = NULL;

	HANDLE f = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (f == INVALID_HANDLE_VALUE) return NULL;
	LARGE_INTEGER li;
	if (!GetFileSizeEx(f, &li) || li.QuadPart <= 0 || li.QuadPart > kFileSizeMax) goto cleanup;
	HANDLE mapping = CreateFileMapping(f, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!mapping) goto cleanup;
	p = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
	CloseHandle(mapping);
	if (!p) goto cleanup;

	*size = (int)li.LowPart;
cleanup:;
	CloseHandle(f);
	return p;
}

static inline void
file_closeView(const uint8_t* p) {
	UnmapViewOfFile(p);
}
