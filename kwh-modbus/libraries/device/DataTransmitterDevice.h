#include "Device.h"
#include "../timeManager/TimeManager.h"

class DataTransmitterDevice : public Device
{
public:
	static bool isDataTransmitterDeviceType(word deviceType);
};