#pragma once

#include <Windows.h>

typedef struct Dpi {
	int	dpi;
	int scaleFactor;

	int edgeCx;
	int edgeCy;
}Dpi;

extern Dpi theDpi;

void
dpi_refresh(HWND wnd);

static inline void
dpi_set(int dpi) {
	theDpi.dpi = dpi;
	theDpi.scaleFactor = MulDiv(dpi, 100, 96);
	theDpi.edgeCx = GetSystemMetricsForDpi(SM_CXEDGE, dpi);
	theDpi.edgeCy = GetSystemMetricsForDpi(SM_CYEDGE, dpi);
}

static inline int
dpi_scale(int n) {
	return MulDiv(n, theDpi.scaleFactor, 100);
}

//static inline void
//dpi_scalePoint(POINT* p) {
//	p->x = dpi_scale(p->x);
//	p->y = dpi_scale(p->y);
//}
//
//static inline void
//dpi_scaleRect(RECT* rc) {
//	rc->left = dpi_scale(rc->left);
//	rc->top = dpi_scale(rc->top);
//	rc->right = dpi_scale(rc->right);
//	rc->bottom = dpi_scale(rc->bottom);
//}
