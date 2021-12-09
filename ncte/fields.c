#include "fields.h"

#include "wnd.h"
#include "resdefs.h"

static inline void
save(HWND wnd, int bottomOptEditId, BYTE fields) {
	LONG_PTR v = (bottomOptEditId << 16) | fields;
	SetWindowLongPtr(wnd, GWLP_USERDATA, v);
}

void
fields_set(HWND wnd, BYTE fields, int dlgTopOptLineY, const Line* const lines, int lineCount) {
	RECT rc, editRc;
	GetWindowRect(GetDlgItem(wnd, kIdTitleLabel), &rc);
	GetWindowRect(GetDlgItem(wnd, kIdTitle), &editRc);
	MapWindowPoints(NULL, wnd, (POINT*)&rc, 1);
	MapWindowPoints(NULL, wnd, (POINT*)&editRc, 1);
	int labelX = rc.left;
	int editX = editRc.left;
	int labelOffset = rc.top - editRc.top;

	rc.top = dlgTopOptLineY;
	rc.bottom = kRcLineH;
	MapDialogRect(wnd, &rc);
	int y = rc.top;
	int lineCy = rc.bottom;

	int bottomOptEditId = 0;
	int visibleLineCount = 0;
	for (int i = 0; i < lineCount; ++i) {
		HWND label = GetDlgItem(wnd, lines[i].labelId);
		HWND edit = GetDlgItem(wnd, lines[i].editId);
		if (fields & (1 << i)) {
			SetWindowPos(label, NULL, labelX, y + labelOffset, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_NOSIZE | SWP_SHOWWINDOW);
			SetWindowPos(edit, NULL, editX, y, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_NOSIZE | SWP_SHOWWINDOW);
			EnableWindow(label, TRUE); // This enables the accelerator.
			bottomOptEditId = lines[i].editId;
			++visibleLineCount;
			y += lineCy;
		}
		else {
			ShowWindow(label, SW_HIDE);
			ShowWindow(edit, SW_HIDE);
			EnableWindow(label, FALSE); // Hiding deesn't disable accelerator, disabling the window does.
		}
	}

	save(wnd, bottomOptEditId, fields);

	rc.top = (dlgTopOptLineY - kRcLineH + kRcEditH + kRcMargin) + kRcLineH * visibleLineCount;
	MapDialogRect(wnd, &rc);
	wnd_setCy(wnd, rc.top);
}
