#pragma once

#include <Windows.h>

#include "resdefs.h"
#include "fields.h"
#include "bit.h"


enum { albumPanel_kIdDefBottom = kIdArtist };

HWND
albumPanel_create(HWND parent);

void
albumPanel_setFields(HWND wnd, const bool fields[albumFields_kEnd]);

static inline int
albumPanel_getBottomEditId(HWND wnd) {
	int ctlId = fields_retrieveBottomOptEditId(wnd);
	if (ctlId) return ctlId;
	return albumPanel_kIdDefBottom;
}

void
albumPanel_fill(HWND wnd, int blockIndex);

static inline int
albumPanel_getDialogH(HWND wnd) {
	BYTE fields = fields_retrieveFields(wnd);
	const optLineCount = bit_count8(fields);
	return kRcLineH * (optLineCount + kRcAlbumFixedLineCount) - kRcSpacingH + kRcMargin + kRcMargin;
}
