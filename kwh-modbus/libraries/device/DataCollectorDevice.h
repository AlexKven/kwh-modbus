#pragma once
#include "Device.h"
#include "../timeManager/TimeManager.h"

class DataCollectorDevice : public Device
{
private_testable:
	static inline bool verifyTimeScaleAndSize(TimeScale timeScale, byte dataPacketSize);

protected_testable:
	bool _accumulateData;
	TimeScale _timeScale;
	byte _dataPacketSize;
	byte* _dataBuffer = nullptr;

	virtual bool readDataPoint(uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) = 0;
	virtual bool init(bool accumulateData, TimeScale timeScale, byte dataPacketSize);

public:
	word getType();

	virtual bool readData(uint32_t startTime, word numPoints, byte page,
		byte* buffer, word bufferSize, byte maxPoints, byte &outDataPointsCount, byte &outPagesRemaining, byte &outDataPointSize);

	static bool getDataCollectorDeviceTypeFromParameters(bool accumulateData, TimeScale timeScale, byte dataPacketSize, word &deviceType);
	static bool getParametersFromDataCollectorDeviceType(word deviceType, bool &accumulateData, TimeScale &timeScale, byte &dataPacketSize);
};