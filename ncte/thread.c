#include "thread.h"

#include <assert.h>

#include "msg.h"


TheThreads theThreads;


DWORD WINAPI
formStatusUpdaterProc(__in  LPVOID params) {
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	HWND navWnd = (HWND)params;

    BOOL bRes;
    while (bRes = GetMessage(&msg, NULL, 0, 0)) {
        if (bRes == -1) ExitThread(1);
		//switch (msg.message) {
			//MSG_THREAD_HANDLE_FORMFILLED(navWnd, onFormFilled);
			//MSG_THREAD_HANDLE_SETTRACKRANGE(onSetTrackRange);
		//}
    }
	ExitThread(0);
	return 0;
}

