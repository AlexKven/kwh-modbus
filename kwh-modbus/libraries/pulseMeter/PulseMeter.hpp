#pragma once
#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#else
#include "../arduinoMacros/arduinoMacros.h"
#endif

#include "../denseShiftBuffer/DenseShiftBuffer.hpp"
#include "../device/DataCollectorDevice.h"

template<typename T, int B>
class PulseMeter : public DataCollectorDevice
{
protected_testable:
	DenseShiftBuffer<T, B> *_dataBuffer = nullptr;
	uint64_t _usPeriod;
	int _bufferCapacity = 0;
	double _unitsPerPulse = 1;

	volatile T _pulseCount = 0;
	uint64_t _lastUpdateTime = 0;
	uint32_t _lastUpdateClock = 0;

private:
	using DataCollectorDevice::init;

public:
	bool readDataPoint(uint32_t time, byte quarterSecondOffset,
		byte* dataBuffer, byte dataSizeBits)
	{
		//if (time > _lastUpdateClock || (time == _lastUpdateClock && quarterSecondOffset > 0))
		//	return false; // Future data point requested; clock needs to be updated
		uint32_t timeDiff = _lastUpdateClock - time; // timeDiff is how many seconds in the past the request is for
		if (timeDiff < 0x80000000 && timeDiff > 0x400000)
			// Quick way to distinguish between rollover and requesting data from the future
			return false;
		timeDiff *= (uint32_t)1000; // timeDiff is now in millliseconds
		timeDiff += 250 * quarterSecondOffset; // exclusively for MS250 timescale
		timeDiff /= TimeManager::getPeriodFromTimeScale(_timeScale); // timeDiff is now number of periods before now
		if (_dataBuffer->getNumStored() > timeDiff)
		{
			T result = _dataBuffer->get(timeDiff);
			BitFunctions::copyBits<T, byte, int>(&result, dataBuffer, 0, 0, dataSizeBits);
			return true;
		}
		return false;
	}

	virtual T getPulseCount()
	{
		return _pulseCount;
	}

	virtual void incrementPulseCount()
	{
		_pulseCount++;
	}

	virtual void zeroPulseCount()
	{
		_pulseCount = 0;
	}

	virtual void periodElapsed()
	{
		T pulseCount = getPulseCount();
		zeroPulseCount();
		T energyUnits = (double)pulseCount * _unitsPerPulse;
		_dataBuffer->push(energyUnits);
	}

	void init(int bufferCapacity, TimeScale timeScale,
		double unitsPerPulse, byte dataPacketSize)
	{
		DataCollectorDevice::init(false, timeScale, dataPacketSize);
		_bufferCapacity = bufferCapacity;
		_unitsPerPulse = unitsPerPulse;

		_dataBuffer = new DenseShiftBuffer<T, B>(_bufferCapacity);
		_usPeriod = TimeManager::getPeriodFromTimeScale(_timeScale) * 1000;
	}

	void setup()
	{
		_lastUpdateTime = getTimeSource()->getCurTime();
		zeroPulseCount();
	}

	uint64_t curTime;
	void loop()
	{
		curTime = getTimeSource()->getCurTime();
		if (_lastUpdateTime - curTime >= _usPeriod)
		{
			periodElapsed();
			_lastUpdateTime = curTime;
			_lastUpdateClock = getTimeSource()->getClock();
		}
	}
};