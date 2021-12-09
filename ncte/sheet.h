#pragma once

#include "cfgfile.h"
#include "cdt.h"

typedef CfgFile Sheet;

static inline bool
sheet_openForReading(Sheet* sheet, const wchar_t* path) {
	return cfgfile_openForReading(sheet, path);
}

static inline void
sheet_close(Sheet sheet) {
	cfgfile_close(sheet);
}

bool
sheet_read(Sheet sheet, Cdt* cdt);

bool
sheet_readWcs(Sheet sheet, Cdt* cdt);

static inline Sheet
sheet_fromWcs(const wchar_t* text, int len) {
	return (Sheet)cfgfile_fromWcs(text, len);
}
