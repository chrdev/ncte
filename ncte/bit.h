#pragma once

#include <Windows.h>

#include <stdint.h>

static inline int
bit_count8(uint8_t v) {
	int c;
	for (c = 0; v; ++c) {
		v &= v - 1;
	}
	return c;
}

static inline uint8_t
bit_flip8(uint8_t v, uint8_t index) {
	uint8_t mask = 1 << index;
	uint8_t flip = ~v;
	return v ^ ((v ^ flip) & mask);
}

// See http://esr.ibiblio.org/?p=5095
//#define IS_BIG_ENDIAN (*(uint16_t*)"\0\xff" < 0x100)
