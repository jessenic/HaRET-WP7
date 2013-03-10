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

//void TRACE_SAVE(TCHAR *format, ...) {
//	TCHAR szMessage[3000];
//	FILE * stream;
//
//	va_list argptr;
//	va_start(argptr, format);
//	vswprintf(szMessage, format, argptr);
//	va_end(argptr);
//
//	stream = fopen("\\DllOut.txt", "a");
//	if (stream != 0) {
//		fputws(szMessage, stream);
//		fclose(stream);
//	}
//}
static const int MAXOUTBUF = 2 * 1024;
static const int PADOUTBUF = 32;

static HANDLE outputLogfile;

static void writeLog(const char *msg, uint32 len) {
	if (!outputLogfile)
		return;
	DWORD nw;
	WriteFile(outputLogfile, msg, len, &nw, 0);
}

static char SourcePath[200];
void fnprepare(const char *ifn, char *ofn, int ofn_max) {
	// Don't translate absolute file names
	if (ifn[0] == '\\')
		strncpy(ofn, ifn, ofn_max);
	else
		_snprintf(ofn, ofn_max, "%s%s", SourcePath, ifn);
}
// Request output to be copied to a local log file.
static int openLogFile(const char *vn) {
	char fn[200];
	fnprepare(vn, fn, sizeof(fn));
	if (outputLogfile)
		CloseHandle(outputLogfile);
	wchar_t wfn[200];
	mbstowcs(wfn, fn, ARRAY_SIZE(wfn));
	outputLogfile = CreateFile(wfn, GENERIC_WRITE, FILE_SHARE_READ, 0,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_WRITE_THROUGH*/,
			0);
	if (!outputLogfile)
		return -1;
	// Append to log
	SetFilePointer(outputLogfile, 0, NULL, FILE_END);
	return 0;
}

// Ask wince to write log fully to disk (like an fsync).
void flushLogFile() {
	if (outputLogfile)
		FlushFileBuffers(outputLogfile);
}

// Close a previously opened log file.
static void closeLogFile() {
	if (outputLogfile)
		CloseHandle(outputLogfile);
	outputLogfile = NULL;
}
static int convertNL(char *outbuf, int maxlen, const char *inbuf, int len) {
	// Convert CR to CR/LF since telnet requires this
	const char *s = inbuf, *s_end = &inbuf[len];
	char *d = outbuf;
	char *d_end = &outbuf[maxlen - 3];
	while (s < s_end && d < d_end) {
		if (*s == '\n')
			*d++ = '\r';
		*d++ = *s++;
	}

	// A trailing tab character is an indicator to not add in a
	// trailing newline - in all other cases add the newline.
	if (d > outbuf && d[-1] == '\t') {
		d--;
	} else {
		*d++ = '\r';
		*d++ = '\n';
	}

	*d = '\0';
	return d - outbuf;
}
void Output(const char *format, ...) {
	// Check for error indicator (eg, format starting with "<0>")
	int code = 7;
	if (format[0] == '<' && format[1] >= '0' && format[1] <= '9'
			&& format[2] == '>') {
		code = format[1] - '0';
		format += 3;
	}

	// Format output string.
	char rawbuf[MAXOUTBUF];
	va_list args;
	va_start(args, format);
	int rawlen = vsnprintf(rawbuf, sizeof(rawbuf) - PADOUTBUF, format, args);
	va_end(args);

	// Convert newline characters
	char buf[MAXOUTBUF];
	int len = convertNL(buf, sizeof(buf), rawbuf, rawlen);

	openLogFile("\\kmode_dll.txt");
	writeLog(buf, len);
	closeLogFile();
}

//Example function for later use
unsigned long KGetProcInfo(unsigned char * InStructurePointer,
		unsigned long InStructureLength, unsigned char * OutStructurePointer,
		unsigned long OutStructureLength) {

	Output("GetProcInfo(0x%x, %d, 0x%X, %d)\n", InStructurePointer,
			InStructureLength, OutStructurePointer, OutStructureLength);

	Output("sizeof(VOID_STRUCT) = %d, sizeof(PROCESSOR_INFO) = %d)\n",
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
			Output("pinfo filled, GetProcInfo returns ERROR_SUCCESS\n");

			return ERROR_SUCCESS;
		}
		Output("pinfo not filled, GetProcInfo returns ERROR_SUCCESS\n");
		return ERROR;
	}
	return ERROR_INVALID_PARAMETER;
}
