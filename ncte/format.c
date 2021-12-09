#include "format.h"

#include "clipboard.h"


bool
format_copy(HWND wnd, const cdt_Block* block, const cdt_Time* toc, BlockToTextFunc blockToTextFunc) {
	size_t len;
	wchar_t* text = blockToTextFunc(block, toc, &len);
	if (!text) return false;

	bool result = clipboard_putText(wnd, text, len);
	HeapFree(GetProcessHeap(), 0, text);
	return result;
}
