#pragma once

#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#endif

class TimeManager
{
private_testable:
	uint64_t _curTime = 0;
	uint64_t _prevTime = 0;

	uint32_t _initialClock = 0;
	uint64_t _clockSet = 0;

protected_testable:
	virtual void tick(uint64_t curTime)
	{
		_prevTime = _curTime;
		_curTime = curTime;
	}

	virtual uint64_t getCurTime()
	{
		return _curTime;
	}

	virtual uint64_t getTimeSincePenultimateTick()
	{
		return _curTime - _prevTime;
	}

public:
	virtual uint32_t getClock()
	{
		return (uint32_t)((_curTime - _clockSet) / 1000000) + _initialClock;
	}

	virtual void setClock(uint32_t clock)
	{
		_initialClock = clock;
		_clockSet = _curTime;
	}
};