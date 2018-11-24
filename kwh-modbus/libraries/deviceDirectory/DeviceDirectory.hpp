#pragma once

#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#endif

struct DeviceDirectoryRow
{
	byte slaveId;
	word deviceNumber;
	word deviceType;
	word deviceRegs;

	DeviceDirectoryRow()
	{
		slaveId = 0;
		deviceType = 0;
		deviceNumber = 0;
		deviceRegs = 0;
	}

	DeviceDirectoryRow(byte _slaveId, word _deviceNumber, word _deviceType, word _deviceRegs)
	{
		slaveId = _slaveId;
		deviceType = _deviceType;
		deviceNumber = _deviceNumber;
		deviceRegs = _deviceRegs;
	}
};

static bool operator==(const DeviceDirectoryRow& lhs, const DeviceDirectoryRow& rhs)
{
	return (lhs.slaveId == rhs.slaveId) &&
		(lhs.deviceNumber == rhs.deviceNumber) &&
		(lhs.deviceType == rhs.deviceType) &&
		(lhs.deviceRegs == rhs.deviceRegs);
}

static bool operator!=(const DeviceDirectoryRow& lhs, const DeviceDirectoryRow& rhs)
{
	return (lhs.slaveId != rhs.slaveId) ||
		(lhs.deviceNumber != rhs.deviceNumber) ||
		(lhs.deviceType != rhs.deviceType) ||
		(lhs.deviceRegs != rhs.deviceRegs);
}

template<typename T>
class DeviceDirectory
{
private_testable:
	word _maxDevices;
	word _deviceNameLength;
	byte* _deviceNames = nullptr;
	DeviceDirectoryRow* _devices = nullptr;
	T* _persistentStore = nullptr;

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
		_devices = new DeviceDirectoryRow[_maxDevices];
		_deviceNames = new byte[_deviceNameLength * _maxDevices];
		for (int i = 0; i < _maxDevices; i++)
		{
			_devices[i] = DeviceDirectoryRow();
		}
	}

	virtual void init(int maxMemory, word deviceNameLength, word &maxDevicesOut, T *persistentStore = nullptr)
	{
		maxDevicesOut = maxMemory / (sizeof(DeviceDirectoryRow) + deviceNameLength * sizeof(byte));
		init(deviceNameLength, maxDevicesOut, persistentStore);
	}

	//Device Directory row: [Name (8), DeviceType (1), SlaveID (1)]
	//If empty, DeviceType = 0, and SlaveID = 0 unless there's
	//device entries after that row, then DeviceType = 1

	virtual DeviceDirectoryRow* findDeviceForName(byte* devName, int &rowOut = 0)
	{
		rowOut = -1;
		for (int i = 0; i < _maxDevices; i++)
		{
			if (_devices[i].slaveId == 0)
			{
				if (_devices[i].deviceType == 0)
					return nullptr;
			}
			else
			{
				if (compareName(i, devName))
				{
					rowOut = i;
					return _devices + i;
				}
			}
		}
		return nullptr;
	}

	//void SetDirectoryValue(int index, byte value)
	//{
	//	DeviceDirectory[index] = value;
	//	if (USE_EEPROM)
	//		EEPROM[index + EEPROM_OFFSET] = value;
	//}

	virtual void insertIntoRow(int row, byte* devName, DeviceDirectoryRow device)
	{
		byte* name = getDeviceName(row);
		for (int i = 0; i < _deviceNameLength; i++)
			name[i] = devName[i];
		_devices[row] = device;
	}

	virtual bool updateItemInDeviceDirectory(byte* devName, DeviceDirectoryRow device)
	{
		int row;
		auto current = findDeviceForName(devName, row);
		if (current != nullptr && *current != device)
		{
			insertIntoRow(row, devName, device);
			return true;
		}
		return false;
	}

	virtual void clearDeviceDirectoryRow(int row)
	{
		_devices[row].slaveId = 0;
		bool devicesAbove = false;

		if (row < _maxDevices - 1)
		{
			devicesAbove = (_devices[row + 1].slaveId > 0 || _devices[row + 1].deviceType > 0);
		}

		if (devicesAbove)
			_devices[row].deviceType = 1;
		else
			_devices[row].deviceType = 0;

		while (!devicesAbove &&
			--row > 0 &&
			_devices[row].slaveId == 0 &&
			_devices[row].deviceType == 1)
		{
			_devices[row].deviceType = 0;
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
		while (_devices[row].slaveId != 0)
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
			if (_devices[i].slaveId == 0)
			{
				if (_devices[i].deviceType == 0)
					return slaveId;
			}
			if (_devices[i].slaveId == slaveId)
			{
				slaveId++;
				i = 0;
				if (slaveId > 246)
					return 0;
			}
		}
		return slaveId;
	}

	virtual int addDevice(byte* devName, DeviceDirectoryRow device)
	{
		int row = findFreeRow();
		if (row == -1)
			return -1;
		insertIntoRow(row, devName, device);
		return row;
	}

	virtual int filterDevicesForSlave(byte** devNames, int devNamesCount, byte slaveId)
	{
		int numDeleted = 0;
		int ind;
		for (int i = 0; i < _maxDevices; i++)
		{
			if (_devices[i].slaveId == 0 && _devices[i].deviceType == 0)
				return numDeleted;
			else if (_devices[i].slaveId == slaveId)
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
	
	virtual int addOrReplaceDevice(byte* devName, DeviceDirectoryRow device)
	{
		int row;
		if (findDeviceForName(devName, row) != nullptr)
		{
			insertIntoRow(row, devName, device);
			return row;
		}
		else
			return addDevice(devName, device);
	}

	// Untested
	DeviceDirectoryRow* findNextDevice(byte* devName, int &row)
	{
		while (_devices[row].slaveId == 0)
		{
			if (_devices[row].deviceType == 0)
			{
				row = -1;
				return nullptr;
			}
			row++;
		}
		byte* name = getDeviceName(row);
		for (int i = 0; i < _deviceNameLength; i++)
		{
			devName[i] = name[i];
		}
		row += 1;
		return (_devices + row - 1);
	}

	// Untested
	bool isEmpty()
	{
		return _devices[0].slaveId == 0 && _devices[0].deviceType == 0;
	}

	~DeviceDirectory()
	{
		if (_devices != nullptr)
			delete [] _devices;
		if (_deviceNames != nullptr)
			delete [] _deviceNames;
	}
};