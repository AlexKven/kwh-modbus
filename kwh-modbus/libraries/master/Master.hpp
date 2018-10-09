#pragma once

#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#endif

#include "../device/Device.h"
#include "../asyncAwait/AsyncAwait.hpp"

#define ENSURE(statement) if (!(statement)) return false

enum SearchResultCode : byte
{
	notFound = 0,
	found = 1,
	error = 2
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

protected_testable:
	virtual byte modbusGetStatus()
	{
		return _modbus->getStatus();
	}
	virtual void modbusReset()
	{
		_modbus->reset();
	}
	virtual bool modbusWork()
	{
		return _modbus->work();
	}
	virtual bool modbusSetRequest_ReadRegisters(byte recipientId, word regStart, word regCount)
	{
		return _modbus->setRequest_ReadRegisters(recipientId, regStart, regCount);
	}
	virtual bool modbusSetRequest_WriteRegisters(byte recipientId, word regStart, word regCount, word *regValues)
	{
		return _modbus->setRequest_WriteRegisters(recipientId, regStart, regCount, regValues);
	}
	virtual bool modbusSetRequest_WriteRegister(byte recipientId, word regIndex, word regValue)
	{
		return _modbus->setRequest_WriteRegister(recipientId, regIndex, regValue);
	}
	virtual word modbusGetFrameLength()
	{
		return _modbus->getFrameLength();
	}
	virtual byte* modbusGetFramePtr()
	{
		return _modbus->getFramePtr();
	}

	virtual void reportMalfunction(int line)
	{

	}

	DEFINE_CLASS_TASK(ESCAPE(Master<M, S, D>), ensureTaskNotStarted, void, VARS());
	virtual ASYNC_CLASS_FUNC(ESCAPE(Master<M, S, D>), ensureTaskNotStarted)
	{
		START_ASYNC;
		if (modbusGetStatus() != TaskNotStarted)
		{
			modbusReset();
			do
			{
				YIELD_ASYNC;
			} while (modbusGetStatus() != TaskNotStarted);
		}
		END_ASYNC;
	}

	DEFINE_CLASS_TASK(ESCAPE(Master<M, S, D>), completeModbusReadRegisters, void, VARS(ensureTaskNotStarted_Task), byte, word, word);
	virtual ASYNC_CLASS_FUNC(ESCAPE(Master<M, S, D>), completeModbusReadRegisters, byte recipientId, word regStart, word regCount)
	{
		ASYNC_VAR(0, taskNotStartedCheck);
		START_ASYNC;
		CREATE_ASSIGN_CLASS_TASK(taskNotStartedCheck, ESCAPE(Master<M, S, D>), this, ensureTaskNotStarted);
		AWAIT(taskNotStartedCheck);
		if (!modbusSetRequest_ReadRegisters(recipientId, regStart, regCount))
		{
			reportMalfunction(__LINE__);
			YIELD_ASYNC;
		}
		while (!modbusWork())
		{
			YIELD_ASYNC;
		}
		END_ASYNC;
	}

	DEFINE_CLASS_TASK(ESCAPE(Master<M, S, D>), completeModbusWriteRegisters, void, VARS(ensureTaskNotStarted_Task), byte, word, word, word*);
	virtual ASYNC_CLASS_FUNC(ESCAPE(Master<M, S, D>), completeModbusWriteRegisters, byte recipientId, word regStart, word regCount, word *regValues)
	{
		ASYNC_VAR(0, taskNotStartedCheck);
		START_ASYNC;
		CREATE_ASSIGN_CLASS_TASK(taskNotStartedCheck, ESCAPE(Master<M, S, D>), this, ensureTaskNotStarted);
		AWAIT(taskNotStartedCheck);
		if (regCount == 1)
		{
			if (!modbusSetRequest_WriteRegister(recipientId, regStart, *regValues))
			{
				reportMalfunction(__LINE__);
				YIELD_ASYNC;
			}
		}
		else
		{
			if (!modbusSetRequest_WriteRegisters(recipientId, regStart, regCount, regValues))
			{
				reportMalfunction(__LINE__);
				YIELD_ASYNC;
			}
		}
		while (!modbusWork())
		{
			YIELD_ASYNC;
		}
		END_ASYNC;
	}

	DEFINE_CLASS_TASK(ESCAPE(Master<M, S, D>), checkForNewSlaves, SearchResultCode, VARS(completeModbusReadRegisters_Task));
	virtual ASYNC_CLASS_FUNC(ESCAPE(Master<M, S, D>), checkForNewSlaves)
	{
		ASYNC_VAR(0, completeReadRegisters);
		START_ASYNC;
		CREATE_ASSIGN_CLASS_TASK(completeReadRegisters, ESCAPE(Master<M, S, D>), this, completeModbusReadRegisters, 1, 0, 7);
		AWAIT(completeReadRegisters);
		if (modbusGetStatus() == TaskComplete)
		{
			RESULT_ASYNC(SearchResultCode, found);
		}
		else if (modbusGetStatus() == TaskStatus::TaskFullyAttempted || modbusGetStatus() == TaskStatus::TaskTimeOut)
		{
			RESULT_ASYNC(SearchResultCode, notFound);
		}
		else
		{
			reportMalfunction(__LINE__);
			RESULT_ASYNC(SearchResultCode, error);
		}
		END_ASYNC;
	}

	DEFINE_CLASS_TASK(ESCAPE(Master<M, S, D>), processNewSlave, void, VARS(completeModbusReadRegisters_Task, completeModbusWriteRegisters_Task, byte, word, word, word, word[3]));
	virtual ASYNC_CLASS_FUNC(ESCAPE(Master<M, S, D>), processNewSlave)
	{
		ASYNC_VAR(0, completeReadRegisters);
		ASYNC_VAR(1, completeWriteRegisters);
		ASYNC_VAR(2, slaveId);
		ASYNC_VAR(3, i);
		ASYNC_VAR(4, numDevices);
		ASYNC_VAR(5, deviceNameLength);
		ASYNC_VAR(6, sentData);
		START_ASYNC;
		word regCount;
		word *regs;
		if (!_modbus->isReadRegsResponse(regCount, regs) || regCount != 7)
		{
			reportMalfunction(__LINE__);
			RETURN_ASYNC;
		}
		numDevices = regs[2];
		deviceNameLength = regs[3];
		if (regs[1] >> 8 < 1)
		{
			// version of slave is less than 1.0
			// reject slave due to version mismatch
		}
		if (numDevices == 0)
		{
			// reject slave due to no devices
		}
		slaveId = _deviceDirectory->findFreeSlaveID();
		for (i = 0; i < numDevices; i++)
		{
			sentData[0] = 1;
			sentData[1] = 2;
			sentData[2] = i;
			CREATE_ASSIGN_CLASS_TASK(completeWriteRegisters, ESCAPE(Master<M, S, D>), this, completeModbusWriteRegisters, 1, 0, 3, sentData);
			AWAIT(completeWriteRegisters);
			if (modbusGetStatus() != TaskComplete)
			{
				reportMalfunction(__LINE__);
				RETURN_ASYNC;
			}
			CREATE_ASSIGN_CLASS_TASK(completeReadRegisters, ESCAPE(Master<M, S, D>), this, completeModbusReadRegisters, 1, 0, 4 + (deviceNameLength + 1) / 2);
			AWAIT(completeReadRegisters);
			if (modbusGetStatus() != TaskComplete)
			{
				reportMalfunction(__LINE__);
				RETURN_ASYNC;
			}
			_modbus->isReadRegsResponse(regCount, regs);

		}
		END_ASYNC;
	}

public:
	void config(S *system, M *modbus, D *deviceDirectory)
	{
		_system = system;
		_modbus = modbus;
		_deviceDirectory = deviceDirectory;
	}

	void init(D *deviceDirectory)
	{
	}

	void task()
	{
		auto tsk = checkForNewSlaves_Task(&Master<M, S, D>::checkForNewSlaves, this);
		tsk();
		_prevTime = _curTime;
		_curTime = _system->micros();
		if (_prevTime == 0)
			return;
	}

	checkForNewSlaves_Task t1;
	processNewSlave_Task t2;

	Master() { }

	~Master() { }
};