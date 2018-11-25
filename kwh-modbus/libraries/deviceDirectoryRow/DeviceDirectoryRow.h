#pragma once
#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#endif

struct DeviceDirectoryRow
{
	byte slaveId;
	word deviceNumber;
	word deviceType;
	word deviceRegs;

	DeviceDirectoryRow()
	{
		slaveId = 0;
		deviceType = 0;
		deviceNumber = 0;
		deviceRegs = 0;
	}

	DeviceDirectoryRow(byte _slaveId, word _deviceNumber, word _deviceType, word _deviceRegs)
	{
		slaveId = _slaveId;
		deviceType = _deviceType;
		deviceNumber = _deviceNumber;
		deviceRegs = _deviceRegs;
	}
};

static bool operator==(const DeviceDirectoryRow& lhs, const DeviceDirectoryRow& rhs)
{
	return (lhs.slaveId == rhs.slaveId) &&
		(lhs.deviceNumber == rhs.deviceNumber) &&
		(lhs.deviceType == rhs.deviceType) &&
		(lhs.deviceRegs == rhs.deviceRegs);
}

static bool operator!=(const DeviceDirectoryRow& lhs, const DeviceDirectoryRow& rhs)
{
	return (lhs.slaveId != rhs.slaveId) ||
		(lhs.deviceNumber != rhs.deviceNumber) ||
		(lhs.deviceType != rhs.deviceType) ||
		(lhs.deviceRegs != rhs.deviceRegs);
}