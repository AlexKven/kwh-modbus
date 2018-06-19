#pragma once
#include "modbus.h"

#ifdef NO_ARDUINO
#include "../../noArduino/arduinofunctions.h"
#include "../../noArduino/arduinomacros.h"
#else
#include "Arduino.h"
#endif

template<typename TBase>
class ModbusArray : public TBase {
private:
	word *_registerArray;
	word _registerStart;
	word _registerCount;
	word _frameLength = 0;

	bool getRegIndex(word address, word &index)
	{
		if (address < _registerStart ||
			address >= _registerStart + _registerCount)
			return false;
		index = address - _registerStart;
		return true;
	}

protected_testable:
	bool Reg(word address, word value)
	{
		word index;
		if (!getRegIndex(address, index))
			return false;
		_registerArray[index] = value;
		return true;
	}

	word Reg(word address)
	{
		word index;
		if (!getRegIndex(address, index))
			return 0;
		return _registerArray[index];
	}

	bool resetFrame(word byteLength)
	{
		if (byteLength > TBase::getFrameLength())
			return false;
		_frameLength = byteLength;
		return true;
	}

	word getFrameLength()
	{
		return _frameLength;
	}

public:
	void init(word *registerArray, word registerStart, word registerCount, word maxFrameLength)
	{
		_registerArray = registerArray;
		_registerStart = registerStart;
		_registerCount = registerCount;
		TBase::resetFrame(maxFrameLength);
	}

	bool validRange(word startReg, word numReg)
	{
		return (startReg >= _registerStart &&
			numReg < _registerCount - startReg + _registerStart);
	}
};