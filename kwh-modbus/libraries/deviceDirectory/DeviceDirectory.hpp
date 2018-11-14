#pragma once

#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#endif

template<typename T>
class DeviceDirectory
{
private_testable:
	word _maxDevices;
	word _deviceNameLength;
	byte* _slaveIds = nullptr;
	word* _deviceTypes = nullptr;
	byte* _deviceNames = nullptr;
	word* _deviceRegs = nullptr;
	T* _persistentStore;

	virtual byte* getDeviceName(word deviceIndex)
	{
		return (_deviceNames + deviceIndex * _deviceNameLength);
	}

	virtual byte getNameChar(word deviceIndex, word charIndex)
	{
		return getDeviceName(deviceIndex)[charIndex];
	}

	virtual bool compareName(word deviceIndex, byte* name)
	{
		byte* deviceName = getDeviceName(deviceIndex);
		for (int i = 0; i < _deviceNameLength; i++)
		{
			if (name[i] != deviceName[i])
				return false;
		}
		return true;
	}

public:
	virtual void init(word deviceNameLength, word maxDevices, T *persistentStore = nullptr)
	{
		_deviceNameLength = deviceNameLength;
		_maxDevices = maxDevices;
		_persistentStore = persistentStore;
		_slaveIds = new byte[_maxDevices];
		_deviceTypes = new word[_maxDevices];
		_deviceNames = new byte[_deviceNameLength * _maxDevices];
		_deviceRegs = new word[_maxDevices];
		for (int i = 0; i < _maxDevices; i++)
		{
			_slaveIds[i] = 0;
			_deviceTypes[i] = 0;
			_deviceRegs[i] = 0;
		}
	}

	virtual void init(int maxMemory, word deviceNameLength, word &maxDevicesOut, T *persistentStore = nullptr)
	{
		maxDevicesOut = maxMemory / (sizeof(byte) + sizeof(word) + sizeof(word) + deviceNameLength * sizeof(byte));
		init(deviceNameLength, maxDevicesOut, persistentStore);
	}

	//Device Directory row: [Name (8), DeviceType (1), SlaveID (1)]
	//If empty, DeviceType = 0, and SlaveID = 0 unless there's
	//device entries after that row, then DeviceType = 1

	virtual bool findDeviceForName(byte* devName, word &devTypeOut, byte &slaveIdOut, word &devRegsOut, int &rowOut = 0)
	{
		devTypeOut = 0;
		slaveIdOut = 0;
		devRegsOut = 0;
		rowOut = -1;
		for (int i = 0; i < _maxDevices; i++)
		{
			if (_slaveIds[i] == 0)
			{
				if (_deviceTypes[i] == 0)
					return false;
			}
			else
			{
				if (compareName(i, devName))
				{
					devTypeOut = _deviceTypes[i];
					slaveIdOut = _slaveIds[i];
					devRegsOut = _deviceRegs[i];
					rowOut = i;
					return true;
				}
			}
		}
		return false;
	}

	//void SetDirectoryValue(int index, byte value)
	//{
	//	DeviceDirectory[index] = value;
	//	if (USE_EEPROM)
	//		EEPROM[index + EEPROM_OFFSET] = value;
	//}

	virtual void insertIntoRow(int row, byte* devName, byte devType, byte slaveId)
	{
		byte* name = getDeviceName(row);
		for (int i = 0; i < _deviceNameLength; i++)
			name[i] = devName[i];
		_deviceTypes[row] = devType;
		_slaveIds[row] = slaveId;
	}

	virtual bool updateItemInDeviceDirectory(byte* devName, word devType, byte slaveId)
	{
		word curType;
		byte curID;
		word curRegs;
		int row;
		findDeviceForName(devName, curType, curID, curRegs, row);
		if (row >= 0 && (curType != devType || curID != slaveId))
		{
			insertIntoRow(row, devName, devType, slaveId);
			return true;
		}
		return false;
	}

	virtual void clearDeviceDirectoryRow(int row)
	{
		_slaveIds[row] = 0;
		bool devicesAbove = false;

		if (row < _maxDevices - 1)
		{
			devicesAbove = (_slaveIds[row + 1] > 0 || _deviceTypes[row + 1] > 0);
		}

		if (devicesAbove)
			_deviceTypes[row] = 1;
		else
			_deviceTypes[row] = 0;

		while (!devicesAbove &&
			--row > 0 &&
			_slaveIds[row] == 0 &&
			_deviceTypes[row] == 1)
		{
			_deviceTypes[row] = 0;
		}
	}

	//void InitializeDeviceDirectory()
	//{
	//	DeviceDirectory = new byte[MAX_DEVICES * 10];
	//	if (USE_EEPROM)
	//	{
	//		if (EEPROM[0] != 1)
	//		{
	//			EEPROM[0] = 1;
	//			for (int i = 0; i < MAX_DEVICES * 10; i++)
	//				SetDirectoryValue(i, 0);
	//		}
	//		else
	//		{
	//			for (int i = 0; i < MAX_DEVICES * 10; i++)
	//				DeviceDirectory[i] = EEPROM[i + EEPROM_OFFSET];
	//		}
	//	}
	//	else
	//	{
	//		if (EEPROM[0] != 0)
	//			EEPROM[0] = 0;
	//		for (int i = 0; i < MAX_DEVICES * 10; i++)
	//			SetDirectoryValue(i, 0);
	//	}

	//	//Test completed
	//	//TestSlaveIDExhaustion();
	//}

	virtual int findFreeRow()
	{
		int row = 0;
		while (_slaveIds[row] != 0)
			row++;
		if (row >= _maxDevices)
			return -1;
		return row;
	}

	virtual byte findFreeSlaveID()
	{
		byte slaveId = 2;
		for (int i = 0; i < _maxDevices; i++)
		{
			if (_slaveIds[i] == 0)
			{
				if (_deviceTypes[i] == 0)
					return slaveId;
			}
			if (_slaveIds[i] == slaveId)
			{
				slaveId++;
				i = 0;
				if (slaveId > 246)
					return 0;
			}
		}
		return slaveId;
	}

	virtual int addDevice(byte* devName, word devType, byte slaveId)
	{
		int row = findFreeRow();
		if (row == -1)
			return -1;
		insertIntoRow(row, devName, devType, slaveId);
		return row;
	}

	virtual int filterDevicesForSlave(byte** devNames, int devNamesCount, byte slaveId)
	{
		int numDeleted = 0;
		int ind;
		for (int i = 0; i < _maxDevices; i++)
		{
			if (_slaveIds[i] == 0 && _deviceTypes[i] == 0)
				return numDeleted;
			else if (_slaveIds[i] == slaveId)
			{
				bool found = false;
				for (int k = 0; k < devNamesCount; k++)
				{
					byte* curName = devNames[k];
					bool match = compareName(i, curName);
					if (match)
						found = true;
				}
				if (!found)
				{
					clearDeviceDirectoryRow(i);
					numDeleted++;
				}
			}
		}
		return numDeleted;
	}
	
	virtual int addOrReplaceDevice(byte* devName, word devType, byte slaveId)
	{
		int row;
		word dummyType;
		byte dummySlave;
		if (findDeviceForName(devName, dummyType, dummySlave, dummyType, row))
		{
			insertIntoRow(row, devName, devType, slaveId);
			return row;
		}
		else
			return addDevice(devName, devType, slaveId);
	}

	// Untested
	int findNextDevice(byte* devName, byte &slaveIdOut, word &devTypeOut, int startRow = 0)
	{
		int row = startRow;
		while (_slaveIds[row] == 0)
		{
			if (_deviceTypes[row] == 0)
				return -1;
			row++;
		}
		byte* name = getDeviceName(row);
		for (int i = 0; i < _deviceNameLength; i++)
		{
			devName[i] = name[i];
		}
		devTypeOut = _deviceTypes[row];
		slaveIdOut = _slaveIds[row];
		return row + 1;
	}

	// Untested
	bool isEmpty()
	{
		return _slaveIds[0] == 0 && _deviceTypes[0] == 0;
	}

	~DeviceDirectory()
	{
		if (_slaveIds != nullptr)
			delete [] _slaveIds;
		if (_deviceTypes != nullptr)
			delete [] _deviceTypes;
		if (_deviceNames != nullptr)
			delete [] _deviceNames;
	}
};