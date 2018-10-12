#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#else
#include "../arduinoMacros/arduinoMacros.h"
#endif

enum TaskStatus : byte
{
	TaskNotStarted = 0,
	TaskInProgress = 1,
	TaskFailure = 2,
	TaskAttemptTimeOut = 3,
	TaskComplete = 4,
	TaskTimeOut = 5,
	TaskFullyAttempted = 6,
	TaskFatal = 7
};

template<typename S>
class ResilientTask
{
private_testable:
	TaskStatus _status = TaskNotStarted;
	int _currentTries = 0;
	int _totalCycles = 0;
	unsigned long long _lastBeginTime = 0;
	unsigned long long _initialBeginTime = 0;
	int _maxTries = 0;
	unsigned long long _maxTimeMicros = 0;
	unsigned long long _maxTimePerTryMicros = 0;
	unsigned long long _minTimePerTryMicros = 0;
	S *_system = nullptr;

protected_testable:
	virtual TaskStatus begin() = 0;
	virtual TaskStatus check() = 0;
	virtual TaskStatus retry() = 0;
	virtual void disposed() {}
	virtual void onReset() {}
	virtual TaskStatus onAutoStatusChange(TaskStatus status)
	{
		return status;
	}

	bool reachedTryLimit()
	{
		if (_maxTries == 0)
			return false;
		return _currentTries >= _maxTries;
	}

	bool reachedMaxTime(unsigned long long curTime)
	{
		if (_maxTimeMicros == 0)
			return false;
		return curTime - _initialBeginTime >= _maxTimeMicros;
	}

	bool reachedMaxTimePerTry(unsigned long long curTime)
	{
		if (_maxTimePerTryMicros == 0)
			return false;
		return curTime - _lastBeginTime >= _maxTimePerTryMicros;
	}

	bool reachedMinTimePerTry(unsigned long long curTime)
	{
		return curTime - _lastBeginTime >= _minTimePerTryMicros;
	}
public:
	virtual bool work()
	{
		bool tryAgain = false;
		bool previouslyCompleted = (_status >= 4);
		do
		{
			tryAgain = false;
			unsigned long long curTime = 0;
			if (_system != nullptr)
				curTime = _system->micros();
			switch (_status)
			{
			case TaskNotStarted:
				_status = begin();
				_currentTries = 1;
				_initialBeginTime = curTime;
				_lastBeginTime = curTime;
				break;
			case TaskInProgress:
				if (reachedMaxTime(curTime))
				{
					if (_status < 4)
					{
						_status = onAutoStatusChange(TaskTimeOut);
						tryAgain = true;
					}
				}
				else if (reachedMaxTimePerTry(curTime))
				{
					if (_status < 4)
					{
						_status = onAutoStatusChange(TaskAttemptTimeOut);
						tryAgain = true;
					}
				}
				else
				{
					_status = check();
				}
				break;
			case TaskFailure:
				if (reachedTryLimit())
				{
					_status = check();
					if (_status < 4)
					{
						_status = onAutoStatusChange(TaskFullyAttempted);
						tryAgain = true;
					}
				}
				else if (reachedMaxTime(curTime))
				{
					if (_status < 4)
					{
						_status = onAutoStatusChange(TaskTimeOut);
						tryAgain = true;
					}
				}
				else if (reachedMinTimePerTry(curTime))
				{
					_lastBeginTime = curTime;
					_currentTries++;
					_status = retry();
				}
				break;
				case TaskAttemptTimeOut:
					if (reachedTryLimit())
					{
						_status = check();
						if (_status < 4)
						{
							_status = onAutoStatusChange(TaskFullyAttempted);
							tryAgain = true;
						}
					}
					else if (reachedMaxTime(curTime))
					{
						if (_status < 4)
						{
							_status = onAutoStatusChange(TaskTimeOut);
							tryAgain = true;
						}
					}
					else if (reachedMinTimePerTry(curTime))
					{
						_lastBeginTime = curTime;
						_currentTries++;
						_status = retry();
					}
					break;
				break;
			}

		} while (tryAgain);
		if (_status >= 4 && !previouslyCompleted)
		{
			disposed();
			_totalCycles++;
		}
		return (_status >= 4);
	}

	virtual bool work(TaskStatus &statusOut)
	{
		auto result = work();
		statusOut = _status;
		return result;
	}

	virtual TaskStatus getStatus()
	{
		return _status;
	}

	virtual int getMaxTries()
	{
		return _maxTries;
	}

	virtual void setMaxTries(int value)
	{
		_maxTries = value;
	}

	virtual unsigned long long getMaxTimeMicros()
	{
		return _maxTimeMicros;
	}

	virtual void setMaxTimeMicros(unsigned long long value)
	{
		_maxTimeMicros = value;
	}

	virtual unsigned long long getMaxTimePerTryMicros()
	{
		return _maxTimePerTryMicros;
	}

	virtual void setMaxTimePerTryMicros(unsigned long long value)
	{
		_maxTimePerTryMicros = value;
	}

	virtual unsigned long long getMinTimePerTryMicros()
	{
		return _minTimePerTryMicros;
	}

	virtual void setMinTimePerTryMicros(unsigned long long value)
	{
		_minTimePerTryMicros = value;
	}

	virtual void setSystem(S *system)
	{
		_system = system;
	}

	// Not tested
	virtual void reset()
	{
		_status = TaskNotStarted;
		_currentTries = 0;
		_totalCycles = 0;
		_lastBeginTime = 0;
		_initialBeginTime = 0;
		onReset();
	}
};