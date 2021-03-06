#pragma once
#include "mselector.h"

#include "fields.h"
#include "resdefs.h"
#include "str.h"
#include "msg.h"
#include "dpi.h"

static inline void
albumFieldsSelector_popup(HWND parent, int x, int y) {
	const ResStr items[] = {
		resstr_load(kTextSongwriter),
		resstr_load(kTextComposer),
		resstr_load(kTextArranger),
		resstr_load(kTextCatalog),
		resstr_load(kTextUpcEan),
		resstr_load(kTextMessage),
		resstr_load(kTextClosedInfo),
		resstr_load(kTextToc),
	};

	MSelectorParams params = {
		.x = x,
		.y = y,
		.cx = dpi_scale(resstr_loadNumber(kTextAlbumFieldsSelectorWidth)),
		.itemCount = ARRAYSIZE(items),
		.items = items,
		.msgId = fields_kAlbum,
	};
	mselector_popup(parent, &params);
}

static inline void
trackFieldsSelector_popup(HWND parent, int x, int y) {
	const ResStr items[] = {
		resstr_load(kTextTitle),
		resstr_load(kTextArtist),
		resstr_load(kTextSongwriter),
		resstr_load(kTextComposer),
		resstr_load(kTextArranger),
		resstr_load(kTextIsrc),
		resstr_load(kTextMessage),
		resstr_load(kTextToc),
	};

	MSelectorParams params = {
		.x = x,
		.y = y,
		.cx = dpi_scale(resstr_loadNumber(kTextTrackFieldsSelectorWidth)),
		.itemCount = ARRAYSIZE(items),
		.items = items,
		.msgId = fields_kTrack,
	};
	mselector_popup(parent, &params);
}
