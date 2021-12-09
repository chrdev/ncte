#include <SDKDDKVer.h>
#include <windows.h>
//#include <Objbase.h>
//#pragma comment(lib, "Ole32.lib")
#include <CommCtrl.h>
#pragma comment(lib, "Comctl32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <stdbool.h>

#define NCTE_MAINWND_H_IMP
#include "mainwnd.h"


static inline bool
initCommonControls(void) {
    INITCOMMONCONTROLSEX iccx = {
        .dwSize = sizeof(INITCOMMONCONTROLSEX),
        .dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES
    };
    return InitCommonControlsEx(&iccx);
}

int WINAPI
wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ LPWSTR cmdLine, _In_ int cmdShow)
{
    //CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (!initCommonControls()) return 3;

    HWND wnd = mainwnd_create();
    if (!wnd) return 2;

    ShowWindow(wnd, cmdShow);

    MSG msg;
    BOOL bRes;
    while (bRes = GetMessage(&msg, NULL, 0, 0)) {
        if (bRes == -1)
            return 1;
        if (IsDialogMessage(wnd, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //CoUninitialize();
    return (int)msg.wParam;
}
