#pragma once

#include <Windows.h>


static inline int
msgbox_show(HWND owner, WORD text, WORD title, UINT type) {
	MSGBOXPARAMS params = {
		.cbSize = sizeof(MSGBOXPARAMS),
		.hwndOwner = owner,
		.hInstance = GetModuleHandle(NULL),
		.lpszText = MAKEINTRESOURCE(text),
		.lpszCaption = title ? MAKEINTRESOURCE(title) : L"",
		.dwStyle = type,
	};
	return MessageBoxIndirect(&params);
}
