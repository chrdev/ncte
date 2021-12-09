#pragma once

#include <Windows.h>

#include <stdbool.h>
#include <math.h>

#include "dpi.h"


static inline bool
sys_isDropdownMenuRtl(void) {
	return GetSystemMetrics(SM_MENUDROPALIGNMENT);
}

static inline UINT
sys_getPopupMenuAlignment(void) {
	return sys_isDropdownMenuRtl() ? TPM_RIGHTALIGN : TPM_LEFTALIGN;
}

static inline bool
sys_mayDrag(const POINT* pt1, const POINT* pt2) {
	if (abs((pt1->x - pt2->x)) <= abs(GetSystemMetrics(SM_CXDRAG))) return false;
	if (abs((pt1->y - pt2->y)) <= abs(GetSystemMetrics(SM_CYDRAG))) return false;
	return true;
}

static inline int
sys_getScrollCy(void) {
	return GetSystemMetricsForDpi(SM_CYHSCROLL, theDpi.dpi);
}

static inline int
sys_getMenuItemCy(void) {
	return GetSystemMetricsForDpi(SM_CYMENUSIZE, theDpi.dpi);
}

static inline int
sys_getMenuCheckCx(void) {
	return GetSystemMetricsForDpi(SM_CXMENUCHECK, theDpi.dpi);
}

static inline int
sys_getMenuCheckCy(void) {
	return GetSystemMetricsForDpi(SM_CYMENUCHECK, theDpi.dpi);
}

static inline int
sys_getPaddedBorder(void) {
	return GetSystemMetricsForDpi(SM_CXPADDEDBORDER, theDpi.dpi);
}
