#pragma once

#include "../../noArduino/TestHelpers.h"
#ifdef NO_ARDUINO
#include "../../noArduino/ArduinoMacros.h"
#endif

#include "../device/Device.h"
#define ENSURE(statement) if (!(statement)) return false

enum SlaveState : word
{
	sIdle = 0,
	sReceivedRequest = 1,
	sDisplayDevInfo = 2,
	sDisplayDevData = 3,
	sReceivingDevData = 4,
	sDisplayDevCommand = 5,
	sReceivingDevCommand = 6,
	sDisplayDevMessage = 7,
	sDisplaySlaveMessage = 8
};

template<class M, class S>
class Master
{
private_testable:
	const byte _majorVersion = 1;
	const byte _minorVersion = 0;
	word _deviceNameLength;
	word _maxDevices;

	S *_system;
	M *_modbus;

public:
	void config(S *system, M *modbus)
	{
		_system = system;
		_modbus = modbus;
	}

	void init(word deviceNameLength, word maxDevices)
	{
		clearDevices();
		_deviceNameLength = deviceNameLength;
		_maxDevices = maxDevices;
	}

	void clearDevices()
	{
	}

	Slave() { }

	~Slave()
	{
		clearDevices();
	}

	word getDeviceNameLength()
	{
		return _deviceNameLength;
	}
};