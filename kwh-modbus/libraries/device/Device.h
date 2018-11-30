#pragma once
#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#else
#include "../arduinoMacros/arduinoMacros.h"
#endif

#include "../timeManager/TimeManager.h"

enum class RecieveDataStatus
{
	success,
	notSupported,
	nameTooLong,
	timeRequested,
	failure
};

class Device
{
public:
	virtual word getType() = 0;
	virtual bool readData(uint32_t startTime, word numPoints, byte page,
		byte* buffer, word bufferSize, byte maxPoints, byte &outDataPointsCount, byte &outPagesRemaining, byte &outDataPointSize);
	virtual void setClock(uint32_t clock);
	virtual RecieveDataStatus receiveDeviceName(word nameLength, byte* name);
	virtual RecieveDataStatus receiveDeviceData(uint32_t startTime, TimeScale timeScale, byte dataPointSize,
		word startOffset, word pointCount, byte* dataPoints);
};