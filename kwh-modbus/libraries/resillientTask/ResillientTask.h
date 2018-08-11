#include "../../noArduino/TestHelpers.h"
#ifdef NO_ARDUINO
#include "../../noArduino/ArduinoMacros.h"
#endif

enum TaskStatus : byte
{
	TaskNotStarted = 0,
	TaskInProgress = 1,
	TaskFailure = 2,
	TaskComplete = 3,
	TaskTimeOut = 4,
	TaskFullyAttempted = 5,
	TaskFatal = 6
};

template<typename S>
class ResillientTask<S>
{
private_testable:
	TaskStatus _status = TaskNotStarted;
	int _currentTries = 0;
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
	virtual TaskStatus onAutoStatusChange(TaskStatus status)
	{
		return status;
	}

	bool reachedTryLimit()
	{
		if (_maxTries = 0)
			return false;
		return _maxTries >= _currentTries;
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
	bool work()
	{
		bool tryAgain = false;
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
				_status = check();
				if (reachedMaxTime(curTime))
				{
					if (_status < 3)
					{
						_status = TaskTimeOut;
					}
				}
				if (reachedTryLimit())
				{
					_status = check();
					if (_status < 3)
					{
						_status = TaskFullyAttempted;
					}
				}
				break;
			}

		} while (tryAgain);
		return (_status >= 3);
	}

	TaskStatus getStatus()
	{
		return _status;
	}

	int getMaxTries()
	{
		return _maxTries;
	}

	void setMaxTries(int value)
	{
		_maxTries = value;
	}

	unsigned long long getMaxTimeMicros()
	{
		return _maxTimeMicros;
	}

	void setMaxTimeMicros(unsigned long long value)
	{
		_maxTimeMicros = value;
	}

	unsigned long long getmaxTimePerTryMicros()
	{
		return _maxTimePerTryMicros;
	}

	void setmaxTimePerTryMicros(unsigned long long value)
	{
		_maxTimePerTryMicros = value;
	}

	unsigned long long getminTimePerTryMicros()
	{
		return _minTimePerTryMicros;
	}

	void setminTimePerTryMicros(unsigned long long value)
	{
		_minTimePerTryMicros = value;
	}
};