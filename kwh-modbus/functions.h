#pragma once

unsigned char bitRead(unsigned char value, unsigned char bit)
{
	return (char)((value >> bit) & 1);
}

void bitSet(unsigned char &value, unsigned char bit)
{
	unsigned char mask = 1 << bit;
	value = value | mask;
}

void bitClear(unsigned char &value, unsigned char bit)
{
	unsigned char mask = 1 << bit;
	value = value & ~mask;
}