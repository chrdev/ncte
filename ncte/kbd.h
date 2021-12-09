#pragma once

#include <Windows.h>

#include <stdbool.h>


static inline bool
kbd_isShiftDown(void) {
	return GetKeyState(VK_SHIFT) & 0x8000;
}

static inline bool
kbd_isCtrlDown(void) {
	return GetKeyState(VK_CONTROL) & 0x8000;
}
