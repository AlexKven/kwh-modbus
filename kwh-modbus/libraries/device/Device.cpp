#include "Device.h"

void Device::setup()
{
}

void Device::loop()
{
}

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

uint32_t Device::masterRequestTime()
{
	return 0;
}

void Device::deviceNotResponding(word nameLength, byte * name, uint32_t reportTime)
{
}

void Device::setTimeSource(TimeManager *timeSource)
{
	_timeSource = timeSource;
}

TimeManager* Device::getTimeSource()
{
	return _timeSource;
}

bool Device::isDataTransmitterDeviceType(word deviceType)
{
	return ((deviceType >> 14) == 2);
}

bool Device::isTimeServerDeviceType(word deviceType)
{
	return (deviceType == 1);
}
