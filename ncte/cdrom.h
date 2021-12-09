#pragma once

#include <Windows.h>

#include <stdbool.h>
#include <stdint.h>


typedef enum cd_Error{
	cd_kGood,
	cd_kNoText,
	cd_kNoCd,
	cd_kOther,
}cd_Error;

// If failed, return INVALID_HANDLE_VALUE
HANDLE
cd_open(wchar_t driveLetter, int* error);

static inline bool
cd_close(HANDLE h) {
	return CloseHandle(h);
}

// If succeed, return buffer, set size and error. Caller should HeapFree() the returned
// If fail, return NULL, set error.
uint8_t*
cd_dumpText(HANDLE f, int* size, int* error);

bool
cd_saveTextDump(const wchar_t* path, const uint8_t* bin, int size);
