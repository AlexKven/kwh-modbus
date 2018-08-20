#include "../modbusSlave/ModbusSlave.hpp"
#include "../resilientTask/ResilientTask.hpp"

template<typename TBase, typename S>
class ResilientModbus : public ResilientTask<S>, public TBase
{
private_testable:
	word _rFrameMaxLength = 0;
	word _rFrameLength = 0;
	byte *_rFrame = nullptr;

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
	TaskStatus begin()
	{
		if (!this->copyFrame())
			return TaskFailure;
		this->send();
		return TaskInProgress;
	}

	TaskStatus check()
	{
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

	TaskStatus retry()
	{
		if (!this->revertFrame())
			return TaskFailure;
		this->send();
		return TaskInProgress;
	}

public:
	ResilientModbus()
	{

	}

	ResilientModbus(word maxFrameLength)
	{
		_rFrameMaxLength = maxFrameLength;
	}
};