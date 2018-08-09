/*
    Modbus.cpp - Source for Modbus Base Library
    Copyright (C) 2014 André Sarmento Barbosa
*/
#include "Modbus.h"
#include <stdlib.h>

Modbus::Modbus() {
}

bool Modbus::resetFrame(word byteLength)
{
	_length = byteLength;
	_frame = (byte*)malloc(_length);
	if (_frame)
		return true;
	else
		return false;
}

byte* Modbus::getFramePtr()
{
	return _frame;
}

word Modbus::getFrameLength()
{
	return _length;
}

bool Modbus::resetFrameRegs(word numRegs, byte begin)
{
	return resetFrame(numRegs * 2 + begin);
}

word Modbus::getFrameReg(word address, byte begin)
{
	word* wordPtr = (word*)(getFramePtr() + begin); 
	return wordPtr[address];
}

bool Modbus::setFrameReg(word address, word value, byte begin)
{
	if (address * 2 + begin >= getFrameLength())
		return false;
	word* wordPtr = (word*)(getFramePtr() + begin);
	wordPtr[address] = value;
	return true;
}

bool Modbus::Hreg(word offset, word value) {
    return Reg(offset, value);
}

word Modbus::Hreg(word offset) {
    return Reg(offset);
}

word Modbus::revWord(word input)
{
	return (input & 0x00FF) << 8 | input >> 8;
}