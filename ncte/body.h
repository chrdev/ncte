#pragma once

#include <Windows.h>

#include "msg.h"


HWND
body_create(HWND parent, intptr_t id);

static inline void
body_setTrackRange(HWND wnd, int first, int last) {
	SendMessage(wnd, MSG_SETTRACKRANGE, (WPARAM)MAKEWORD(first, last), 0);
}
