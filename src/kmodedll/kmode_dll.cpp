#include <windows.h> // SystemParametersInfo
#include "pkfuncs.h" // KernelIoControl
#include "kmode_dll.h"

int DllEntry(HINSTANCE DllInstance, ULONG Reason, LPVOID Reserved) {
	return TRUE;
}

void TRACE_SAVE(TCHAR *format, ...) {
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

	TRACE_SAVE(L"GetProcInfo(0x%x, %d, 0x%X, %d)\n", InStructurePointer,
			InStructureLength, OutStructurePointer, OutStructureLength);

	TRACE_SAVE(
			L"sizeof(VOID_STRUCT) = %d, sizeof(PROCESSOR_INFO) = %d)\n",
			sizeof(VOID_STRUCT), sizeof(PROCESSOR_INFO));

	if (InStructurePointer && OutStructurePointer
			&& sizeof(VOID_STRUCT) == InStructureLength
			&& sizeof(PROCESSOR_INFO) == OutStructureLength) {
		PROCESSOR_INFO * pinfo = (PROCESSOR_INFO *) OutStructurePointer;

		// Try to lookup processor type.
		DWORD rsize;
		memset(&pinfo, sizeof(pinfo), 0);
		int ret = KernelIoControl(IOCTL_PROCESSOR_INFORMATION, NULL, 0, &pinfo,
				sizeof(pinfo), &rsize);
		if (ret) {
			TRACE_SAVE(L"pinfo filled, GetProcInfo returns ERROR_SUCCESS\n");

			return ERROR_SUCCESS;
		}
		TRACE_SAVE(L"pinfo not filled, GetProcInfo returns ERROR_SUCCESS\n");
		return ERROR;
	}
	return ERROR_INVALID_PARAMETER;
}
