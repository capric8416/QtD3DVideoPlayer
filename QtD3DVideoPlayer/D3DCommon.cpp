#include "D3DCommon.h"

#include <stdarg.h>
#include <stdio.h>



const char * ConvertToChars(const wchar_t *src, UINT CodePage)
{
	int size = (int)wcslen(src);

	char *dst;
	int length = WideCharToMultiByte(CodePage, 0, src, size, NULL, 0, NULL, NULL);
	dst = new char[length + 1];
	WideCharToMultiByte(CodePage, 0, src, size, dst, length, NULL, NULL);
	dst[length] = '\0';
	
	return dst;
}


const wchar_t * ConvertToWideChars(const char *src, UINT CodePage)
{
	int size = (int)strlen(src);

	wchar_t *dst;
	int length = MultiByteToWideChar(CodePage, 0, src, size, NULL, 0);
	dst = new wchar_t[length + 1];
	MultiByteToWideChar(CodePage, 0, src, size, dst, length);
	dst[length] = '\0';
	
	return dst;
}



// Trace like printf
inline void TraceA(const char *format, ...)
{
	va_list args = NULL;
	va_start(args, format);
	size_t length = _vscprintf(format, args) + 1;
	char  *buffer = new char[length];
	_vsnprintf_s(buffer, length, length, format, args);
	va_end(args);
	OutputDebugStringA(buffer);
	delete[] buffer;
}

// Trace like printf
inline void TraceW(const wchar_t *format, ...)
{
	va_list args = NULL;
	va_start(args, format);
	size_t length = _vscwprintf(format, args) + 1;
	wchar_t *buffer = new wchar_t[length];
	_vsnwprintf_s(buffer, length, length, format, args);
	va_end(args);
	OutputDebugStringW(buffer);
	delete[] buffer;
}


// 100��������
int SleepNanoseconds(LONGLONG hundreds)
{
	LARGE_INTEGER t;

	t.QuadPart = -hundreds;

	// Create a waitable timer.
	HANDLE timer = CreateWaitableTimer(nullptr, TRUE, L"WaitableTimer");
	if (!timer)
	{
		TRACEW(LOG_LEVEL_WARNING, "CreateWaitableTimer failed code 0x%x", GetLastError());
		return -1;
	}

	// Set a timer to wait for ...
	// ������ʾ���ʱ�䣬������ʾ����ʱ��
	// ��λΪ100ns
	if (!SetWaitableTimer(timer, &t, 0, nullptr, nullptr, 0))
	{
		TRACEW(LOG_LEVEL_WARNING, "SetWaitableTimer failed code 0x%x", GetLastError());
		return -2;
	}

	// Wait for the timer to achieve
	if (WaitForSingleObject(timer, INFINITE) != WAIT_OBJECT_0)
	{
		TRACEW(LOG_LEVEL_WARNING, "WaitForSingleObject failed code 0x%x", GetLastError());
	}

	CloseHandle(timer);

	return 0;
}
