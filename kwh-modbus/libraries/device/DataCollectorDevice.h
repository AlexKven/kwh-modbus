#include "Device.h"
#include "../timeManager/TimeManager.h"

class DataCollectorDevice : public Device
{
private_testable:
	bool _accumulateData;
	TimeScale _timeScale;
	byte _dataPacketSize;

	static inline bool verifyTimeScaleAndSize(TimeScale timeScale, byte dataPacketSize);

public:
	word getType();
	bool init(bool accumulateData, TimeScale timeScale, byte dataPacketSize);

	static bool getDataCollectorDeviceTypeFromParameters(bool accumulateData, TimeScale timeScale, byte dataPacketSize, word &deviceType);
	static bool getParametersFromDataCollectorDeviceType(bool &accumulateData, TimeScale &timeScale, byte &dataPacketSize, word deviceType);
};