#pragma once

#include <Windows.h>

#include <stdbool.h>
#include <assert.h>


static inline bool
mem_reserve(HANDLE heap, void** p, size_t size) {
	assert(heap);
	if (!*p) {
		*p = HeapAlloc(heap, 0, size);
		return *p;
	}
	else if (HeapSize(heap, 0, *p) < size) {
		void* newP = HeapReAlloc(heap, 0, *p, size);
		if (!newP) return false;
		*p = newP;
	}
	return true;
}

// Expand p to twice its original size.
static inline bool
mem_expand(HANDLE heap, void** p) {
	assert(heap);
	assert(*p);
	size_t size = 2 * HeapSize(heap, 0, *p);
	void* newP = HeapReAlloc(heap, 0, *p, size);
	if (newP) *p = newP;
	return newP;
}

static inline bool
mem_reserveBuddies(HANDLE heap, void** p1, void** p2, size_t size1, size_t size2) {
	assert(heap);
	void* m1 = *p1 ? *p1 : HeapAlloc(heap, 0, 0);
	if (!m1) return false;
	void* m2 = *p2 ? *p2 : HeapAlloc(heap, 0, 0);
	if (!m2) {
		HeapFree(heap, 0, m1);
		return false;
	}

	size_t orgSize1 = HeapSize(heap, 0, m1);
	size_t orgSize2 = HeapSize(heap, 0, m2);
	__try {
		m1 = HeapReAlloc(heap, HEAP_GENERATE_EXCEPTIONS, m1, size1);
		m2 = HeapReAlloc(heap, HEAP_GENERATE_EXCEPTIONS, m2, size2);
	}
	__except (1) {
		if (*p1) HeapReAlloc(heap, 0, *p1, orgSize1);
		else HeapFree(heap, 0, m1);
		if (*p2) HeapReAlloc(heap, 0, *p1, orgSize2); // We don't really need this because *p2 never got reallocated, but it's harmless.
		else HeapFree(heap, 0, m2);
		return false;
	}
	*p1 = m1;
	*p2 = m2;
	return true;
}

static inline bool
mem_expandBuddies(HANDLE heap, void** p1, void** p2) {
	assert(heap);
	assert(*p1);
	assert(*p2);

	size_t size1 = 2 * HeapSize(heap, 0, *p1);
	size_t size2 = 2 * HeapSize(heap, 0, *p2);
	return mem_reserveBuddies(heap, p1, p2, size1, size2);
}
