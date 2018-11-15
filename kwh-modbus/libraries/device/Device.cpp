#include "Device.h"

bool Device::readData(uint32_t startTime, word numPoints, byte page, byte * buffer, word bufferSize, byte maxPoints, byte & outDataPointsCount, byte & outPagesRemaining, byte &outDataPointSize)
{
	return false;
}

void Device::setClock(uint32_t clock)
{
}

RecieveDataStatus Device::receiveDeviceName(word nameLength, byte * name)
{
	return RecieveDataStatus::notSupported;
}

RecieveDataStatus Device::receiveDeviceData(uint32_t startTime, TimeScale timeScale, byte dataPointSize, word startOffset, word pointCount, byte * dataPoints)
{
	return RecieveDataStatus::notSupported;
}
