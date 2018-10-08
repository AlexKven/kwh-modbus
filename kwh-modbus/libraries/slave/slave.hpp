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
class Slave
{
private_testable:
	const byte _majorVersion = 1;
	const byte _minorVersion = 0;
	word _deviceNameLength;
	word _deviceCount;
	byte **_deviceNames = nullptr;
	Device **_devices = nullptr;
	SlaveState _state = sIdle;

	S *_system;
	M *_modbus;

	virtual bool setOutgoingState()
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
			word deviceNum = _modbus->Hreg(2);
			ENSURE(_modbus->Hreg(1, deviceNum));
			ENSURE(_modbus->Hreg(2, _devices[deviceNum]->getType()));
			byte* name = _deviceNames[deviceNum];
			word index = 0;
			for (int i = 0; i < _deviceNameLength; i += 2)
			{
				word reg = name[i] << 8;
				if (i < _deviceNameLength - 1)
				{
					reg = reg | name[i + 1];
				}
				ENSURE(_modbus->Hreg(3 + index, reg));
				index++;
			}
			// Commands & Messages not yet implemented
			ENSURE(_modbus->Hreg(3 + index, 0)); // Commands waiting
			ENSURE(_modbus->Hreg(4 + index, 0)); // Messages waiting
			break;
		}
		return true;
	}

	virtual bool processIncomingState(bool &requestProcessed)
	{
		requestProcessed = false;
		ENSURE(_modbus->validRange(0, 10));
		if (_modbus->Hreg(0) == sReceivedRequest)
		{
			requestProcessed = true;
			switch (_modbus->Hreg(1))
			{
			case 0:
				_state = sIdle;
				setOutgoingState();
				break;
			case 1:
				_state = sIdle;
				_modbus->setSlaveId((byte)_modbus->Hreg(2));
				setOutgoingState();
				break;
			case 2:
				_state = sDisplayDevInfo;
				setOutgoingState();
				break;
			}
		}
		return true;
	}

public:
	void config(S *system, M *modbus)
	{
		_system = system;
		_modbus = modbus;
	}

	void init(word deviceCount, word deviceNameLength, Device **devices, byte **deviceNames)
	{
		clearDevices();
		_deviceCount = deviceCount;
		_deviceNameLength = deviceNameLength;
		_deviceNames = new byte*[_deviceNameLength];
		_devices = new Device*[_deviceCount];
		for (int i = 0; i < _deviceCount; i++)
		{
			_deviceNames[i] = new byte[_deviceNameLength];
			for (int j = 0; j < _deviceNameLength; j++)
			{
				_deviceNames[i][j] = deviceNames[i][j];
			}
			_devices[i] = devices[i];
		}
	}

	// Basic initial version
	void task()
	{
		bool processed;
		bool broadcast;
		_modbus->task(processed, broadcast);
		if (processed)
			processIncomingState(processed);
	}

	void clearDevices()
	{
		if (_deviceNames != nullptr)
		{
			delete[] _deviceNames;
			_deviceNames = nullptr;
		}
		if (_devices != nullptr)
		{
			delete[] _devices;
			_devices = nullptr;
		}
		_deviceCount = 0;
	}

	Slave() { }

	~Slave()
	{
		clearDevices();
	}

	word getDeviceCount()
	{
		return _deviceCount;
	}

	word getDeviceNameLength()
	{
		return _deviceNameLength;
	}

	byte getSlaveId()
	{
		return _modbus->getSlaveId();
	}
};