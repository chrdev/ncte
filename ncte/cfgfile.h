#pragma once

#include <Windows.h>

#include <stdbool.h>


typedef void* CfgFile;

bool
cfgfile_openForWriting(CfgFile* file, const wchar_t* path);

bool
cfgfile_openForReading(CfgFile* file, const wchar_t* path);

void
cfgfile_close(CfgFile file);

bool
cfgfile_write(CfgFile file, const wchar_t* key, const wchar_t* value, int valueWidth);

bool
cfgfile_read(CfgFile file, const char** key, int* keyLen, const char** value, int* valueLen);

bool
cfgfile_readWcs(CfgFile file, const wchar_t** key, int* keyLen, const wchar_t** value, int* valueLen);

void
cfgfile_setCodepage(CfgFile file, UINT codepage);

UINT
cfgfile_getCodePage(CfgFile file);

CfgFile
cfgfile_fromWcs(const wchar_t* text, int len);

bool
cfgfile_isWcs(CfgFile file);
