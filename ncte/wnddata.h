#pragma once

#define USE_WNDDATA WndData* wd = (WndData*)GetWindowLongPtr(wnd, GWLP_USERDATA)

#define STOR_WNDDATA \
	WndData* wd = (WndData*)(cs)->lpCreateParams;\
	SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR)wd)
