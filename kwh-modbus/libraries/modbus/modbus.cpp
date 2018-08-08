/*
    Modbus.cpp - Source for Modbus Base Library
    Copyright (C) 2014 Andr� Sarmento Barbosa
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

bool Modbus::resetFrameRegs(word numRegs, bool begin)
{
	return resetFrame(numRegs * 2 + begin);
}

word Modbus::getFrameReg(word address)
{
	word* wordPtr = (word*)(getFramePtr()); 
	return wordPtr[address];
}

bool Modbus::setFrameReg(word address, word value)
{
	if (address * 2 >= getFrameLength())
		return false;
	word* wordPtr = (word*)(getFramePtr());
	wordPtr[address] = value;
	return true;
}

bool Modbus::Hreg(word offset, word value) {
    return Reg(offset, value);
}

word Modbus::Hreg(word offset) {
    return Reg(offset);
}


