#include "Device.h"

bool Device::readData(uint32_t startTime, word numPoints, byte page, byte * buffer, word bufferSize, byte maxPoints, byte & outDataPointsCount, byte & outPagesRemaining, byte &outDataPointSize)
{
	return false;
}

void Device::setClock(uint32_t clock)
{
}

void Device::receiveDeviceName(word nameLength, byte * name)
{
}

void Device::receiveDeviceData(uint32_t startTime, TimeScale timeScale, byte dataPointSize, word startOffset, word pointCount, byte * dataPoints)
{
}
