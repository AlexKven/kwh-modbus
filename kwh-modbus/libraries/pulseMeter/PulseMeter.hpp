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

private:
	using DataCollectorDevice::init;

public:
	bool readDataPoint(uint32_t time, byte quarterSecondOffset,
		byte* dataBuffer, byte dataSizeBits)
	{
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
		while (_lastUpdateTime - curTime >= _usPeriod)
		{
			periodElapsed();
			_lastUpdateTime += _usPeriod;
		}
	}
};