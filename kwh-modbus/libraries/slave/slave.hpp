#pragma once

#ifdef NO_ARDUINO
#include "../../noArduino/ArduinoMacros.h"
#include "../../noArduino/TestHelpers.h"
#else
#include "../arduinoMacros/arduinoMacros.h"
#endif

#include "../device/Device.h"
#include "../timeManager/TimeManager.h"
#include "../bitFunctions/BitFunctions.hpp"
#define ENSURE(statement) if (!(statement)) return false

enum SlaveState : word
{
	sIdle = 0,
	sReceivedRequest = 1,
	sDisplayDevInfo = 2,
	sDisplayDevData = 3,
	sPreparingToReceiveDevData = 4,
	sReceivingDevData = 5,
	sDisplayDevCommand = 6,
	sReceivingDevCommand = 7,
	sDisplayDevMessage = 8,
	sDisplaySlaveMessage = 9,
	sSetTime = 32770
};

template<class M, class S>
class Slave : public TimeManager
{
private_testable:
	const byte _majorVersion = 1;
	const byte _minorVersion = 0;
	word _deviceNameLength;
	word _deviceCount;
	byte **_deviceNames = nullptr;
	Device **_devices = nullptr;
	SlaveState _state = sIdle;
	word _stateDetail;
	bool displayedStateInvalid = true;
	word _hregCount;

	byte* _dataBuffer = nullptr;
	byte _dataBufferSize;

	S *_system;
	M *_modbus;

private_testable:
	virtual bool setOutgoingState()
	{
		ENSURE(_modbus->validRange(0, _hregCount));
		ENSURE(_modbus->Hreg(0, _state));
		byte* name;
		word deviceNum;
		word index = 0;
		switch (_state)
		{
		case sIdle:
			ENSURE(_modbus->Hreg(1, _majorVersion << 8 | _minorVersion));
			ENSURE(_modbus->Hreg(2, _deviceCount));
			ENSURE(_modbus->Hreg(3, _deviceNameLength));
			ENSURE(_modbus->Hreg(4, _hregCount));
			// Keep these 0 until I implement this
			ENSURE(_modbus->Hreg(5, 0));
			ENSURE(_modbus->Hreg(6, 0));
			ENSURE(_modbus->Hreg(7, 0));
			break;
		case sReceivedRequest:
			// Why would we do this?
			break;
		case sDisplayDevInfo:
			deviceNum = _modbus->Hreg(2);
			ENSURE(_modbus->Hreg(1, deviceNum));
			ENSURE(_modbus->Hreg(2, _devices[deviceNum]->getType()));
			name = _deviceNames[deviceNum];
			for (int i = 0; i < _deviceNameLength; i += 2)
			{
				word reg = name[i];
				if (i < _deviceNameLength - 1)
				{
					reg = reg | (name[i + 1] << 8);
				}
				ENSURE(_modbus->Hreg(3 + index, reg));
				index++;
			}
			// Commands & Messages not yet implemented
			ENSURE(_modbus->Hreg(3 + index, 0)); // Commands waiting
			ENSURE(_modbus->Hreg(4 + index, 0)); // Messages waiting
			break;
		case sDisplayDevData:
			if (wasNeverSet())
			{
				// need the time
				ENSURE(_modbus->Hreg(1, 1));
			}
			else
			{
				deviceNum = _modbus->Hreg(2);
				uint32_t startTime = _modbus->Hreg(3) + (_modbus->Hreg(4) << 16);
				word numDataPointsRequested = _modbus->Hreg(5);
				byte curPage = (byte)(_modbus->Hreg(6) & 0x00FF);
				byte maxPoints = (byte)((_modbus->Hreg(6) >> 8) & 0x00FF);
				Device *device = _devices[deviceNum];
				byte dataPointsCount;
				byte pagesRemaining;
				byte dataPointSize;
				if (device->readData(startTime, numDataPointsRequested, curPage,
					_dataBuffer, _dataBufferSize, maxPoints, dataPointsCount, pagesRemaining, dataPointSize))
				{
					// success
					ENSURE(_modbus->Hreg(1, 0));
					ENSURE(_modbus->Hreg(2, (word)(startTime & 0xFFFF)));
					ENSURE(_modbus->Hreg(3, (word)((startTime >> 16) & 0xFFFF)));
					ENSURE(_modbus->Hreg(4, (word)(dataPointsCount + ((word)(dataPointSize << 8)))));
					ENSURE(_modbus->Hreg(5, (word)(curPage + ((word)(pagesRemaining << 8)))));
					word totalBits = dataPointsCount * dataPointSize;
					byte numRegs = BitFunctions::bitsToStructs<word, word>(totalBits);
					word curReg;
					for (int i = 0; i < numRegs; i++)
					{
						curReg = 0;
						if (i < numRegs - 1)
						{
							BitFunctions::copyBits((byte*)_dataBuffer, &curReg, (word)(i * 16), (word)0, (word)16);
						}
						else
						{
							BitFunctions::copyBits((byte*)_dataBuffer, &curReg, (word)(i * 16), (word)0, (word)((totalBits - 1) % 16 + 1));
						}
						ENSURE(_modbus->Hreg(6 + i, curReg));
					}
				}
				else
				{
					// device does not send data
					ENSURE(_modbus->Hreg(1, 2));
				}
				//ENSURE(_modbus->Hreg(1, deviceNum));
				//ENSURE(_modbus->Hreg(2, _devices[deviceNum]->getType()));
			}
			break;
		case sPreparingToReceiveDevData:
			ENSURE(_modbus->Hreg(1, _stateDetail));
			break;
		}
		displayedStateInvalid = false;
		return true;
	}

	virtual bool processIncomingState(bool &requestProcessed)
	{
		requestProcessed = false;
		ENSURE(_modbus->validRange(0, _hregCount));
		if (_modbus->Hreg(0) == sReceivedRequest)
		{
			requestProcessed = true;
			switch (_modbus->Hreg(1))
			{
			case 0:
				_state = sIdle;
				displayedStateInvalid = true;
				break;
			case 1:
				_state = sIdle;
				_modbus->setSlaveId((byte)_modbus->Hreg(2));
				displayedStateInvalid = true;
				break;
			case 2:
				_state = sDisplayDevInfo;
				displayedStateInvalid = true;
				break;
			case 3:
				_state = sDisplayDevData;
				displayedStateInvalid = true;
				break;
			case 4:
			{
				_state = sPreparingToReceiveDevData;
				if (wasNeverSet())
				{
					_stateDetail = (word)RecieveDataStatus::timeRequested;
				}
				RecieveDataStatus status;
				Device *device = _devices[_modbus->Hreg(2)];
				word nameLength = _modbus->Hreg(3);
				uint32_t startTime = _modbus->Hreg(4) + (_modbus->Hreg(5) << 16);
				byte dataPointSize = (byte)(_modbus->Hreg(6) & 0xFF);
				TimeScale dataTimeScale = (TimeScale)((_modbus->Hreg(6) >> 8) & 0xFF);
				word dataPointsCount = _modbus->Hreg(7);
				if (nameLength > _dataBufferSize)
				{
					_stateDetail = (word)RecieveDataStatus::nameTooLong;
				}
				else
				{
					word curReg;
					for (int i = 0; i < nameLength; i++)
					{
						if (i % 2 == 0)
							curReg = _modbus->Hreg(8 + i / 2);
						else
							curReg >>= 8;
						_dataBuffer[i] = (byte)(curReg & 0xFF);
					}
					byte dataPointsPerPage = 0;
					status = device->prepareReceiveData(nameLength, _dataBuffer, startTime, dataPointSize, dataTimeScale, dataPointsCount, dataPointsPerPage);
					_stateDetail = (word)status + ((word)dataPointsPerPage << 8);
				}
				displayedStateInvalid = true;
			}
				break;
			case 5:
			{
				_state = sIdle;
				RecieveDataStatus status;
				Device *device = _devices[_modbus->Hreg(2)];
				byte dataPointsInPage = (byte)_modbus->Hreg(3);
				byte dataPointSize = (byte)(_modbus->Hreg(3) >> 8);
				TimeScale timeScale = (TimeScale)((byte)_modbus->Hreg(4));
				byte pageNumber = (byte)(_modbus->Hreg(4) >> 8);

				word curReg = 0;
				word dataLengthBytes = BitFunctions::bitsToBytes(dataPointsInPage * dataPointSize);
				if (dataLengthBytes > _dataBufferSize)
				{
					// Data is too long. Save myself the segfault
					return true;
				}
				else
				{
					for (int i = 0; i < dataLengthBytes; i++)
					{
						if (i % 2 == 0)
							curReg = _modbus->Hreg(5 + i / 2);
						else
							curReg >>= 8;
						_dataBuffer[i] = (byte)(curReg & 0xFF);
					}
				}
				
				status = device->receiveDeviceData(dataPointsInPage, dataPointSize, timeScale, pageNumber, _dataBuffer);
			}
				displayedStateInvalid = true;
				break;
			case 32770:
				_state = sIdle;
				setClock((uint32_t)(_modbus->Hreg(3) << 16) + (uint16_t)_modbus->Hreg(2));
				displayedStateInvalid = true;
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

	void init(word deviceCount, word deviceNameLength, word hregCount, byte dataBufferSize, Device **devices, byte **deviceNames)
	{
		clearDevices();
		_deviceCount = deviceCount;
		_deviceNameLength = deviceNameLength;
		_hregCount = hregCount;
		_dataBufferSize = dataBufferSize;
		_dataBuffer = new byte[_dataBufferSize];
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

	void setClock(uint32_t clock)
	{
		TimeManager::setClock(clock);
		for (int i = 0; i < _deviceCount; i++)
		{
			_devices[i]->setClock(clock);
		}
	}

	// Basic initial version
	void loop()
	{
		tick(_system->millis());

		bool processed;
		bool broadcast;
		_modbus->task(processed, broadcast);
		if (processed)
			processIncomingState(processed);
		if (displayedStateInvalid)
			setOutgoingState();
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

	word getHregCount()
	{
		return _hregCount;
	}
};