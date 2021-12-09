#pragma once

#include <Windows.h>
#include <strsafe.h>
#pragma comment(lib, "strsafe.lib")

#include <stdlib.h> // _countof
#include <stdarg.h> // va_start, va_end


static inline void _cdecl
dbg_printf(const wchar_t* format, ...) {
#ifdef _DEBUG
	wchar_t buffer[260];
	va_list ap;
	va_start(ap, format);
	StringCchVPrintf(buffer, _countof(buffer), format, ap);
	va_end(ap);
	OutputDebugString(buffer);
#endif
}
