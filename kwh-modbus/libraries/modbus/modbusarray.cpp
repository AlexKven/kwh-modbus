#include "ModbusArray.h"

bool ModbusArray::getRegIndex(word address, word & index)
{
	if (address < _registerStart ||
		address >= _registerStart + _registerCount)
		return false;
	index = address - _registerStart;
	return true;
}

word ModbusArray::Reg(word address)
{
	word index;
	if (!getRegIndex(address, index))
		return 0;
	return _registerArray[index];
}

bool ModbusArray::resetFrame(word byteLength)
{
	if (byteLength > Modbus::getFrameLength())
		return false;
	_frameLength = byteLength;
	return true;
}

word ModbusArray::getFrameLength()
{
	return _frameLength;
}

void ModbusArray::init(word * registerArray, word registerStart, word registerCount, word maxFrameLength)
{
	_registerArray = registerArray;
	_registerStart = registerStart;
	_registerCount = registerCount;
	Modbus::resetFrame(maxFrameLength);
}

bool ModbusArray::validRange(word startReg, word numReg)
{
	return (startReg >= _registerStart &&
		numReg <= _registerCount - startReg + _registerStart);
}


bool ModbusArray::Reg(word address, word value)
{
	word index;
	if (!getRegIndex(address, index))
		return false;
	_registerArray[index] = value;
	return true;
}