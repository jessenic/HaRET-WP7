#ifndef __GPIO_API_H__
#define __GPIO_API_H__

#define IOCTL_SET_GPIO (1)
#define IOCTL_CLEAR_GPIO (2)
#define IOCTL_READ_GPIO (3)
#define IOCTL_SET_OUTPUT_GPIO (4)
#define IOCTL_SET_INPUT_GPIO (5)

#define WP7_DLL_CALL (9)

#define MIN_BUFFER_SIZE 1

struct DLL_CALL_IN
{
	wchar_t szDllName[MAX_PATH];
	wchar_t szFunctionName[MAX_PATH];
	unsigned long dwLength;
	unsigned char lpEntryBytes[MIN_BUFFER_SIZE];
};

struct DLL_CALL_OUT
{
	wchar_t szOutTrace[MAX_PATH];
	unsigned long dwResult;
	unsigned long dwLength;
	unsigned char lpResultBytes[MIN_BUFFER_SIZE];
};

#endif // __GPIO_API_H__

