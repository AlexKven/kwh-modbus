#pragma once
#include "../libraries/resilientModbusMaster/ResilientModbusMaster.hpp"

template<typename TSerial, typename TSystemFunctions, typename TBase>
class MockableResilientModbusMaster : public ResilientModbusMaster<TSerial, TSystemFunctions, TBase>
{
public:
	virtual bool work()
	{
		return ResilientTask<TSystemFunctions>::work();
	}

	virtual bool setRequest_ReadRegisters(byte recipientId, word regStart, word regCount)
	{
		return ModbusMaster<TSerial, TSystemFunctions, TBase>::setRequest_ReadRegisters(recipientId, regStart, regCount);
	}
};