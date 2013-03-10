#ifndef _KERNELMODESTUFF_H
#define _KERNELMODESTUFF_H
#include "pkfuncs.h"
#include "KMDriverApi.h"
bool InitKMDriver();
bool DeinitKMDriver();
PROCESSOR_INFO GetProcInfo();
#define RunInKernelMode(FunctionName, InStructurePointer, InStructureLength, OutStructurePointer, OutStructureLength, OutTrace, OutTraceLen) WP7RunInKernelMode(L"kmodedll.dll", FunctionName, InStructurePointer, InStructureLength, OutStructurePointer, OutStructureLength, OutTrace, OutTraceLen)

#endif /* _KERNELMODESTUFF_H */
