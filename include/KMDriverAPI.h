#ifndef __KM_DRIVER_API_H__
#define __KM_DRIVER_API_H__

#define __KM_DRIVER_API __declspec(dllimport)
//#include "GPIO_Driver.h"
#ifdef __cplusplus
extern "C" {
#endif

//BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved );

HANDLE WP7InitDriverAPI(wchar_t * szDriverName = TEXT("KMD1:"));
BOOL WP7DeinitDriverAPI( HANDLE hDriver = NULL);
HANDLE WP7ListRunningDrivers(wchar_t * szDriverName = TEXT("KMD1:"));
BOOL WP7LoadDriver(wchar_t * szRegPath = L"Drivers\\BuiltIn\\KMDriver", wchar_t * szDriverName = TEXT("KMD1:"));
BOOL WP7UnloadDriver(wchar_t * szDriverName = TEXT("KMD1:"));
BOOL WP7RunInKernelMode(wchar_t * szDllName, wchar_t * szFunctionName, void * lpInStructurePointer, unsigned long nInStructureLength, void * lpOutStructurePointer, unsigned long nOutStructureLength, wchar_t * szTraceOut, unsigned long nTraceOutLength);

BOOL WP7SetGPIOPin( DWORD PinNumber );
BOOL WP7ClearGPIOPin( DWORD PinNumber );
BOOL WP7SetInputGPIOPin( DWORD PinNumber );
BOOL WP7SetOutputGPIOPin( DWORD PinNumber );
BOOL WP7ReadGPIOPin( DWORD PinNumber );
#ifdef __cplusplus
}
#endif

#endif //__KM_DRIVER_API_H__
