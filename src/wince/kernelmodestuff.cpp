#include "kmode_dll.h"
#include "KMDriverAPI.h"
#include "pkfuncs.h"
#include "kernelmodestuff.h"
#include "StructDll1.h"
#include "output.h"

bool InitKMDriver() {
	return WP7LoadDriver() && WP7InitDriverAPI();
}
bool DeinitKMDriver() {
	return WP7DeinitDriverAPI() && WP7UnloadDriver();
}

//Example function for later use
PROCESSOR_INFO GetProcInfo() {
	VOID_STRUCT vs = { };
	PROCESSOR_INFO procinfo = { };
	wchar_t szTraceMsg[100];
	int status = RunInKernelMode(L"KGetProcInfo", (void * )&vs,
			sizeof(vs), (void * )&procinfo,
			sizeof(procinfo), szTraceMsg, sizeof(szTraceMsg) - 2);
	Output("GetProcInfo returned %i", status);
	return procinfo;
}
