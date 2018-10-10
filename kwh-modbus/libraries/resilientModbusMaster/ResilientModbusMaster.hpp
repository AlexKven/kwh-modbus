#pragma once
#include "../modbusMaster/ModbusMaster.hpp"
#include "../resilientTask/ResilientTask.hpp"

template<typename TSerial, typename TSystemFunctions, typename TBase>
class ResilientModbusMaster : public ResilientTask<TSystemFunctions>, public ModbusMaster<TSerial, TSystemFunctions, TBase>
{
private_testable:
	word _rFrameMaxLength = 0;
	word _rFrameLength = 0;
	byte *_rFrame = nullptr;
	bool broadcast = false;

	bool copyFrame()
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();

		if (_rFrameMaxLength > 0)
		{
			if (length > _rFrameMaxLength)
				return false;
			_rFrameLength = length;
		}
		else
		{
			if (_rFrameLength != length)
			{
				delete[] _rFrame;
				_rFrame = new byte[length];
				if (!_rFrame)
					return false;
				_rFrameLength = length;
			}
		}
		
		for (int i = 0; i < _rFrameLength; i++)
		{
			_rFrame[i] = frame[i];
		}
		if (length > 0)
		{
			broadcast = (frame[0] == 0);
		}
		else
			broadcast = false;
		return true;
	}

	bool revertFrame()
	{
		if (!this->resetFrame(_rFrameLength))
			return false;
		byte *frame = this->getFramePtr();
		for (int i = 0; i < _rFrameLength; i++)
		{
			frame[i] = _rFrame[i];
		}
		return true;
	}

protected_testable:
	virtual TaskStatus begin()
	{
		if (!this->copyFrame())
			return TaskFailure;
		this->send();
		if (broadcast)
			return TaskComplete;
		else
			return TaskInProgress;
	}

	virtual TaskStatus check()
	{
		if (broadcast)
			return TaskComplete;
		if (!this->receive())
			return TaskInProgress;
		else
		{
			if (this->verifyResponseIntegrity())
				return TaskComplete;
			else
				return TaskFailure;
		}
	}

	virtual TaskStatus retry()
	{
		if (!this->revertFrame())
			return TaskFailure;
		this->send();
		if (broadcast)
			return TaskComplete;
		else
			return TaskInProgress;
	}

public:
	ResilientModbusMaster()
	{

	}

	ResilientModbusMaster(word maxFrameLength)
	{
		_rFrameMaxLength = maxFrameLength;
	}

	virtual bool config(TSerial* port, TSystemFunctions* system, long baud, int txPin = -1)
	{
		setSystem(system);
		return ModbusMaster<TSerial, TSystemFunctions, TBase>::config(port, system, baud, txPin);
	}
};