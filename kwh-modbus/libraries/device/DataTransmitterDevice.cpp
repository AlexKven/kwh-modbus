#include "DataTransmitterDevice.h"

bool DataTransmitterDevice::isDataTransmitterDeviceType(word deviceType)
{
	return ((deviceType >> 14) == 2);
}