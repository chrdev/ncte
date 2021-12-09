#include "cdrom.h"

#include <winioctl.h> // needed by some following files
#define _NTSCSI_USER_MODE_
#include <scsi.h> // CDB6GENERIC_LENGTH, SCSIOP_TEST_UNIT_READY
#include <ntddscsi.h> // SCSI_PASS_THROUGH
#include <ntddcdrm.h> // CDROM_READ_TOC_EX_FORMAT_CDTEXT

#include <stdlib.h> // offsetof


typedef struct SPT {
	SCSI_PASS_THROUGH;
	SENSE_DATA sense;
	//WORD aligner_;
}SPT;
static_assert(offsetof(SPT, sense) % sizeof(DWORD) == 0, "offset of SPT.sense must align to DWORD");
static_assert(sizeof(SPT) % sizeof(DWORD) == 0, "sizeof SPT must align to DWORD");

typedef struct SPTD {
	SCSI_PASS_THROUGH_DIRECT;
	SENSE_DATA sense;
	//WORD aligner_;
}SPTD;
static_assert(offsetof(SPTD, sense) % sizeof(DWORD) == 0, "offset of SPTD.sense must align to DWORD");
static_assert(sizeof(SPTD) % sizeof(DWORD) == 0, "sizeof SPTD must align to DWORD");


// On errors, see Annex A - Error Lists, mmc3r10g.pdf

static cd_Error
getError(UCHAR scsiStatus, const SENSE_DATA* sense) {
	if (scsiStatus == SCSISTAT_GOOD) return cd_kGood;
	if (sense->SenseKey == 0x05 && sense->AdditionalSenseCode == 0x24) return cd_kNoText;
	if (sense->SenseKey == 0x02 && sense->AdditionalSenseCode == 0x3A) return cd_kNoCd;
	return cd_kOther;
}

static inline bool
isUnitReady(HANDLE f, int* error) {
	SPT spt = {
		.Length = sizeof(SCSI_PASS_THROUGH),
		.CdbLength = CDB6GENERIC_LENGTH,
		.SenseInfoLength = sizeof(SENSE_DATA),
		.DataIn = SCSI_IOCTL_DATA_IN,
		.TimeOutValue = 2,
		.SenseInfoOffset = offsetof(SPT, sense),
		.Cdb[0] = SCSIOP_TEST_UNIT_READY,
	};

	DWORD cb = 0;
	BOOL ok = DeviceIoControl(
		f, IOCTL_SCSI_PASS_THROUGH,
		&spt, sizeof(SCSI_PASS_THROUGH),
		&spt, sizeof(SPT),
		&cb, FALSE
	);
	if (!ok) {
		*error = cd_kOther;
		return false;
	}
	*error = getError(spt.ScsiStatus, &spt.sense);
	return *error == cd_kGood;
}

uint8_t*
cd_dumpText(HANDLE h, int* size, int* error) {
	enum { kBinSizeMax = 18 * 256 * 8 + 4 };
	uint8_t* bin = HeapAlloc(GetProcessHeap(), 0, kBinSizeMax);
	if (!bin) {
		*error = cd_kOther;
		return NULL;
	}

	SPTD sptd = {
		.Length = sizeof(SCSI_PASS_THROUGH_DIRECT),
		.CdbLength = CDB10GENERIC_LENGTH,
		.SenseInfoLength = sizeof(SENSE_DATA),
		.DataIn = SCSI_IOCTL_DATA_IN,
		.DataTransferLength = kBinSizeMax,
		.TimeOutValue = 10,
		.DataBuffer = bin,
		.SenseInfoOffset = offsetof(SPTD, sense),
		.Cdb[0] = SCSIOP_READ_TOC,
		.Cdb[2] = CDROM_READ_TOC_EX_FORMAT_CDTEXT,
		.Cdb[7] = kBinSizeMax >> 8,
		.Cdb[8] = kBinSizeMax & 0xFF,
	};

	DWORD cb = 0;
	BOOL ok = DeviceIoControl(
		h, IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptd, sizeof(SCSI_PASS_THROUGH_DIRECT),
		&sptd, sizeof(SPTD),
		&cb, FALSE
	);
	if (!ok) {
		*error = cd_kOther;
		HeapFree(GetProcessHeap(), 0, bin);
		return NULL;
	}
	*error = getError(sptd.ScsiStatus, &sptd.sense);
	if (*error != cd_kGood) {
		HeapFree(GetProcessHeap(), 0, bin);
		return NULL;
	}

	*size = sptd.DataTransferLength;
	return bin;
}

bool
cd_saveTextDump(const wchar_t* path, const uint8_t* bin, int size) {
	HANDLE f = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE) return false;

	DWORD cb;
	BOOL ok = WriteFile(f, bin + sizeof(DWORD), size - sizeof(DWORD), &cb, NULL);
	if (!ok) goto cleanup;
	uint8_t nul = 0;
	ok = WriteFile(f, &nul, 1, &cb, NULL);

cleanup:;
	CloseHandle(f);
	return ok;
}

HANDLE
cd_open(wchar_t driveLetter, int* error) {
	wchar_t path[] = L"\\\\.\\X:";
	path[4] = driveLetter;
	HANDLE h = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		*error = cd_kOther;
		return INVALID_HANDLE_VALUE;
	}

	if (!isUnitReady(h, error))
	{
		CloseHandle(h);
		return INVALID_HANDLE_VALUE;
	}
	return h;
}
