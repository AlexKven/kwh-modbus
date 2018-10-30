#include "TimeManager.h"

void TimeManager::tick(uint64_t curTime)
{
	_prevTime = _curTime;
	_curTime = curTime;
}

uint64_t TimeManager::getCurTime()
{
	return _curTime;
}

uint64_t TimeManager::getTimeSincePenultimateTick()
{
	return _curTime - _prevTime;
}

uint32_t TimeManager::getTimeCodeForTime(TimeScale timeScale, uint32_t clock) // 0 for current time
{
	if (clock == 0)
		clock = getClock();
	auto period = getPeriodFromTimeScale(timeScale); // this is milliseconds
	auto periodsElapsed = (uint32_t)((uint64_t)clock * 1000 / period);
	return periodsElapsed;
}

// not tested because it is very trivial
uint32_t TimeManager::getPeriodFromTimeScale(TimeScale timeScale)
{
	switch (timeScale)
	{
	case TimeScale::ms250:
		return 250;
	case TimeScale::sec1:
		return 1000;
	case TimeScale::sec15:
		return 15000;
	case TimeScale::min1:
		return 60000;
	case TimeScale::min10:
		return 600000;
	case TimeScale::min30:
		return 1800000;
	case TimeScale::hr1:
		return 3600000;
	case TimeScale::hr24:
		return 86400000;
	default:
		return 0; // bad timescale
	}
}

uint32_t TimeManager::getClock()
{
	return (uint32_t)((_curTime - _clockSet) / 1000000) + _initialClock;
}

void TimeManager::setClock(uint32_t clock)
{
	_initialClock = clock;
	_clockSet = _curTime;
}