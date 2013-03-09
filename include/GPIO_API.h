#ifndef __GPIO_API_H__
#define __GPIO_API_H__

HANDLE InitGPIOAPI();
void DeinitGPIOAPI( HANDLE );
BOOL SetGPIOPin( HANDLE, DWORD PinNumber );
BOOL ClearGPIOPin( HANDLE, DWORD PinNumber );
BOOL SetInputGPIOPin( HANDLE, DWORD PinNumber );
BOOL SetOutputGPIOPin( HANDLE, DWORD PinNumber );
BOOL ReadGPIOPin( HANDLE, DWORD PinNumber );

#endif  // __GPIO_API_H__

