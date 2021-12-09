#pragma once
// The typical usage is to call 3 functions in order:
// 1. cdt_createDecContext();
// 2. cdt_decode(); or cdt_decodeFile()
// 4. cdt_destroyDecContext();
//
// It's safe to reuse CdtDecContext by simply re-call cdt_decode() on a different Cdt*,
// as long as the bin memmory region is valid

#include "cdt.h"


typedef void* CdtDecContext;

// Return block count
int
cdt_decode(const void* bin, int binSize, Cdt* cdt, CdtDecContext ctx);

// Return block count
int
cdt_decodeFile(const wchar_t* path, Cdt* cdt, CdtDecContext ctx);

CdtDecContext
cdt_createDecContext(void);

void
cdt_destroyDecContext(CdtDecContext ctx);
