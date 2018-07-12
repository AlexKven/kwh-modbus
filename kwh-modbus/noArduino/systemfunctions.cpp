#include "SystemFunctions.h"
#ifdef WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif

void SystemFunctions::pinMode(unsigned char pin, unsigned char mode) 
{
}

void SystemFunctions::digitalWrite(unsigned char pin, unsigned char value)
{
}

void SystemFunctions::delay(unsigned long ms)
{
#ifdef WINDOWS
	//Sleep(ms);
#else
	usleep(ms * 1000);
#endif
}

void SystemFunctions::delayMicroseconds(unsigned long us)
{
#ifdef WINDOWS
	//Sleep(us / 1000);
#else
	usleep(us);
#endif
}

SystemFunctions::SystemFunctions()
{
}
