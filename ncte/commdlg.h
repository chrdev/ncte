#pragma once

#include <Windows.h>
#include <commdlg.h>
#pragma comment(lib, "Comdlg32.lib")

#include <stdbool.h>


typedef enum FileType {
	filetype_kNone,
	filetype_kBin,
	filetype_kSheet,
	filetype_kDrive,
	filetype_kDriveAndDump,
}FileType;

FileType
commdlg_getOpenPath(HWND owner, wchar_t* path, DWORD size);

bool
commdlg_getSaveBinPath(HWND owner, wchar_t* path, DWORD size);
