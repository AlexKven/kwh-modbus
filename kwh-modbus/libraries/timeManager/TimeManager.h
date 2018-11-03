#pragma once

#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#endif

enum class TimeScale
{
	ms250 = 0,
	sec1,
	sec15,
	min1,
	min10,
	min30,
	hr1,
	hr24
};

class TimeManager
{
private_testable:
	uint64_t _curTime = 0;
	uint64_t _prevTime = 0;

	uint32_t _initialClock = 0;
	uint64_t _clockSet = 0;

protected_testable:
	virtual void tick(uint64_t curTime);
	virtual uint64_t getCurTime();
	virtual uint64_t getTimeSincePenultimateTick();

	virtual uint32_t getTimeCodeForClock(TimeScale timeScale, uint32_t clock = 0);
	virtual uint32_t getClockForTimeCode(TimeScale timeScale, uint32_t timeCode, uint32_t referenceClock = 0);

public:
	virtual uint32_t getClock();
	virtual void setClock(uint32_t clock);

	static uint32_t getPeriodFromTimeScale(TimeScale timeScale);
};