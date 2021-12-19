#pragma once

#include <Windows.h>

#include <stdint.h>
#include "str.h"
#include "cdt.h"
#include "msg.h"


enum {
	mselector_kItemLimit = 8,
};

typedef struct MSelectorParams {
	int x;
	int y;
	int cx;
	int msgId;

	int itemCount;
	const ResStr* items;
}MSelectorParams;


void
mselector_popup(HWND parent, const MSelectorParams* params);
