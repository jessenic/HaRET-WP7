#include "StructDll1.h"

int DllEntry(HINSTANCE DllInstance, ULONG Reason, LPVOID Reserved);

unsigned long KernelDllFn1(unsigned char * InStructurePointer, unsigned long InStructureLength, unsigned char * OutStructurePointer, unsigned long OutStructureLength);

#define RunKernelDllFn1(InStructurePointer, InStructureLength, OutStructurePointer, OutStructureLength, OutTrace, OutTraceLen) WP7RunInKernelMode(L"SimpleKernelDll1.dll", L"KernelDllFn1", InStructurePointer, InStructureLength, OutStructurePointer, OutStructureLength, OutTrace, OutTraceLen)
