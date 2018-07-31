#pragma once
#include "../kwh-modbus/interfaces/ISystemFunctions.h"
#include "WindowsFunctions.h"

class WindowsSystemFunctions
	: ISystemFunctions
{
private:
	wait_status waitForThreads(handle *threads, int num, int size, handle head);
	wait_status waitForThreads(handle *threads, int num, int size, handle head, handle tail...);
	WindowsFunctions _windows;
public:
	WindowsSystemFunctions();
	~WindowsSystemFunctions();
	virtual void pinMode(unsigned char pin, unsigned char mode);
	virtual void digitalWrite(unsigned char pin, unsigned char value);
	void delay(unsigned long ms);
	void delayMicroseconds(unsigned long us);
	unsigned long millis();
	handle createThread(void(*func)(void*), void* param);
	wait_status waitForThreads(handle threads...);
};