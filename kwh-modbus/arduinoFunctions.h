#pragma once
class ArduinoFunctions
{
public:
	static char bitRead(unsigned char value, unsigned char bit);
	static void bitSet(unsigned char &value, unsigned char bit);
	static void bitClear(unsigned char &value, unsigned char bit);
};

