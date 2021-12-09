#include "theme.h"

static const int kCheckStates[2][2] = {
	{ CBS_UNCHECKEDNORMAL, CBS_UNCHECKEDHOT },
	{ CBS_CHECKEDNORMAL, CBS_CHECKEDHOT },
};

void
theme_drawCheckbox(HTHEME theme, HDC dc, RECT* rc, bool checked, bool hot) {
	if (theme) {
		int state = kCheckStates[checked][hot];
		DrawThemeBackground(theme, dc, BP_CHECKBOX, state, rc, NULL);
	}
	else {
		UINT state = DFCS_BUTTONCHECK;
		if (checked) state |= DFCS_CHECKED;
		DrawFrameControl(dc, rc, DFC_BUTTON, state);
	}
}

void
theme_drawText(HTHEME theme, HDC dc, RECT* rc, const wchar_t* text, int cch, bool checked, bool hot) {
	DWORD flags = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX;
	if (theme) {
		int state = hot ? MPI_HOT : MPI_NORMAL;
		DrawThemeText(theme, dc, MENU_POPUPITEM, state, text, cch, flags , 0, rc);
	}
	else {
		COLORREF color = hot ? 0xFFFFFF - GetSysColor(COLOR_MENUTEXT) : GetSysColor(COLOR_MENUTEXT);
		SetTextColor(dc, color);
		SetBkMode(dc, TRANSPARENT);
		DrawText(dc, text, cch, rc, flags);
	}
}

void
theme_drawMenuItem(HTHEME theme, HDC dc, RECT* rc, bool hot) {
	if (theme) {
		int state = hot ? MPI_HOT : MPI_NORMAL;
		DrawThemeBackground(theme, dc, MENU_POPUPITEM, state, rc, NULL);
	}
	else {
		int color = hot ? COLOR_MENUHILIGHT : COLOR_MENU;
		FillRect(dc, rc, GetSysColorBrush(color));
	}
}
