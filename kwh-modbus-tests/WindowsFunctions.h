#pragma once

enum wait_status
{
	wait_success = 0,
	wait_abandoned = 1,
	wait_timeout = 2,
	wait_failed = 3
};

typedef void *handle;

class WindowsFunctions
{
public:
	WindowsFunctions();
	void Windows_Sleep(int millis);
	handle Windows_CreateThread(void(*func)(void*), void* param);
	wait_status Windows_WaitForMultipleObjects(int num, handle* objects, bool waitAll, int timeout);
	unsigned long long RelativeMicroseconds();
};