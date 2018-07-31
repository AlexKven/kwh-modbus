#include "pch.h"
#include "WindowsSystemFunctions.h"
#include "WindowsFunctions.h"


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
	//return GetTickCount64();
	return 0;
}

handle WindowsSystemFunctions::createThread(void (*func)(void*), void* param)
{
	//auto data = new thread_data();
	//data->func = func;
	//data->param = param;
	//return CreateThread(NULL, 0, thread_func, data, 0, 0);
	return _windows.Windows_CreateThread(func, param);
}

wait_status WindowsSystemFunctions::waitForThreads(handle threads ...)
{
	handle *handles = new handle[2];
	return waitForThreads(handles, 0, 2, threads);
}

wait_status WindowsSystemFunctions::waitForThreads(handle *threads, int num, int size, handle head)
{
	if (num < size)
	{
		handle *newThreads = new handle[num];
		for (int i = 0; i < size; i++)
			newThreads[i] = threads[i];
		delete[] threads;
		threads = newThreads;
		size = num;
	}
	threads[num] = head;
	num++;
	auto res = _windows.Windows_WaitForMultipleObjects(num, threads, true, 300000);
	//auto res = WaitForMultipleObjects(num, threads, true, 300000);
	delete[] threads;
	//if (res < WAIT_ABANDONED_0)
	//	return wait_success;
	//else if (res < WAIT_TIMEOUT)
	//	return wait_abandoned;
	//else if (res < WAIT_FAILED)
	//	return wait_timeout;
	//else
	//	return wait_failed;
	return res;
}

wait_status WindowsSystemFunctions::waitForThreads(handle * threads, int num, int size, handle head, handle tail ...)
{
	if (num < size)
	{
		handle *newThreads = new handle[num * 1.5];
		for (int i = 0; i < size; i++)
			newThreads[i] = threads[i];
		delete[] threads;
		threads = newThreads;
		size = num;
	}
	threads[num] = head;
	num++;
	return waitForThreads(threads, num, size, tail);
}
