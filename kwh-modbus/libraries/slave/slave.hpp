#pragma once

#include "../../noArduino/TestHelpers.h"
#ifdef NO_ARDUINO
#include "../../noArduino/ArduinoMacros.h"
#endif

#include "../device/Device.h"
#define ENSURE(statement) if (!(statement)) return false

enum SlaveState
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

class DeviceName
{
private:
	byte *chars;
	// Keep track of length outside this class!
public:
	DeviceName(word length)
	{
		chars = new byte[length];
	}

	byte *getChars()
	{
		return chars;
	}

	~DeviceName()
	{
		delete[] chars;
	}
};

template<class M, class S>
class Slave
{
private_testable:
	const byte _majorVersion = 1;
	const byte _minorVersion = 0;
	word _deviceNameLength;
	word _deviceCount;
	Device *_devices;
	SlaveState _state;

	S *_system;
	M *_modbus;

	bool setOutgoingState()
	{
		ENSURE(_modbus->validRange(0, 10));
		ENSURE(_modbus->Hreg(0, _state));
		switch (_state)
		{
		case sIdle:
			ENSURE(_modbus->Hreg(1, _majorVersion << 8 | _minorVersion));
			ENSURE(_modbus->Hreg(2, _deviceCount));
			ENSURE(_modbus->Hreg(3, _deviceNameLength));
			// Keep these 0 until I implement this
			ENSURE(_modbus->Hreg(4, 0));
			ENSURE(_modbus->Hreg(5, 0));
			ENSURE(_modbus->Hreg(6, 0));
			break;
		case sReceivedRequest:
			// Why would we do this?
			break;
		case sDisplayDevInfo:
			break;
		}
		return true;
	}

	bool processIncomingState()
	{
		ENSURE(_modbus->validRange(0, 10));
		ENSURE(_modbus->Hreg(0) == sReceivedRequest);
		switch (_modbus->Hreg(1))
		{
		case 0:
			_state = sIdle;
			setOutgoingState();
		case 1:
			_state = sIdle;
			_modbus->setSlaveId((byte)_modbus->Hreg(2));
			setOutgoingState();
		}
		return true;
	}

	void config(S *system, M *modbus)
	{
		_system = system;
		_modbus = modbus;
	}
public:
	Slave() { }


};