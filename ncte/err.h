#pragma once

#include <Windows.h>


enum {
	kErrEmptyText = 0xE0000300,
	kErrBadTranscoding,
	kErrBadTimeRange,
	kErrBadTocAlignment,
	kErrPackCountOverflow,

	kErrFileBadOpen,
	kErrFileBadSize,
	kErrFileBadMapping,
	kErrFileBadMapView,
};

static inline void
err_raise(DWORD code) {
	RaiseException(code, 0, 0, NULL);
}
