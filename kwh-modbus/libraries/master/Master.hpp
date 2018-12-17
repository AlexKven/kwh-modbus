#pragma once

#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#else
#include "../arduinoMacros/arduinoMacros.h"
#endif

#include "../device/Device.h"
#include "../device/DataCollectorDevice.h"
#include "../device/DataTransmitterDevice.h"
#include "../asyncAwait/AsyncAwait.hpp"
#include "../timeManager/TimeManager.h"
#include "../deviceDirectoryRow/DeviceDirectoryRow.h"
#include "../bitFunctions/BitFunctions.hpp"

#define ENSURE(statement) if (!(statement)) return false
#define ENSURE_NONMALFUNCTION(modbus_task) if (modbus_task.result() != success) \
{ \
	reportMalfunction(__LINE__); \
	return true; \
}

enum SearchResultCode : byte
{
	notFound = 0,
	found = 1,
	badSlave = 2,
	error = 3
};

enum ModbusRequestStatus : byte
{
	success = 0,
	masterFailure = 1,
	taskFailure = 2,
	exceptionResponse = 3,
	incorrectResponseSize = 4,
	otherResponse = 5
};

template<class M, class S, class D>
class Master : public TimeManager
{
private_testable:
	const byte _majorVersion = 1;
	const byte _minorVersion = 0;
	bool _timeUpdatePending = false;
	byte *_dataBuffer = nullptr;
	word _registerBuffer[10];
	word _dataBufferSize;

	uint32_t lastUpdateTimes[8];

	D *_deviceDirectory;
	S *_system;
	M *_modbus;

	typedef Master<M, S, D> THIS_T;

protected_testable:
	virtual void reportMalfunction(int line)
	{

	}

	virtual uint32_t getPollPeriodForTimeScale(TimeScale timeScale)
	{
		switch (timeScale)
		{
		case TimeScale::ms250: return 2;
		case TimeScale::sec1: return 5;
		case TimeScale::sec15: return 60;
		case TimeScale::min1: return 120;
		case TimeScale::min10: return 1200;
		case TimeScale::min30: return 1800;
		case TimeScale::hr1: return 3600;
		case TimeScale::hr24: return 86400;
		default: return 0; // Bad timescale
		}
	}

	DEFINE_CLASS_TASK(THIS_T, ensureTaskNotStarted, void, VARS());
	ensureTaskNotStarted_Task _ensureTaskNotStarted;
	virtual ASYNC_CLASS_FUNC(THIS_T, ensureTaskNotStarted)
	{
		START_ASYNC;
		if (_modbus->getStatus() != TaskNotStarted)
		{
			_modbus->reset();
			do
			{
				YIELD_ASYNC;
			} while (_modbus->getStatus() != TaskNotStarted);
		}
		END_ASYNC;
	}
	ensureTaskNotStarted_Task &ensureTaskNotStarted()
	{
		CREATE_ASSIGN_CLASS_TASK(_ensureTaskNotStarted, THIS_T, this, ensureTaskNotStarted);
		return _ensureTaskNotStarted;
	}

	DEFINE_CLASS_TASK(THIS_T, completeModbusReadRegisters, ModbusRequestStatus, VARS(), byte, word, word);
	completeModbusReadRegisters_Task _completeModbusReadRegisters;
	virtual ASYNC_CLASS_FUNC(THIS_T, completeModbusReadRegisters, byte recipientId, word regStart, word regCount)
	{
		word* dummy0 = nullptr;
		byte dummy1 = 0;
		START_ASYNC;
		ensureTaskNotStarted();
		AWAIT(_ensureTaskNotStarted);
		if (!_modbus->setRequest_ReadRegisters(recipientId, regStart, regCount))
		{
			RESULT_ASYNC(ModbusRequestStatus, masterFailure);
		}
		while (!_modbus->work())
		{
			YIELD_ASYNC;
		}
		if (_modbus->getStatus() != TaskComplete)
		{
			RESULT_ASYNC(ModbusRequestStatus, taskFailure);
		}
		word finalRegCount;
		if (_modbus->isReadRegsResponse(finalRegCount, dummy0))
		{
			if (finalRegCount == regCount)
			{
				RESULT_ASYNC(ModbusRequestStatus, success);
			}
			else
			{
				RESULT_ASYNC(ModbusRequestStatus, incorrectResponseSize);
			}
		}
		else
		{
			if (_modbus->isExceptionResponse(dummy1, dummy1))
			{
				RESULT_ASYNC(ModbusRequestStatus, exceptionResponse);
			}
			else
			{
				RESULT_ASYNC(ModbusRequestStatus, otherResponse);
			}
		}
		END_ASYNC;
	}
	completeModbusReadRegisters_Task &completeModbusReadRegisters(byte reciptientId, word regStart, word regCount)
	{
		CREATE_ASSIGN_CLASS_TASK(_completeModbusReadRegisters, THIS_T, this, completeModbusReadRegisters, reciptientId, regStart, regCount);
		return _completeModbusReadRegisters;
	}

	DEFINE_CLASS_TASK(THIS_T, completeModbusWriteRegisters, ModbusRequestStatus, VARS(), byte, word, word, word*);
	completeModbusWriteRegisters_Task _completeModbusWriteRegisters;
	virtual ASYNC_CLASS_FUNC(THIS_T, completeModbusWriteRegisters, byte recipientId, word regStart, word regCount, word *regValues)
	{
		byte dummy;
		START_ASYNC;
		ensureTaskNotStarted();
		AWAIT(_ensureTaskNotStarted);
		if (regCount == 1)
		{
			if (!_modbus->setRequest_WriteRegister(recipientId, regStart, *regValues))
			{
				RESULT_ASYNC(ModbusRequestStatus, masterFailure);
			}
		}
		else
		{
			if (!_modbus->setRequest_WriteRegisters(recipientId, regStart, regCount, regValues))
			{
				RESULT_ASYNC(ModbusRequestStatus, masterFailure);
			}
		}
		while (!_modbus->work())
		{
			YIELD_ASYNC;
		}
		if (_modbus->getStatus() != TaskComplete)
		{
			RESULT_ASYNC(ModbusRequestStatus, taskFailure);
		}
		if (recipientId == 0)
		{
			// broadcast, we will not receive a response
			RESULT_ASYNC(ModbusRequestStatus, success);
		}
		word finalRegCount;
		if ((_modbus->isWriteRegResponse() && regCount == 1) || (_modbus->isWriteRegsResponse() && regCount != 1))
		{
			RESULT_ASYNC(ModbusRequestStatus, success);
		}
		else
		{
			if (_modbus->isExceptionResponse(dummy, dummy))
			{
				RESULT_ASYNC(ModbusRequestStatus, exceptionResponse);
			}
			else
			{
				RESULT_ASYNC(ModbusRequestStatus, otherResponse);
			}
		}
		END_ASYNC;
	}
	completeModbusWriteRegisters_Task &completeModbusWriteRegisters(byte recipientId, word regStart, word regCount, word *regValues)
	{
		CREATE_ASSIGN_CLASS_TASK(_completeModbusWriteRegisters, THIS_T, this, completeModbusWriteRegisters, recipientId, regStart, regCount, regValues);
		return _completeModbusWriteRegisters;
	}

	DEFINE_CLASS_TASK(THIS_T, checkForNewSlaves, SearchResultCode, VARS());
	checkForNewSlaves_Task _checkForNewSlaves;
	virtual ASYNC_CLASS_FUNC(THIS_T, checkForNewSlaves)
	{
		START_ASYNC;
		completeModbusReadRegisters(1, 0, 8);
		AWAIT(_completeModbusReadRegisters);
		switch (_completeModbusReadRegisters.result())
		{
		case success:
			RESULT_ASYNC(SearchResultCode, found);
		case taskFailure:
			if (_modbus->getStatus() == TaskStatus::TaskFullyAttempted || _modbus->getStatus() == TaskStatus::TaskTimeOut)
			{
				RESULT_ASYNC(SearchResultCode, notFound);
			}
			else
			{
				reportMalfunction(__LINE__);
				RESULT_ASYNC(SearchResultCode, error);
			}
		case masterFailure:
			reportMalfunction(__LINE__);
			RESULT_ASYNC(SearchResultCode, error);
		default:
			RESULT_ASYNC(SearchResultCode, badSlave);
		}
		END_ASYNC;
	}
	checkForNewSlaves_Task &checkForNewSlaves()
	{
		CREATE_ASSIGN_CLASS_TASK(_checkForNewSlaves, THIS_T, this, checkForNewSlaves);
		return _checkForNewSlaves;
	}

	DEFINE_CLASS_TASK(THIS_T, processNewSlave, void, VARS(byte, word, byte, word, word), bool);
	processNewSlave_Task _processNewSlave;
	virtual ASYNC_CLASS_FUNC(THIS_T, processNewSlave, bool justReject)
	{
		ASYNC_VAR(0, slaveId);
		ASYNC_VAR(1, i);
		ASYNC_VAR(2, numDevices);
		ASYNC_VAR(3, deviceNameLength);
		ASYNC_VAR(4, slaveRegisters)
		START_ASYNC;
		word regCount;
		word *regs;
		numDevices = 0;
		if (!_modbus->isReadRegsResponse(regCount, regs))
		{
			reportMalfunction(__LINE__);
			RETURN_ASYNC;
		}
		numDevices = regs[2];
		deviceNameLength = regs[3];
		slaveRegisters = regs[4];
		slaveId = _deviceDirectory->findFreeSlaveID();
		if ((regs[1] >> 8 < 1) ||
			(numDevices == 0) ||
			justReject ||
			(slaveId == 0))
		{
			// first case: version of slave is less than 1.0
			// reject slave due to version mismatch

			// second case: reject slave due to no devices

			// third case: we are told to simply reject the slave

			// fourth case: reject slave due to master being full and unable to accept new slaves

			_registerBuffer[0] = 1;
			_registerBuffer[1] = 1;
			_registerBuffer[2] = 255;
			completeModbusWriteRegisters(1, 0, 3, _registerBuffer);
			AWAIT(_completeModbusWriteRegisters);
			ENSURE_NONMALFUNCTION(_completeModbusWriteRegisters);
			RETURN_ASYNC;
		}
		for (i = 0; i < numDevices; i++)
		{
			_registerBuffer[0] = 1;
			_registerBuffer[1] = 2;
			_registerBuffer[2] = i;
			completeModbusWriteRegisters(1, 0, 3, _registerBuffer);
			AWAIT(_completeModbusWriteRegisters);
			ENSURE_NONMALFUNCTION(_completeModbusWriteRegisters);
			completeModbusReadRegisters(1, 0, 4 + (deviceNameLength + 1) / 2);
			AWAIT(_completeModbusReadRegisters);
			ENSURE_NONMALFUNCTION(_completeModbusReadRegisters);
			_modbus->isReadRegsResponse(regCount, regs);
			if (_deviceDirectory->addOrReplaceDevice((byte*)(regs + 3), DeviceDirectoryRow(slaveId, i, regs[2], slaveRegisters)) == -1)
			{
				_registerBuffer[0] = 1;
				_registerBuffer[1] = 1;
				_registerBuffer[2] = 255;
				completeModbusWriteRegisters(1, 0, 3, _registerBuffer);
				AWAIT(_completeModbusWriteRegisters);
				ENSURE_NONMALFUNCTION(_completeModbusWriteRegisters);
				_deviceDirectory->filterDevicesForSlave(nullptr, 0, slaveId);
				RETURN_ASYNC;
			}
		}
		_registerBuffer[0] = 1;
		_registerBuffer[1] = 1;
		_registerBuffer[2] = slaveId;
		completeModbusWriteRegisters(1, 0, 3, _registerBuffer);
		AWAIT(_completeModbusWriteRegisters);
		ENSURE_NONMALFUNCTION(_completeModbusWriteRegisters);
		_timeUpdatePending = true;
		END_ASYNC;
	}
	processNewSlave_Task &processNewSlave(bool justReject)
	{
		CREATE_ASSIGN_CLASS_TASK(_processNewSlave, THIS_T, this, processNewSlave, justReject);
		return _processNewSlave;
	}

	DEFINE_CLASS_TASK(THIS_T, readAndSendDeviceData, void, VARS(int, DeviceDirectoryRow*, byte*, bool, TimeScale, byte, uint32_t, word, byte, byte, byte, byte, byte, int, DeviceDirectoryRow*), TimeScale, uint32_t);
	readAndSendDeviceData_Task _readAndSendDeviceData;
	virtual ASYNC_CLASS_FUNC(THIS_T, readAndSendDeviceData, TimeScale maxTimeScale, uint32_t currentTime)
	{
		ASYNC_VAR_INIT(0, deviceRow_receive, 0);
		ASYNC_VAR(1, device_receive);
		ASYNC_VAR(2, device_name);
		ASYNC_VAR(3, accumulateData);
		ASYNC_VAR(4, timeScale);
		ASYNC_VAR(5, dataSize);
		ASYNC_VAR(6, readStart);
		ASYNC_VAR(7, numDataPoints);
		ASYNC_VAR(8, numReadPagesRemaining);
		ASYNC_VAR(9, curReadPage);
		ASYNC_VAR(10, numPointsInReadPage);
		ASYNC_VAR(11, curWritePage);
		ASYNC_VAR(12, numPointsInWritePage);
		ASYNC_VAR_INIT(13, deviceRow_transmit, 0);
		ASYNC_VAR(14, device_transmit);
		word regCount;
		word *regs;
		byte *dummyName;
		START_ASYNC;
		while (deviceRow_receive != -1)
		{
			device_receive = _deviceDirectory->findNextDevice(device_name, deviceRow_receive);
			if (device_receive != nullptr)
			{
				if (DataCollectorDevice::getParametersFromDataCollectorDeviceType(device_receive->deviceType, accumulateData, timeScale, dataSize))
				{
					if (timeScale <= maxTimeScale)
					{
						readStart = lastUpdateTimes[(int)timeScale];
						numDataPoints = (currentTime - readStart) * 1000 / TimeManager::getPeriodFromTimeScale(timeScale);
						curReadPage = 0;
						numReadPagesRemaining = 1;

						while (numReadPagesRemaining > 0)
						{
							numReadPagesRemaining--;
							_registerBuffer[0] = 1;
							_registerBuffer[1] = 3;
							_registerBuffer[2] = device_receive->deviceNumber;
							_registerBuffer[3] = (word)readStart;
							_registerBuffer[4] = (word)(readStart >> 16);
							_registerBuffer[5] = numDataPoints;
							_registerBuffer[6] = curReadPage + (word)((_dataBufferSize * 8 / dataSize) << 8);
							completeModbusWriteRegisters(device_receive->slaveId, 0, 7, _registerBuffer);
							AWAIT(_completeModbusWriteRegisters);
							ENSURE_NONMALFUNCTION(_completeModbusWriteRegisters);
							completeModbusReadRegisters(device_receive->slaveId, 0, 6);
							AWAIT(_completeModbusReadRegisters);
							ENSURE_NONMALFUNCTION(_completeModbusReadRegisters);
							_modbus->isReadRegsResponse(regCount, regs);
							if (regs[0] != 3)
							{
								reportMalfunction(__LINE__);
								return true;
							}
							if (regs[1] == 0)
							{
								numPointsInReadPage = (byte)regs[4];
								curReadPage = (byte)regs[5];
								numReadPagesRemaining = (byte)(regs[5] >> 8);

								completeModbusReadRegisters(device_receive->slaveId, 6,
									BitFunctions::bitsToStructs<word, word>(numDataPoints * dataSize));
								AWAIT(_completeModbusReadRegisters);
								ENSURE_NONMALFUNCTION(_completeModbusReadRegisters);
								_modbus->isReadRegsResponse(regCount, regs);

								{
									word numDataBytes = BitFunctions::bitsToBytes(numDataPoints * dataSize);
									if (numDataBytes > _dataBufferSize)
									{
										reportMalfunction(__LINE__);
										return true;
									}
									for (int i = 0; i < numDataBytes; i++)
									{
										if (i % 2 == 0)
											_dataBuffer[i] = (byte)regs[i / 2];
										else
											_dataBuffer[i] = (byte)(regs[i / 2] >> 8);
									}
								}

								deviceRow_transmit = 0;
								while (deviceRow_transmit != -1)
								{
									device_transmit = _deviceDirectory->findNextDevice(dummyName, deviceRow_transmit);
									if (device_transmit != nullptr)
									{
										if (DataTransmitterDevice::isDataTransmitterDeviceType(device_transmit->deviceType))
										{
											prepare_write:
											_registerBuffer[0] = 1;
											_registerBuffer[1] = 4;
											_registerBuffer[2] = device_transmit->deviceNumber;
											_registerBuffer[3] = _deviceDirectory->getDeviceNameLength();
											_registerBuffer[4] = (word)readStart;
											_registerBuffer[5] = (word)(readStart >> 16);
											_registerBuffer[6] = dataSize + ((word)timeScale << 8);
											_registerBuffer[7] = numPointsInReadPage;
											{
												word curReg = 0;
												for (int i = 0; i < _deviceDirectory->getDeviceNameLength(); i++)
												{
													if (i % 2 == 0)
													{
														curReg = device_name[i];
													}
													else
													{
														curReg += ((word)device_name[i] << 8);
													}
													if (i % 2 == 1 || i == _deviceDirectory->getDeviceNameLength() - 1)
													{
														_registerBuffer[8 + i / 2] = curReg;
													}
												}
											}
											completeModbusWriteRegisters(device_transmit->slaveId, 0,
												9 + (_deviceDirectory->getDeviceNameLength() - 1) / 2, _registerBuffer);
											AWAIT(_completeModbusWriteRegisters);
											ENSURE_NONMALFUNCTION(_completeModbusWriteRegisters);
											completeModbusReadRegisters(device_transmit->slaveId, 0, 2);
											AWAIT(_completeModbusReadRegisters);
											ENSURE_NONMALFUNCTION(_completeModbusReadRegisters);
											_modbus->isReadRegsResponse(regCount, regs);
											if ((regs[1] & 0xFF) == 3)
											{
												broadcastTime();
												_system->delayMicroseconds(10000);
												goto prepare_write;
											}
											else if ((regs[1] & 0xFF == 0))
											{
												curWritePage = 0;
												numPointsInWritePage = (byte)(regs[1] >> 8);
											}
											else
											{
												reportMalfunction(__LINE__);
												return true;
											}
										}
									}
								}

								curReadPage++;
							}
							else if (regs[1] == 1)
							{
								broadcastTime();
								_system->delayMicroseconds(10000);
							}
							else if (regs[1] == 2)
							{
								// We were mistaken; move on to other senders
							}
							else
							{
								// wtf
								reportMalfunction(__LINE__);
								return true;
							}
						}
					}
				}
			}
		}
		RETURN_ASYNC;
		END_ASYNC;
	}

	DEFINE_CLASS_TASK(THIS_T, loop, void, VARS(unsigned long, bool));
	loop_Task _loop;
	virtual ASYNC_CLASS_FUNC(THIS_T, loop)
	{
		tick(_system->millis());

		ASYNC_VAR_INIT(0, lastActivityTime, 0);
		ASYNC_VAR(1, something);
		START_ASYNC;
		for(;;)
		{
			if (_timeUpdatePending)
			{
				_timeUpdatePending = false;
				broadcastTime();
			}
			if (lastActivityTime == 0 || (getCurTime() - lastActivityTime > 2000))
			{
				checkForNewSlaves();
				AWAIT(_checkForNewSlaves);
				something = (_checkForNewSlaves.result() == found) || (_checkForNewSlaves.result() == badSlave);
				while (something)
				{
					processNewSlave(_checkForNewSlaves.result() == badSlave);
					AWAIT(_processNewSlave);
					checkForNewSlaves();
					AWAIT(_checkForNewSlaves);
					something = (_checkForNewSlaves.result() == found) || (_checkForNewSlaves.result() == badSlave);
				}
				lastActivityTime = getCurTime();
			}
			YIELD_ASYNC;
		}
		END_ASYNC;
	}

	virtual bool broadcastTime()
	{
		word data[4];
		data[0] = 1;
		data[1] = 32770; // Broadcast time function
		uint32_t *clock = (uint32_t*)(data + 2);
		*clock = getClock();
		return (completeModbusWriteRegisters(0, 0, 4, (word*)data).runSynchronously() == success);
	}

public:
	void config(S *system, M *modbus, D *deviceDirectory, byte dataBufferSize)
	{
		_system = system;
		_modbus = modbus;
		_deviceDirectory = deviceDirectory;
		_dataBufferSize = dataBufferSize;
		_dataBuffer = new byte[_dataBufferSize];
		for (int i = 0; i < 8; i++)
		{
			lastUpdateTimes[i] = 0;
		}
	}

	bool started = false;
	void loop()
	{
		if (!started)
		{
			CREATE_ASSIGN_CLASS_TASK(_loop, THIS_T, this, loop);
			started = true;
		}
		_loop();
	}

	virtual void setClock(uint32_t clock)
	{
		TimeManager::setClock(clock);
		_timeUpdatePending = true;
	}

	bool slaveFound = false;

	Master() { }

	~Master() { }
};