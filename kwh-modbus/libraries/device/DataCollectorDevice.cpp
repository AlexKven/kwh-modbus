#include "DataCollectorDevice.h"

inline bool DataCollectorDevice::verifyTimeScaleAndSize(TimeScale timeScale, byte dataPacketSize)
{
	if ((byte)timeScale > 7)
		return false;
	if (dataPacketSize > 63 || dataPacketSize == 0)
		return false;
	return true;
}

inline byte DataCollectorDevice::bitsToBytes(byte bits)
{
	byte result = bits / 8;
	if (bits % 8 == 0)
		return result;
	else
		return result + 1;
}

word DataCollectorDevice::getType()
{
	word result;
	getDataCollectorDeviceTypeFromParameters(_accumulateData, _timeScale, _dataPacketSize, result);
	return result;
}

bool DataCollectorDevice::init(bool accumulateData, TimeScale timeScale, byte dataPacketSize)
{
	if (!verifyTimeScaleAndSize(timeScale, dataPacketSize))
		return false;
	_accumulateData = accumulateData;
	_timeScale = timeScale;
	_dataPacketSize = dataPacketSize;
	if (_dataBuffer != nullptr)
		delete[] _dataBuffer;
	_dataBuffer = new byte(bitsToBytes(_dataPacketSize));
}

bool DataCollectorDevice::readData(uint32_t startTime, word numPoints, byte page, byte * buffer, word bufferSize, byte & outDataPointsCount, byte & outPagesRemaining)
{
	uint32_t period = TimeManager::getPeriodFromTimeScale(_timeScale) / 1000; // Seconds
	auto numPointsPerPage = (bufferSize * 8) / (numPoints * _dataPacketSize);
	auto startPoint = page * numPointsPerPage;
	outPagesRemaining = (numPoints - startPoint - 1) / numPointsPerPage;
	auto curNumPoints = numPointsPerPage < numPoints ? numPointsPerPage : numPoints;
	if (outPagesRemaining == 0)
		curNumPoints = numPoints - startPoint;

	uint32_t curTime;
	byte quarterSecondOffset = 0;

	if (_timeScale == TimeScale::ms250)
	{
		curTime = startTime + startPoint / 4;
		quarterSecondOffset = startPoint % 4;
	}
	else
	{
		curTime = startTime + startPoint * period;
	}

	for (int i = 0; i < curNumPoints; i++)
	{

		if (_timeScale == TimeScale::ms250)
		{
			quarterSecondOffset = (quarterSecondOffset + 1) % 4;
			if (quarterSecondOffset == 0)
				curTime++;
		}
		else
		{
			curTime += period;
		}
	}
	return true;
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

bool DataCollectorDevice::getParametersFromDataCollectorDeviceType(word deviceType, bool & accumulateData, TimeScale & timeScale, byte & dataPacketSize)
{
	if (deviceType & 0xFF != 0)
		// device type is not padded with zeros
		return false;
	deviceType >>= 4;
	dataPacketSize = deviceType & 0x3F;
	deviceType >>= 6;
	timeScale = (TimeScale)(deviceType & 0x07);
	deviceType >>= 3;
	accumulateData = deviceType & 0x01;
	deviceType >>= 1;
	// device type is not fundamentally a data collector if it doesn't start with 01
	return deviceType == 0x01;
}
