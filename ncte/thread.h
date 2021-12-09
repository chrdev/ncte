#pragma once

#include <Windows.h>

#include <stdbool.h>


typedef struct Thread {
	HANDLE handle;
	DWORD id;
	HWND buddyWnd;
}Thread;

typedef struct TheThreads {
	Thread formStatusUpdater;
}TheThreads;

extern TheThreads theThreads;

DWORD WINAPI
formStatusUpdaterProc(__in  LPVOID params);

static inline bool
threads_init(HWND navWnd) {
	theThreads.formStatusUpdater.handle = CreateThread(NULL, 0, formStatusUpdaterProc, navWnd, 0, &theThreads.formStatusUpdater.id);
	if (!theThreads.formStatusUpdater.handle) return false;
	for (; !PostThreadMessage(theThreads.formStatusUpdater.id, WM_NULL, 0, 0); Sleep(0));
	return true;
}

static inline void
threads_uninit(void) {
	PostThreadMessage(theThreads.formStatusUpdater.id, WM_QUIT, 0, 0);
	WaitForSingleObject(theThreads.formStatusUpdater.handle, INFINITE);
	CloseHandle(theThreads.formStatusUpdater.handle);
}
