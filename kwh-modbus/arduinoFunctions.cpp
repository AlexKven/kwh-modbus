#include "arduinoFunctions.h"

char ArduinoFunctions::bitRead(unsigned char value, unsigned char bit)
{
	return (char)((value >> bit) & 1);
}

void ArduinoFunctions::bitSet(unsigned char & value, unsigned char bit)
{
	unsigned char mask = 1 << bit;
	value = value | mask;
}

void ArduinoFunctions::bitClear(unsigned char & value, unsigned char bit)
{
	unsigned char mask = 1 << bit;
	value = value & ~mask;
}
