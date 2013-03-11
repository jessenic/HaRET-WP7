#include <windows.h> // SystemParametersInfo
#include "pkfuncs.h" // KernelIoControl
#include "kmode_dll.h"
#include <stdio.h> // vsnprintf
#include <ctype.h> // toupper
#include <windowsx.h> // Edit_SetSel
#include <commctrl.h> // TBM_SETRANGEMAX
#include "xtypes.h" // uint
//int DllEntry(HINSTANCE DllInstance, ULONG Reason, LPVOID Reserved) {
//	return TRUE;
//}

void TRACE_SAVE(const TCHAR *format, ...) {
	TCHAR szMessage[3000];
	FILE * stream;

	va_list argptr;
	va_start(argptr, format);
	vswprintf(szMessage, format, argptr);
	va_end(argptr);

	stream = fopen("\\DllOut.txt", "a");
	if (stream != 0) {
		fputws(szMessage, stream);
		fclose(stream);
	}
}

unsigned long KGetProcInfo(unsigned char * InStructurePointer,
		unsigned long InStructureLength, unsigned char * OutStructurePointer,
		unsigned long OutStructureLength) {

	TRACE_SAVE(L"GetProcInfo(0x%X, %d, 0x%X, %d)\n", InStructurePointer,
			InStructureLength, OutStructurePointer, OutStructureLength);

	if (OutStructurePointer && sizeof(PROCESSOR_INFO) == OutStructureLength) {
		PROCESSOR_INFO * pinfo = (PROCESSOR_INFO *) OutStructurePointer;

		// Try to lookup processor type.
		DWORD rsize = 0;
		memset(pinfo, sizeof(PROCESSOR_INFO), 0);
		int ret = KernelIoControl(IOCTL_PROCESSOR_INFORMATION, NULL, 0, pinfo,
				sizeof(PROCESSOR_INFO), &rsize);
		if (ret) {
			TRACE_SAVE(L"pinfo filled, GetProcInfo returns ERROR_SUCCESS\n");

			TRACE_SAVE(L"wVersion = %d\n", pinfo->wVersion);
			TRACE_SAVE(L"szProcessCore = %s\n", pinfo->szProcessCore);
			TRACE_SAVE(L"wCoreRevision = %d\n", pinfo->wCoreRevision);
			TRACE_SAVE(L"szProcessorName = %s\n", pinfo->szProcessorName);
			TRACE_SAVE(L"wProcessorRevision = %d\n", pinfo->wProcessorRevision);
			TRACE_SAVE(L"szCatalogNumber = %s\n", pinfo->szCatalogNumber);
			TRACE_SAVE(L"szVendor = %s\n", pinfo->szVendor);
			TRACE_SAVE(L"dwInstructionSet = %d\n", pinfo->dwInstructionSet);
			TRACE_SAVE(L"dwClockSpeed = %d\n", pinfo->dwClockSpeed);

			return ERROR_SUCCESS;
		}
		DWORD dwError = 0x80070000 | GetLastError();
		TRACE_SAVE(
				L"pinfo not filled, GetProcInfo returns 0x%X, needs rsize %d\n",
				dwError, rsize);

		return dwError;
	}
	return ERROR_INVALID_PARAMETER;
}
