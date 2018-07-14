#pragma once
#include "../kwh-modbus/interfaces/ISystemFunctions.h"
class WindowsSystemFunctions
	: ISystemFunctions
{
public:
	WindowsSystemFunctions();
	~WindowsSystemFunctions();
	virtual void pinMode(unsigned char pin, unsigned char mode);
	virtual void digitalWrite(unsigned char pin, unsigned char value);
	void delay(unsigned long ms);
	void delayMicroseconds(unsigned long us);
	unsigned long millis();
};