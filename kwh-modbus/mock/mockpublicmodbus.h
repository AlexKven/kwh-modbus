#pragma once

#include "../noArduino/arduinofunctions.h"
#include "../noArduino/arduinomacros.h"

template<typename TBase>
class MockPublicModbus : public TBase {
public:
	void _addReg(word address, word value = 0)
	{
		return addReg(address, value);
	}

	bool _Reg(word address, word value)
	{
		return Reg(address, value);
	}

	word _Reg(word address)
	{
		return Reg(address);
	}
	
	void _receivePDU(byte* frame)
	{
		receivePDU(frame);
	}

	bool _resetFrame(word byteLength)
	{
		return resetFrame(byteLength);
	}

	byte* _getFramePtr()
	{
		return getFramePtr();
	}

	word _getFrameLength()
	{
		return getFrameLength();
	}

	bool _resetFrameRegs(word numRegs)
	{
		return resetFrameRegs(numRegs);
	}

	word _getFrameReg(word address)
	{
		return getFrameReg(address);
	}

	bool _setFrameReg(word address, word value)
	{
		return setFrameReg(address, value);
	}
};