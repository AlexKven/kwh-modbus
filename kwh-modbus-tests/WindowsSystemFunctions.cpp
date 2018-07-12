#include "pch.h"
#include "WindowsSystemFunctions.h"
#include <Windows.h>


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
	Sleep(ms);
}

void WindowsSystemFunctions::delayMicroseconds(unsigned long us)
{
	Sleep(us / 1000);
}