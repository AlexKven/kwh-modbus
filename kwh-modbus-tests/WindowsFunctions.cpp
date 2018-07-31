#include "pch.h"
#include "WindowsFunctions.h"
#include <Windows.h>

struct thread_data
{
	void(*func)(void*);
	void* param;
};

DWORD WINAPI thread_func(LPVOID lpParameter)
{
	thread_data *data = (thread_data*)lpParameter;
	data->func(data->param);
	return 0;
}

WindowsFunctions::WindowsFunctions()
{
}

void WindowsFunctions::Windows_Sleep(int millis)
{
	Sleep(millis);
}

handle WindowsFunctions::Windows_CreateThread(void(*func)(void*), void* param)
{
	auto data = new thread_data();
	data->func = func;
	data->param = param;
	return CreateThread(NULL, 0, thread_func, data, 0, 0);
}

wait_status WindowsFunctions::Windows_WaitForMultipleObjects(int num, handle* objects, bool waitAll, int timeout)
{
	auto res = WaitForMultipleObjects(num, objects, waitAll, timeout);
	if (res < WAIT_ABANDONED_0)
		return wait_success;
	else if (res < WAIT_TIMEOUT)
		return wait_abandoned;
	else if (res < WAIT_FAILED)
		return wait_timeout;
	else
		return wait_failed;
}