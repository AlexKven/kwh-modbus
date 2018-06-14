#pragma once

#include "../noArduino/arduinofunctions.h"
#include "../noArduino/arduinomacros.h"

template<typename TBase>
class MockPublicModbus : public TBase {
protected:
	virtual void addReg(word address, word value = 0)
	{
		_addReg(address, value);
	}

	virtual bool Reg(word address, word value)
	{
		return _Reg(address, value);
	}

	virtual word Reg(word address)
	{
		return _Reg(address);
	}

	virtual bool resetFrame(word byteLength)
	{
		return _resetFrame(byteLength);
	}

	virtual byte* getFramePtr()
	{
		return _getFramePtr();
	}

	virtual word getFrameLength()
	{
		return _getFrameLength();
	}

	virtual bool setFrameReg(word address, word value)
	{
		return _setFrameReg(address, value);
	}
public:
	virtual void _addReg(word address, word value = 0)
	{
		return TBase::addReg(address, value);
	}

	virtual bool _Reg(word address, word value)
	{
		return TBase::Reg(address, value);
	}

	virtual word _Reg(word address)
	{
		return TBase::Reg(address);
	}
	
	void _receivePDU(byte* frame)
	{
		receivePDU(frame);
	}

	virtual bool _resetFrame(word byteLength)
	{
		return TBase::resetFrame(byteLength);
	}

	virtual byte* _getFramePtr()
	{
		return TBase::getFramePtr();
	}

	virtual word _getFrameLength()
	{
		return TBase::getFrameLength();
	}

	bool _resetFrameRegs(word numRegs)
	{
		return resetFrameRegs(numRegs);
	}

	word _getFrameReg(word address)
	{
		return getFrameReg(address);
	}

	virtual bool _setFrameReg(word address, word value)
	{
		return TBase::setFrameReg(address, value);
	}

	byte _getReply()
	{
		return _reply;
	}

	void _setReply(byte value)
	{
		_reply = value;
	}
};