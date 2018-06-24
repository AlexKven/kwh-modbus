#pragma once
class ISystemFunctions
{
public:
	virtual void pinMode(unsigned char pin, unsigned char mode) = 0;
	virtual void digitalWrite(unsigned char pin, unsigned char value) = 0;
	virtual void delay(unsigned long ms) = 0;
	virtual void delayMicroseconds(unsigned long us) = 0;
	ISystemFunctions() {}
};