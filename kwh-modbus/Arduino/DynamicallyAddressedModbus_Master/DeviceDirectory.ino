#if VS_BUILD
#include "C:\GitHub\KWH-Smart-Metering-System\Visual Studio\Test Bench\Test Bench\VSPDE.h"
//#include "C:\GitHub\KWH-Smart-Metering-System\Visual Studio\Test Bench\Test Bench\DeviceDirectory.h"
#endif
#if UNIT_TEST
class DeviceDirectory_
{
public:
#endif
#if VS_BUILD
	char EEPROM[512];
#endif

	const int MAX_DEVICES = 256;
	uint8_t* DeviceDirectory;
	const int EEPROM_OFFSET = 1;
	const bool USE_EEPROM = false;

	//Device Directory row: [Name (8), DeviceType (1), SlaveID (1)]
	//If empty, DeviceType = 0, and SlaveID = 0 unless there's
	//device entries after that row, then SlaveID = 1

	bool FindDeviceDetails(uint8_t* devName8, uint8_t *devTypeOut, uint8_t *slaveIDOut, int *rowOut)
	{
		*devTypeOut = 0;
		*slaveIDOut = 0;
		*rowOut = -1;
		int ind;
		bool found;
		for (int i = 0; i < MAX_DEVICES; i++)
		{
			ind = 10 * i;
			if (DeviceDirectory[ind + 8] == 0)
			{
				if (DeviceDirectory[ind + 9] == 0)
					return false;
			}
			else
			{
				bool found = true;
				for (int j = 0; j < 8; j++)
				{
					if (devName8[j] != DeviceDirectory[ind + j])
						found = false;
				}
				if (found)
				{
					*devTypeOut = DeviceDirectory[ind + 8];
					*slaveIDOut = DeviceDirectory[ind + 9];
					*rowOut = i;
					return true;
				}
			}
		}
		return false;
	}

	bool FindDeviceDetails(uint8_t* devName8, uint8_t *devTypeOut, uint8_t *slaveIDOut)
	{
		int dummy;
		return FindDeviceDetails(devName8, devTypeOut, slaveIDOut, &dummy);
	}

	void SetDirectoryValue(int index, uint8_t value)
	{
		DeviceDirectory[index] = value;
		if (USE_EEPROM)
			EEPROM[index + EEPROM_OFFSET] = value;
	}

	void InsertIntoDeviceDirectoryRow(int row, uint8_t* devName8, uint8_t devType, int slaveID)
	{
		int ind = 10 * row;
		for (int i = 0; i < 8; i++)
			SetDirectoryValue(ind + i, devName8[i]);
		SetDirectoryValue(ind + 8, devType);
		SetDirectoryValue(ind + 9, slaveID);
	}

	bool UpdateItemInDeviceDirectory(uint8_t* devName8, uint8_t devType, int slaveID)
	{
		uint8_t curType;
		uint8_t curID;
		int row;
		FindDeviceDetails(devName8, &curType, &curID, &row);
		if (row >= 0 && (curType != devType || curID != slaveID))
		{
			InsertIntoDeviceDirectoryRow(row, devName8, devType, slaveID);
			return true;
		}
		return false;
	}

	void ClearDeviceDirectoryRow(int row)
	{
		int ind = 10 * row;
		for (int i = 0; i < 9; i++)
			SetDirectoryValue(ind + i, 0);
		if (row == MAX_DEVICES - 1 || DeviceDirectory[ind + 18] == 0)
			SetDirectoryValue(ind + 9, 0);
		else
			SetDirectoryValue(ind + 9, 1);
		if (row > 0 && DeviceDirectory[ind - 2] == 0)
			SetDirectoryValue(ind - 1, 0);
	}

	void InitializeDeviceDirectory()
	{
		DeviceDirectory = new uint8_t[MAX_DEVICES * 10];
		if (USE_EEPROM)
		{
			if (EEPROM[0] != 1)
			{
				EEPROM[0] = 1;
				for (int i = 0; i < MAX_DEVICES * 10; i++)
					SetDirectoryValue(i, 0);
			}
			else
			{
				for (int i = 0; i < MAX_DEVICES * 10; i++)
					DeviceDirectory[i] = EEPROM[i + EEPROM_OFFSET];
			}
		}
		else
		{
			if (EEPROM[0] != 0)
				EEPROM[0] = 0;
			for (int i = 0; i < MAX_DEVICES * 10; i++)
				SetDirectoryValue(i, 0);
		}

		//Test completed
		//TestSlaveIDExhaustion();
	}

	//Warning: allocates memory!
	void TestSlaveIDExhaustion()
	{
		uint8_t* dummyName = new uint8_t[8];
		for (int i = 1; i <= 245; i++)
		{
			InsertIntoDeviceDirectoryRow(i - 1, dummyName, 1, i);
		}
	}

	int FindFreeRow()
	{
		int row = 0;
		while (DeviceDirectory[row * 10 + 8] != 0)
			row++;
		return row;
	}

	int FindFreeSlaveID()
	{
		int slaveID = 1;
		int ind;
		for (int i = 0; i < MAX_DEVICES; i++)
		{
			ind = 10 * i;
			if (DeviceDirectory[ind + 8] == 0)
			{
				if (DeviceDirectory[ind + 9] == 0)
					return slaveID;
			}
			if (DeviceDirectory[ind + 9] == slaveID)
			{
				slaveID++;
				i = 0;
				if (slaveID > 246)
					return 0;
			}
		}
		return slaveID;
	}

	int AddToDeviceDirectory(uint8_t* devName8, uint8_t devType, int slaveID)
	{
		int row = FindFreeRow();
		InsertIntoDeviceDirectoryRow(row, devName8, devType, slaveID);
		return row;
	}

	int DeleteDevicesForSlaveNotInList(uint8_t** devName8s, int devName8sCount, uint8_t slaveID)
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
			else if (DeviceDirectory[ind + 9] == slaveID)
			{
				bool found = false;
				for (int k = 0; k < devName8sCount; k++)
				{
					uint8_t* curName = devName8s[k];
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

#if UNIT_TEST
};
#endif