#pragma once

#include "../../noArduino/TestHelpers.h"
#ifdef NO_ARDUINO
#include "../../noArduino/ArduinoMacros.h"
#endif

#include "../device/Device.h"
#include <tuple>

#define ENSURE(statement) if (!(statement)) return false


#define STATE word;
#define RESUME(state) goto line ## state;
#define IN_STATE STATE await_state;
#define AWAIT(statement) \
line##__line__##: \
await_state = __LINE__ \
if (!statement) return false;



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

enum MasterState : byte
{
	mIdle = 0,
	mReadingSlave = 1,
	mAwaitingResponse = 2,
	mSendingData = 3,
	mAwaitingConfirmation = 4
};

template<class M, class S, class D>
class Master
{
private_testable:
	const byte _majorVersion = 1;
	const byte _minorVersion = 0;

	D *_deviceDirectory;
	S *_system;
	M *_modbus;

	unsigned long _curTime = 0;
	unsigned long _prevTime = 0;

	MasterState _state;

public:
	void config(S *system, M *modbus)
	{
		_system = system;
		_modbus = modbus;
	}

	void init(D *deviceDirectory)
	{
		_deviceDirectory = deviceDirectory;
	}

	void task()
	{
		_prevTime = _curTime;
		_curTime = _system->micros();
		if (_prevTime == 0)
			return;
	}

	byte newSlaveId;
	bool onboardNewSlaveAsync()
	{
		auto status = _modbus->getStatus()
		switch (_state)
		{
		case mIdle:
			if (status != TaskNotStarted)
			{
				_modbus->reset();
			}
			_modbus->setRequest_ReadRegisters(0, 0, 7);
			_modbus->work();
			status = _modbus->getStatus();
			if (status >= TaskInProgress)
			{
				status = mReadingSlave;
				return false;
			}
			break;
		case: mReadingSlave:
			if (status >= TaskComplete)
			{
				word[7] registers;
				if (isReadRegsResponse(7, registers))
				{
					if (registers[0] == _majorVersion << 8 | _minorVersion)
					{
						// New slave is accepted
						newSlaveId = _deviceDirectory->findFreeSlaveID();
						
					}
				}
			}
			break;
		}
		auto status = _modbus->getStatus();
		if (readPending)
		{
			if (status == TaskNotStarted)
			{
				return false;
			}
			if (status >= TaskInProgress && status <= TaskAttemptTimeOut)
			{
				_modbus->work();
				return false;
			}
			if (status >= TaskComplete)
			{
				readPending = false;
				return false;
			}
		}
		else
		{
			if (status == TaskComplete)
			{
				word[7] registers;
				if (isReadRegsResponse(7, registers))
				{
					if (registers[0] == _majorVersion << 8 | _minorVersion)
					{

					}
				}
			}
		}
	}

	Master() { }

	~Master() { }
};