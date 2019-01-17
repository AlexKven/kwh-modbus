#include "Device.h"

bool Device::readData(uint32_t startTime, word numPoints, byte page, byte * buffer, word bufferSize, byte maxPoints, byte & outDataPointsCount, byte & outPagesRemaining, byte &outDataPointSize)
{
	return false;
}

void Device::setClock(uint32_t clock)
{
}

RecieveDataStatus Device::prepareReceiveData(word nameLength, byte * name, uint32_t startTime, byte dataPointSize, TimeScale dataTimeScale, word dataPointsCount, byte & outDataPointsPerPage)
{
	return RecieveDataStatus::notSupported;
}

RecieveDataStatus Device::receiveDeviceData(byte dataPointsInPage, byte dataPointSize, TimeScale timesScale, byte pageNumber, byte * dataPoints)
{
	return RecieveDataStatus::notSupported;
}

void Device::deviceNotResponding(word nameLength, byte * name, uint32_t reportTime)
{
}
