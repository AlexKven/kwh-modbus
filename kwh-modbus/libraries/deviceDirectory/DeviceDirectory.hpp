#pragma once

#include "../../noArduino/TestHelpers.h"
#ifdef NO_ARDUINO
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
	T* _persistentStore;

	byte* getDeviceName(word deviceIndex)
	{
		return (_deviceNames + deviceIndex * _deviceNameLength);
	}

	byte getNameChar(word deviceIndex, word charIndex)
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
		for (int i = 0; i < _maxDevices; i++)
		{
			_slaveIds[i] = 0;
			_deviceTypes[i] = 0;
		}
	}

	virtual void init(int maxMemory, word deviceNameLength, word &maxDevicesOut, T *persistentStore = nullptr)
	{
		maxDevicesOut = maxMemory / (sizeof(byte) + sizeof(word) + deviceNameLength * sizeof(byte));
		init(deviceNameLength, maxDevicesOut, persistentStore);
	}

	//Device Directory row: [Name (8), DeviceType (1), SlaveID (1)]
	//If empty, DeviceType = 0, and SlaveID = 0 unless there's
	//device entries after that row, then DeviceType = 1

	bool findDeviceForName(byte* devName, word &devTypeOut, byte &slaveIdOut, int &rowOut = 0)
	{
		devTypeOut = 0;
		slaveIdOut = 0;
		rowOut = -1;
		bool found;
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

	void insertIntoDeviceDirectoryRow(int row, byte* devName, byte devType, byte slaveId)
	{
		byte* name = getDeviceName(row);
		for (int i = 0; i < _deviceNameLength; i++)
			name[i] = devName[i];
		_deviceTypes[row] = devType;
		_slaveIds[row] = slaveId;
	}

	bool updateItemInDeviceDirectory(byte* devName, word devType, byte slaveId)
	{
		word curType;
		byte curID;
		int row;
		findDeviceForName(devName, curType, curID, row);
		if (row >= 0 && (curType != devType || curID != slaveId))
		{
			insertIntoDeviceDirectoryRow(row, devName8, devType, slaveId);
			return true;
		}
		return false;
	}

	void ClearDeviceDirectoryRow(int row)
	{
		_slaveIds[row] = 0;

		if (row < _maxDevices - 1)
		{
			if (_slaveIds[row + 1] > 0 || _deviceTypes[row + 1] > 0)
				_deviceTypes[row] = 1;
			else
				_deviceTypes[row] = 0;
		}

		// This code is embarrassingly hard to read
		//int ind = 10 * row;
		//for (int i = 0; i < 9; i++)
		//	SetDirectoryValue(ind + i, 0);

		//if (row == MAX_DEVICES - 1 || DeviceDirectory[ind + 18] == 0)
		//	SetDirectoryValue(ind + 9, 0);
		//else
		//	SetDirectoryValue(ind + 9, 1);
		//if (row > 0 && DeviceDirectory[ind - 2] == 0)
		//	SetDirectoryValue(ind - 1, 0);
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

	////Warning: allocates memory!
	//void TestSlaveIDExhaustion()
	//{
	//	byte* dummyName = new byte[8];
	//	for (int i = 1; i <= 245; i++)
	//	{
	//		InsertIntoDeviceDirectoryRow(i - 1, dummyName, 1, i);
	//	}
	//}

	int findFreeRow()
	{
		int row = 0;
		while (_slaveIds[row] != 0)
			row++;
		return row;
	}

	int findFreeSlaveID()
	{
		byte slaveId = 1;
		for (int i = 0; i < _maxDevices; i++)
		{
			if (_slaveIds[i] == 0)
			{
				if (_deviceTypes[i] == 0)
					return slaveId;
			}
			if (slaveIds[i] == slaveId)
			{
				slaveId++;
				i = 0;
				if (slaveId > 246)
					return 0;
			}
		}
		return slaveId;
	}
	/*
	int AddToDeviceDirectory(byte* devName8, byte devType, byte slaveId)
	{
		int row = FindFreeRow();
		InsertIntoDeviceDirectoryRow(row, devName8, devType, slaveId);
		return row;
	}

	int DeleteDevicesForSlaveNotInList(byte** devName8s, int devName8sCount, byte slaveId)
	{
		int numDeleted = 0;
		int ind;
		for (int i = 0; i < MAX_DEVICES; i++)
		{
			ind = 10 * i;
			if (DeviceDirectory[ind + 8] == 0)
			{
				if (DeviceDirectory[ind + 9] == 0)
					return numDeleted;
			}
			else if (DeviceDirectory[ind + 9] == slaveId)
			{
				bool found = false;
				for (int k = 0; k < devName8sCount; k++)
				{
					byte* curName = devName8s[k];
					bool match = true;
					for (int j = 0; j < 8; j++)
					{
						if (curName[j] != DeviceDirectory[ind + j])
							match = false;
					}
					if (match)
						found = true;
				}
				if (!found)
				{
					ClearDeviceDirectoryRow(i);
					numDeleted++;
				}
			}
		}
		numDeleted++;
	}
	*/

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