#ifndef __KM_DRIVER_API_H__
#define __KM_DRIVER_API_H__

#define __KM_DRIVER_API __declspec(dllimport)
//#include "GPIO_Driver.h"
#ifdef __cplusplus
extern "C" {
#endif

//BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved );

HANDLE WP7InitDriverAPI(const wchar_t * szDriverName = TEXT("KMD1:"));
BOOL WP7DeinitDriverAPI( HANDLE hDriver = NULL);
HANDLE WP7ListRunningDrivers(wchar_t * szDriverName = TEXT("KMD1:"));
BOOL WP7LoadDriver(const wchar_t * szRegPath = L"Drivers\\BuiltIn\\KMDriver", const wchar_t * szDriverName = TEXT("KMD1:"));
BOOL WP7UnloadDriver(const wchar_t * szDriverName = TEXT("KMD1:"));
BOOL WP7RunInKernelMode(const wchar_t * szDllName, const wchar_t * szFunctionName, void * lpInStructurePointer, unsigned long nInStructureLength, void * lpOutStructurePointer, unsigned long nOutStructureLength, const wchar_t * szTraceOut, unsigned long nTraceOutLength);

LPVOID WP7VirtualAlloc(LPVOID lpAddress, DWORD dwSize, DWORD flAllocationType, DWORD flProtect);
BOOL WP7VirtualFree(LPVOID lpAddress, DWORD dwSize, DWORD dwFreeType);
BOOL WP7VirtualCopy(LPVOID lpvDest, LPVOID lpvSrc, DWORD cbSize, DWORD fdwProtect);

BOOL WP7SetGPIOPin( DWORD PinNumber );
BOOL WP7ClearGPIOPin( DWORD PinNumber );
BOOL WP7SetInputGPIOPin( DWORD PinNumber );
BOOL WP7SetOutputGPIOPin( DWORD PinNumber );
BOOL WP7ReadGPIOPin( DWORD PinNumber );
#ifdef __cplusplus
}
#endif

#endif //__KM_DRIVER_API_H__
