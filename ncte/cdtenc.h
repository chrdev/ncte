#pragma once
// The typical usage is to call these 4 functions in order:
// 1. cdt_createEncContext();
// 2. cdt_encode();
// 3. cdt_saveBin();
// 4. cdt_destroyEncContext();
//
// It's safe to reuse CdtEncContext by simply re-call cdt_encode() on a different Cdt*

#include "cdt.h"


typedef void* CdtEncContext;

bool
cdt_encode(const Cdt* cdt, CdtEncContext ctx);

CdtEncContext
cdt_createEncContext(void);

void
cdt_destroyEncContext(CdtEncContext ctx);

bool
cdt_saveBin(const wchar_t* path, const CdtEncContext ctx);
