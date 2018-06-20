#pragma once
#include "../interfaces/isystemfunctions.h"

class SystemFunctions : ISystemFunctions
{
public:
	void pinMode(unsigned char pin, unsigned char mode);
	void digitalWrite(unsigned char pin, unsigned char value);
};

