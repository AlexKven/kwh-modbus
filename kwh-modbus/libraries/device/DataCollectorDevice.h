#include "Device.h"
#include "../timeManager/TimeManager.h"

class DataCollectorDevice : public Device
{
private_testable:
	bool _accumulateData;
	TimeScale _timeScale;
	byte _dataPacketSize;
	byte *_dataBuffer;

	static inline bool verifyTimeScaleAndSize(TimeScale timeScale, byte dataPacketSize);
	static inline byte bitsToBytes(byte bits);

protected_testable:
	virtual bool readDataPoint(uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte bufferSizeBits) = 0;

public:
	word getType();
	bool init(bool accumulateData, TimeScale timeScale, byte dataPacketSize);

	virtual bool readData(uint32_t startTime, word numPoints, byte page,
		byte* buffer, word bufferSize, byte &outDataPointsCount, byte &outPagesRemaining);

	static bool getDataCollectorDeviceTypeFromParameters(bool accumulateData, TimeScale timeScale, byte dataPacketSize, word &deviceType);
	static bool getParametersFromDataCollectorDeviceType(word deviceType, bool &accumulateData, TimeScale &timeScale, byte &dataPacketSize);
};