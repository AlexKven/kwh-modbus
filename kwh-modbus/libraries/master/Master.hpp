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

	DEFINE_CLASS_TASK(ESCAPE(Master<M, S, D>), completeModbusReadRegisters, void, VARS(), byte, word, word);
	virtual ASYNC_CLASS_FUNC(ESCAPE(Master<M, S, D>), completeModbusReadRegisters, byte recipientId, word regStart, word regCount)
	{
		START_ASYNC;
		//AsyncClassTaskSpecific<int, int, VARS()>( c;
		modbusSetRequest_ReadRegisters(recipientId, regStart, regCount);
		while (!modbusWork())
		{
			YIELD_ASYNC;
		}
		END_ASYNC;
	}

	DEFINE_CLASS_TASK(ESCAPE(Master<M, S, D>), checkForNewSlaves, SearchResultCode, VARS(ensureTaskNotStarted_Task, completeModbusReadRegisters_Task));
	ASYNC_CLASS_FUNC(ESCAPE(Master<M, S, D>), checkForNewSlaves)
	{
		ASYNC_VAR(0, taskNotStartedCheck);
		ASYNC_VAR(1, completeReadRegisters);
		START_ASYNC;
		//completeReadRegisters = ESCAPE(Master<M, S, D>)::completeModbusReadRegisters_Task(&Master<M, S, D>::completeModbusReadRegisters, this, 2, 5, 5);
		CREATE_ASSIGN_CLASS_TASK(taskNotStartedCheck, ESCAPE(Master<M, S, D>), this, ensureTaskNotStarted);
		CREATE_ASSIGN_CLASS_TASK(completeReadRegisters, ESCAPE(Master<M, S, D>), this, completeModbusReadRegisters, 0, 0, 7);
		AWAIT(taskNotStartedCheck);
		AWAIT(completeReadRegisters);
		if (modbusGetStatus() == TaskComplete)
		{
			RESULT_ASYNC(SearchResultCode, found);
		}
		else if (modbusGetStatus() == TaskFatal || modbusGetStatus() == TaskFullyAttempted)
		{
			RESULT_ASYNC(SearchResultCode, error);
		}
		else
		{
			RESULT_ASYNC(SearchResultCode, notFound);
		}
		END_ASYNC;
	}

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
		auto tsk = checkForNewSlaves_Task(&Master<M, S, D>::checkForNewSlaves, this);
		tsk();
		_prevTime = _curTime;
		_curTime = _system->micros();
		if (_prevTime == 0)
			return;
	}

	Master() { }

	~Master() { }
};