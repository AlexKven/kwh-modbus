#pragma once
#include "../interfaces/isystemfunctions.h"

class SystemFunctions 
	: ISystemFunctions
{
public:
	virtual void pinMode(unsigned char pin, unsigned char mode);
	virtual void digitalWrite(unsigned char pin, unsigned char value);
	void delay(unsigned long ms);
	void delayMicroseconds(unsigned long us);

	SystemFunctions();
};

