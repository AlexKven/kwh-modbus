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

uint32_t TimeManager::getClock()
{
	return (uint32_t)((_curTime - _clockSet) / 1000000) + _initialClock;
}

void TimeManager::setClock(uint32_t clock)
{
	_initialClock = clock;
	_clockSet = _curTime;
}