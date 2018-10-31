#include "DataCollectorDevice.h"

inline bool DataCollectorDevice::verifyTimeScaleAndSize(TimeScale timeScale, byte dataPacketSize)
{
	if ((byte)timeScale > 7)
		return false;
	if (dataPacketSize > 127 || dataPacketSize == 0)
		return false;
	return true;
}

word DataCollectorDevice::getType()
{
	return word();
}

bool DataCollectorDevice::init(bool accumulateData, TimeScale timeScale, byte dataPacketSize)
{
	if (!verifyTimeScaleAndSize(timeScale, dataPacketSize))
		return false;
	_accumulateData = accumulateData;
	_timeScale = timeScale;
	_dataPacketSize = dataPacketSize;
}

bool DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(bool accumulateData, TimeScale timeScale, byte dataPacketSize, word & deviceType)
{
	if (!verifyTimeScaleAndSize(timeScale, dataPacketSize))
		return false;

	deviceType = 1;
	deviceType <<= 1;
	deviceType += accumulateData;
	deviceType <<= 3;
	deviceType += (byte)timeScale;
	deviceType <<= 6;
	deviceType += dataPacketSize;
	deviceType <<= 4;
	return true;
}

bool DataCollectorDevice::getParametersFromDataCollectorDeviceType(bool & accumulateData, TimeScale & timeScale, byte & dataPacketSize, word deviceType)
{
	return false;
}
