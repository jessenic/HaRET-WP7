#ifndef KMODE_DLL_H
#define KMODE_DLL_H
#include "StructDll1.h"
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILDING_KMODE_DLL
#define KMODE_DLL __declspec(dllexport)
#else
#define KMODE_DLL __declspec(dllimport)
#endif


//int DllEntry(HINSTANCE DllInstance, ULONG Reason, LPVOID Reserved);

unsigned long KGetProcInfo(unsigned char * InStructurePointer, unsigned long InStructureLength, unsigned char * OutStructurePointer, unsigned long OutStructureLength);

#ifdef __cplusplus
}
#endif

#endif  // KMODE_DLL_H
