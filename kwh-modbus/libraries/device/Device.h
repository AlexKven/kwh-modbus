#pragma once
#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#else

#endif

class Device
{
public:
	virtual word getType() = 0;
	virtual bool readData(uint32_t startTime, word numPoints, byte page,
		byte* buffer, word bufferSize, byte maxPoints, byte &outDataPointsCount, byte &outPagesRemaining, byte &outDataPointSize);
};