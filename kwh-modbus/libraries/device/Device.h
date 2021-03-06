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
private:
	TimeManager *_timeSource = nullptr;

public:
	virtual void setup();
	virtual void loop();
	virtual word getType() = 0;
	virtual bool readData(uint32_t startTime, word numPoints, byte page,
		byte* buffer, word bufferSize, byte maxPoints, byte &outDataPointsCount, byte &outPagesRemaining, byte &outDataPointSize);
	virtual void setClock(uint32_t clock);
	virtual RecieveDataStatus prepareReceiveData(word nameLength, byte* name, uint32_t startTime,
		byte dataPointSize, TimeScale dataTimeScale, word dataPointsCount, byte &outDataPointsPerPage);
	virtual RecieveDataStatus receiveDeviceData(byte dataPointsInPage, byte dataPointSize,
		TimeScale timesScale, byte pageNumber, byte* dataPoints);
	virtual uint32_t masterRequestTime();
	virtual void deviceNotResponding(word nameLength, byte* name, uint32_t reportTime);
	virtual void setTimeSource(TimeManager *timeSource);
	virtual TimeManager* getTimeSource();

	static bool isDataTransmitterDeviceType(word deviceType);
	static bool isTimeServerDeviceType(word deviceType);
};