#include "kmode_dll.h"
#include "KMDriverAPI.h"
#include "pkfuncs.h"
#include "kernelmodestuff.h"
#include "StructDll1.h"

bool InitKMDriver() {
	return WP7InitDriverAPI() && WP7LoadDriver(L"Drivers\\BuiltIn\\KMDriver");
}
bool DeinitKMDriver() {
	return WP7DeinitDriverAPI() && WP7UnloadDriver(L"KMD1:");
}

PROCESSOR_INFO GetProcInfo() {
	VOID_STRUCT vs = { };
	PROCESSOR_INFO procinfo = { };
	wchar_t szTraceMsg[100];
	int status = RunInKernelMode(L"KGetProcInfo", (void * )&vs,
			sizeof(vs), (void * )&procinfo,
			sizeof(procinfo), szTraceMsg, sizeof(szTraceMsg) - 2);
	return procinfo;
}
