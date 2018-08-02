#include "pch.h"
#include "WindowsSystemFunctions.h"
#include "WindowsFunctions.h"
#include <stdarg.h>

WindowsSystemFunctions::WindowsSystemFunctions()
{
}


WindowsSystemFunctions::~WindowsSystemFunctions()
{
}

void WindowsSystemFunctions::pinMode(unsigned char pin, unsigned char mode)
{
}

void WindowsSystemFunctions::digitalWrite(unsigned char pin, unsigned char value)
{
}

void WindowsSystemFunctions::delay(unsigned long ms)
{
	_windows.Windows_Sleep(ms);
}

void WindowsSystemFunctions::delayMicroseconds(unsigned long us)
{
	_windows.Windows_Sleep(us / 1000);
}

unsigned long WindowsSystemFunctions::millis()
{
	return _windows.RelativeMicroseconds() / 1000;
}

unsigned long WindowsSystemFunctions::micros()
{
	return _windows.RelativeMicroseconds();
}

handle WindowsSystemFunctions::createThread(void (*func)(void*), void* param)
{
	return _windows.Windows_CreateThread(func, param);
}

wait_status WindowsSystemFunctions::waitForThreads(int num, ...)
{
	va_list ap;
	va_start(ap, num);
	handle* threads = new handle[num];
	for (int i = 0; i < num; i++)
	{
		handle thread = va_arg(ap, handle);
		threads[i] = thread;
	}
	return _windows.Windows_WaitForMultipleObjects(num, threads, true, 300000);
}