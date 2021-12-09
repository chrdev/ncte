#pragma once

#include <Windows.h>

#include "resdefs.h"
#include "fields.h"
#include "bit.h"


enum { trackPanel_kIdDefBottom = kIdTitle };

HWND
trackPanel_create(HWND parent, intptr_t id);

void
trackPanel_setFields(HWND wnd, const bool fields[trackFields_kEnd]);

void
trackPanel_fill(HWND wnd, int blockIndex, int trackIndex);

void
trackPanel_saveCurrentCtl(HWND wnd);

static inline int
trackPanel_getDialogH(HWND wnd) {
	BYTE fields = fields_retrieveFields(wnd);
	const optLineCount = bit_count8(fields);
	return kRcLineH * (optLineCount + kRcTrackFixedLineCount) - kRcSpacingH + kRcMargin + kRcMargin;
}

static inline bool
trackPanel_isOnlyTocVisible(HWND wnd) {
	BYTE fields = fields_retrieveFields(wnd);
	BYTE mask = 1 << trackFields_kToc;
	return (mask & fields) && !(~mask & fields);
}
