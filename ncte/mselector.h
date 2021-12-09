#pragma once

#include <Windows.h>

#include <stdint.h>
#include "str.h"
#include "cdt.h"


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

	uint8_t fields;
	cdt_getFieldsFunc getFieldsFunc;
	cdt_getHiddenFieldsFunc getHiddenFieldsFunc;
	int notifyHideFieldsExtra;
}MSelectorParams;


void
mselector_popup(HWND parent, const MSelectorParams* params);
