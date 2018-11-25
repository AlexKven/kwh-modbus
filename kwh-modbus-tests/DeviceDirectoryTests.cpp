#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/deviceDirectory/DeviceDirectory.hpp"
#include "test_helpers.h"
#define getMock() Mock<DeviceDirectory<byte*>>(*deviceDirectory)

using namespace fakeit;

class DeviceDirectoryTests : public ::testing::Test
{
protected:
	word * registerArray;
	DeviceDirectory<byte*> *deviceDirectory = new DeviceDirectory<byte*>();

	inline void setupCompareName_ReturnTrueOn(Mock<DeviceDirectory<byte*>> &mock, word index)
	{
		for (int i = 0; i < index; i++)
		{
			When(Method(mock, compareName).Using(i, Any<byte*>())).AlwaysReturn(false);
		}
		When(Method(mock, compareName).Using(index, Any<byte*>())).AlwaysReturn(true);
	}
public:
	void SetUp()
	{
	}

	void TearDown()
	{
		delete deviceDirectory;
	}

	int asyncTest(int __resume_line__)
	{
		
	}

	static bool conditionFct(byte* name, DeviceDirectoryRow &device)
	{
		int row = (name[3] - '0');
		if (row >= 3 && row <= 5)
			return false;
		return (device.slaveId + device.deviceType) % 2 == 0;
	};
};

TEST_F(DeviceDirectoryTests, getDeviceName)
{
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceNames = new byte[100];
	for (int i = 0; i < 100; i++)
	{
		deviceDirectory->_deviceNames[i] = i;
	}

	assertArrayEq<byte, byte, byte, byte, byte>
		(deviceDirectory->getDeviceName(0),
			0, 1, 2, 3, 4);
	assertArrayEq<byte, byte, byte, byte, byte>
		(deviceDirectory->getDeviceName(3),
			15, 16, 17, 18, 19);
	assertArrayEq<byte, byte, byte, byte, byte>
		(deviceDirectory->getDeviceName(12),
			60, 61, 62, 63, 64);
}

TEST_F(DeviceDirectoryTests, getNameChar)
{
	deviceDirectory->_deviceNameLength = 7;
	deviceDirectory->_deviceNames = new byte[100];
	for (int i = 0; i < 100; i++)
	{
		deviceDirectory->_deviceNames[i] = i;
	}


	ASSERT_EQ(deviceDirectory->getNameChar(1, 2), 9);
	ASSERT_EQ(deviceDirectory->getNameChar(1, 6), 13);
	ASSERT_EQ(deviceDirectory->getNameChar(4, 0), 28);
	ASSERT_EQ(deviceDirectory->getNameChar(7, 5), 54);
	ASSERT_EQ(deviceDirectory->getNameChar(13, 3), 94);
}

TEST_F(DeviceDirectoryTests, compareName)
{
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceNames = (byte*)"dev00dev01dev02dev03dev04dev05dev06oth00oth01oth02oth03";

	ASSERT_TRUE(deviceDirectory->compareName(0, (byte*)"dev00"));
	ASSERT_TRUE(deviceDirectory->compareName(0, (byte*)"dev00dev01"));
	// Doesn't check past device name length
	ASSERT_TRUE(deviceDirectory->compareName(0, (byte*)"dev00hijklmnop"));
	ASSERT_TRUE(deviceDirectory->compareName(5, (byte*)"dev05"));
	ASSERT_TRUE(deviceDirectory->compareName(8, (byte*)"oth01"));


	ASSERT_FALSE(deviceDirectory->compareName(0, (byte*)"dev01"));
	ASSERT_FALSE(deviceDirectory->compareName(0, (byte*)"dve00dev01"));
	// Doesn't check past device name length
	ASSERT_FALSE(deviceDirectory->compareName(0, (byte*)"dev0ghijklmnop"));
	ASSERT_FALSE(deviceDirectory->compareName(5, (byte*)"deb05"));
	ASSERT_FALSE(deviceDirectory->compareName(8, (byte*)"oth0"));

	// This test only. Since this test uses a C-style string and
	// directly assigns it to _deviceNames, that means it's deleted
	// after this scope ends, so I need to set it to nullptr,
	// otherwise the destructor will try to delete it and will
	// cause a segfault.
	deviceDirectory->_deviceNames = nullptr;
}

TEST_F(DeviceDirectoryTests, init_default)
{
	deviceDirectory->init(11, 5);
	for (int i = 0; i < 55; i++)
	{
		deviceDirectory->_deviceNames[i] = i;
	}
	for (int i = 0; i < 5; i++)
	{
		deviceDirectory->_devices[i].deviceType = i;
		deviceDirectory->_devices[i].slaveId = i;
	}

	ASSERT_EQ(deviceDirectory->_maxDevices, 5);
	ASSERT_EQ(deviceDirectory->_deviceNameLength, 11);

	ASSERT_EQ(deviceDirectory->_deviceNames[0], 0);
	ASSERT_EQ(deviceDirectory->_deviceNames[54], 54);
	ASSERT_EQ(deviceDirectory->_devices[0].deviceType, 0);
	ASSERT_EQ(deviceDirectory->_devices[4].deviceType, 4);
	ASSERT_EQ(deviceDirectory->_devices[0].slaveId, 0);
	ASSERT_EQ(deviceDirectory->_devices[4].slaveId, 4);
	ASSERT_EQ(deviceDirectory->_persistentStore, nullptr);
}

TEST_F(DeviceDirectoryTests, init_persistentStore)
{
	byte *persistentStore = new byte();

	deviceDirectory->init(11, 5, &persistentStore);
	for (int i = 0; i < 55; i++)
	{
		deviceDirectory->_deviceNames[i] = i;
	}

	ASSERT_EQ(deviceDirectory->_maxDevices, 5);
	ASSERT_EQ(deviceDirectory->_deviceNameLength, 11);

	ASSERT_EQ(deviceDirectory->_deviceNames[0], 0);
	ASSERT_EQ(deviceDirectory->_deviceNames[54], 54);
	ASSERT_EQ(deviceDirectory->_devices[0].deviceType, 0);
	ASSERT_EQ(deviceDirectory->_devices[4].deviceType, 0);
	ASSERT_EQ(deviceDirectory->_devices[0].slaveId, 0);
	ASSERT_EQ(deviceDirectory->_devices[4].slaveId, 0);
	ASSERT_EQ(deviceDirectory->_persistentStore, &persistentStore);
}

TEST_F(DeviceDirectoryTests, init_maxMemory)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	Fake(OverloadedMethod(mock, init, void(word, word, byte**)));

	word numDevices;
	deviceDirectory->init(500, 11, numDevices);

	ASSERT_EQ(numDevices, 26);
	Verify(OverloadedMethod(mock, init, void(word, word, byte**)).Using(11, 26, nullptr)).Once();
}

TEST_F(DeviceDirectoryTests, findDeviceForName_empty)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	setupCompareName_ReturnTrueOn(mock, 3);
	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_devices = new DeviceDirectoryRow();

	int row;
	auto result = deviceDirectory->findDeviceForName((byte*)"fiver", row);

	ASSERT_EQ(result, nullptr);
	ASSERT_EQ(row, -1);
	Verify(Method(mock, compareName).Using(Any<word>(), Any<byte*>())).Never();
}

TEST_F(DeviceDirectoryTests, findDeviceForName_emptyish)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	setupCompareName_ReturnTrueOn(mock, 3);
	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_devices = new DeviceDirectoryRow[2]
	{
		DeviceDirectoryRow(),
		DeviceDirectoryRow(0, 0, 1, 0)
	};

	int row;
	auto result = deviceDirectory->findDeviceForName((byte*)"fiver", row);

	ASSERT_EQ(result, nullptr);
	ASSERT_EQ(row, -1);
	Verify(Method(mock, compareName).Using(Any<word>(), Any<byte*>())).Never();
}

TEST_F(DeviceDirectoryTests, findDeviceForName_exhaustedList)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	setupCompareName_ReturnTrueOn(mock, 6);
	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_devices = new DeviceDirectoryRow[6]
	{
		DeviceDirectoryRow(0, 2, 1, 1),
		DeviceDirectoryRow(3, 3, 4, 1),
		DeviceDirectoryRow(6, 5, 7, 2),
		DeviceDirectoryRow(9, 7, 10, 3),
		DeviceDirectoryRow(12, 11, 13, 5),
		DeviceDirectoryRow(15, 13, 16, 8)
	};

	int row;
	auto result = deviceDirectory->findDeviceForName((byte*)"fiver", row);

	ASSERT_EQ(result, nullptr);
	ASSERT_EQ(row, -1);
	Verify(Method(mock, compareName).Using(0, Any<byte*>())).Never();
	Verify(Method(mock, compareName).Using(1, Any<byte*>())).Once();
	Verify(Method(mock, compareName).Using(5, Any<byte*>())).Once();
}

TEST_F(DeviceDirectoryTests, findDeviceForName_found)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	setupCompareName_ReturnTrueOn(mock, 3);
	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_devices = new DeviceDirectoryRow[6]
	{
		DeviceDirectoryRow(0, 2, 1, 1),
		DeviceDirectoryRow(3, 3, 4, 1),
		DeviceDirectoryRow(6, 5, 7, 2),
		DeviceDirectoryRow(9, 7, 10, 3),
		DeviceDirectoryRow(12, 11, 13, 5),
		DeviceDirectoryRow(15, 13, 16, 8)
	};

	int row;
	auto result = deviceDirectory->findDeviceForName((byte*)"fiver", row);

	ASSERT_EQ(result, deviceDirectory->_devices + 3);
	ASSERT_EQ(row, 3);
	Verify(Method(mock, compareName).Using(0, Any<byte*>())).Never();
	Verify(Method(mock, compareName).Using(1, Any<byte*>())).Once();
	Verify(Method(mock, compareName).Using(3, Any<byte*>())).Once();
	Verify(Method(mock, compareName).Using(5, Any<byte*>())).Never();
}

TEST_F(DeviceDirectoryTests, insertIntoRow)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_devices = new DeviceDirectoryRow[6];

	byte *name = new byte[5];
	When(Method(mock, getDeviceName).Using((word)3)).Return(name);

	deviceDirectory->insertIntoRow(3, (byte*)"Aleah", DeviceDirectoryRow(31, 7, 29, 10));

	ASSERT_EQ(deviceDirectory->_devices[3], DeviceDirectoryRow(31, 7, 29, 10));
	assertArrayEq<byte, byte, byte, byte, byte>(name,
		'A', 'l', 'e', 'a', 'h');
}

TEST_F(DeviceDirectoryTests, updateItemInDeviceDirectory_notFound)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();

	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_devices = new DeviceDirectoryRow[6];

	When(Method(mock, findDeviceForName)).Do([](byte* name, int &row)
	{
		row = -1;
		return nullptr;
	});
	Fake(Method(mock, insertIntoRow));

	bool success = deviceDirectory->updateItemInDeviceDirectory((byte*)"Aleah", DeviceDirectoryRow(31, 7, 29, 10));

	ASSERT_FALSE(success);
	Verify(Method(mock, insertIntoRow)).Never();
}

TEST_F(DeviceDirectoryTests, updateItemInDeviceDirectory_foundButSame)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();

	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_devices = new DeviceDirectoryRow[6];
	auto device = DeviceDirectoryRow(31, 7, 29, 703);

	When(Method(mock, findDeviceForName)).Do([&device](byte* name, int &row)
	{
		row = 5;
		return &device;
	});
	Fake(Method(mock, insertIntoRow));

	bool success = deviceDirectory->updateItemInDeviceDirectory((byte*)"Aleah", DeviceDirectoryRow(31, 7, 29, 703));

	ASSERT_FALSE(success);
	Verify(Method(mock, insertIntoRow)).Never();
}

TEST_F(DeviceDirectoryTests, updateItemInDeviceDirectory_foundAndChanged)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();

	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_devices = new DeviceDirectoryRow[6];
	auto device = DeviceDirectoryRow(37, 8, 29, 429);

	When(Method(mock, findDeviceForName)).Do([&device](byte* name, int &row)
	{
		row = 5;
		return &device;
	});
	Fake(Method(mock, insertIntoRow));

	bool success = deviceDirectory->updateItemInDeviceDirectory((byte*)"Aleah", DeviceDirectoryRow(31, 7, 29, 703));

	ASSERT_TRUE(success);
	Verify(Method(mock, insertIntoRow).Using(5, Any<byte*>(), DeviceDirectoryRow(31, 7, 29, 703))).Once();
}

TEST_F(DeviceDirectoryTests, clearDeviceDirectoryRow_topDevice)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(3, 0, 1, 0),
		DeviceDirectoryRow(4, 0, 4, 0),
		DeviceDirectoryRow(5, 0, 1, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	deviceDirectory->clearDeviceDirectoryRow(4);

	assertArrayEq(deviceDirectory->_devices,
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(3, 0, 1, 0),
		DeviceDirectoryRow(4, 0, 4, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow());
}

TEST_F(DeviceDirectoryTests, clearDeviceDirectoryRow_penultimateDevice)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(3, 0, 1, 0),
		DeviceDirectoryRow(4, 0, 4, 0),
		DeviceDirectoryRow(5, 0, 1, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	deviceDirectory->clearDeviceDirectoryRow(3);
	
	assertArrayEq(deviceDirectory->_devices,
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(3, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(5, 0, 1, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow());
}

TEST_F(DeviceDirectoryTests, clearDeviceDirectoryRow_penultimateDeviceEmptyBelow)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(4, 0, 4, 0),
		DeviceDirectoryRow(5, 0, 1, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	deviceDirectory->clearDeviceDirectoryRow(3);

	assertArrayEq(deviceDirectory->_devices,
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(5, 0, 1, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow());
}

TEST_F(DeviceDirectoryTests, clearDeviceDirectoryRow_topDeviceEmptyBelow)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(5, 0, 1, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	deviceDirectory->clearDeviceDirectoryRow(4);

	assertArrayEq(deviceDirectory->_devices,
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow());
}

TEST_F(DeviceDirectoryTests, findFreeRow_empty)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int result = deviceDirectory->findFreeRow();

	ASSERT_EQ(result, 0);
}

TEST_F(DeviceDirectoryTests, findFreeRow_beginning)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(3, 6, 3, 7),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int result = deviceDirectory->findFreeRow();

	ASSERT_EQ(result, 0);
}

TEST_F(DeviceDirectoryTests, findFreeRow_middle)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(2, 0, 1, 0),
		DeviceDirectoryRow(3, 0, 1, 0),
		DeviceDirectoryRow(4, 0, 1, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int result = deviceDirectory->findFreeRow();

	ASSERT_EQ(result, 3);
}

TEST_F(DeviceDirectoryTests, findFreeRow_middleWithOthersAfter)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(2, 0, 1, 0),
		DeviceDirectoryRow(3, 0, 1, 0),
		DeviceDirectoryRow(4, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(5, 0, 3, 0),
		DeviceDirectoryRow()
	};

	int result = deviceDirectory->findFreeRow();

	ASSERT_EQ(result, 3);
}

TEST_F(DeviceDirectoryTests, findFreeRow_last)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(2, 0, 1, 0),
		DeviceDirectoryRow(3, 0, 1, 0),
		DeviceDirectoryRow(4, 0, 2, 0),
		DeviceDirectoryRow(5, 0, 3, 0),
		DeviceDirectoryRow(6, 0, 5, 0),
		DeviceDirectoryRow(7, 0, 8, 0),
		DeviceDirectoryRow()
	};

	int result = deviceDirectory->findFreeRow();

	ASSERT_EQ(result, 6);
}

TEST_F(DeviceDirectoryTests, findFreeRow_full)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(2, 0, 1, 0),
		DeviceDirectoryRow(3, 0, 1, 0),
		DeviceDirectoryRow(4, 0, 2, 0),
		DeviceDirectoryRow(5, 0, 3, 0),
		DeviceDirectoryRow(6, 0, 5, 0),
		DeviceDirectoryRow(7, 0, 8, 0),
		DeviceDirectoryRow(8, 0, 13, 0)
	};

	int result = deviceDirectory->findFreeRow();

	ASSERT_EQ(result, -1);
}

TEST_F(DeviceDirectoryTests, findFreeSlaveID_empty)
{
	deviceDirectory->_maxDevices = 10;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	byte freeSlaveId = deviceDirectory->findFreeSlaveID();

	ASSERT_EQ(freeSlaveId, 2);
}

TEST_F(DeviceDirectoryTests, findFreeSlaveID_full)
{
	deviceDirectory->_maxDevices = 246;
	deviceDirectory->_devices = new DeviceDirectoryRow[246];
	for (int i = 0; i < 246; i++)
	{
		deviceDirectory->_devices[i].slaveId = i + 1;
	}

	byte freeSlaveId = deviceDirectory->findFreeSlaveID();

	ASSERT_EQ(freeSlaveId, 0);
}

TEST_F(DeviceDirectoryTests, findFreeSlaveID_partiallyFilled_case1)
{
	deviceDirectory->_maxDevices = 10;
	deviceDirectory->_devices = new DeviceDirectoryRow[10]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(1, 0, 2, 0),
		DeviceDirectoryRow(2, 0, 3, 0),
		DeviceDirectoryRow(3, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	byte freeSlaveId = deviceDirectory->findFreeSlaveID();

	ASSERT_EQ(freeSlaveId, 4);
}

TEST_F(DeviceDirectoryTests, findFreeSlaveID_partiallyFilled_case2)
{
	deviceDirectory->_maxDevices = 10;
	deviceDirectory->_devices = new DeviceDirectoryRow[10]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 3, 0),
		DeviceDirectoryRow(3, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	byte freeSlaveId = deviceDirectory->findFreeSlaveID();

	ASSERT_EQ(freeSlaveId, 4);
}

TEST_F(DeviceDirectoryTests, findFreeSlaveID_partiallyFilled_case3)
{
	deviceDirectory->_maxDevices = 10;
	deviceDirectory->_devices = new DeviceDirectoryRow[10]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(1, 0, 2, 0),
		DeviceDirectoryRow(4, 0, 3, 0),
		DeviceDirectoryRow(3, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	byte freeSlaveId = deviceDirectory->findFreeSlaveID();

	ASSERT_EQ(freeSlaveId, 2);
}

TEST_F(DeviceDirectoryTests, findFreeSlaveID_partiallyFilled_case4)
{
	deviceDirectory->_maxDevices = 10;
	deviceDirectory->_devices = new DeviceDirectoryRow[10]
	{
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(1, 0, 2, 0),
		DeviceDirectoryRow(4, 0, 3, 0),
		DeviceDirectoryRow(3, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	byte freeSlaveId = deviceDirectory->findFreeSlaveID();

	ASSERT_EQ(freeSlaveId, 2);
}

TEST_F(DeviceDirectoryTests, addDevice_success)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	When(Method(mock, findFreeRow)).Return(6);
	Fake(Method(mock, insertIntoRow));

	byte* name = (byte*)"Aleah";
	int row = deviceDirectory->addDevice(name, DeviceDirectoryRow(6, 7, 8, 9));

	ASSERT_EQ(row, 6);
	Verify(Method(mock, insertIntoRow).Using(6, name, DeviceDirectoryRow(6, 7, 8, 9))).Once();
}

TEST_F(DeviceDirectoryTests, addDevice_failure)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	When(Method(mock, findFreeRow)).Return(-1);
	Fake(Method(mock, insertIntoRow));
	
	byte* name = (byte*)"Aleah";
	int row = deviceDirectory->addDevice(name, DeviceDirectoryRow(6, 7, 8, 9));

	ASSERT_EQ(row, -1);
	Verify(Method(mock, insertIntoRow)).Never();
}

TEST_F(DeviceDirectoryTests, addOrReplaceDevice_deviceAdded)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	When(Method(mock, findDeviceForName)).Do([](byte* name, int &row)
	{
		row = -1;
		return nullptr;
	});
	When(Method(mock, addDevice)).Return(703);

	byte* name = (byte*)"Aleah";
	int row = deviceDirectory->addOrReplaceDevice(name, DeviceDirectoryRow(6, 7, 8, 9));

	ASSERT_EQ(row, 703);
	Verify(Method(mock, addDevice).Using(name, DeviceDirectoryRow(6, 7, 8, 9))).Once();
}

TEST_F(DeviceDirectoryTests, addOrReplaceDevice_deviceReplaced)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	auto device = DeviceDirectoryRow(10, 11, 12, 13);
	When(Method(mock, findDeviceForName)).Do([&device](byte* name, int &row)
	{
		row = 23;
		return &device;
	});
	Fake(Method(mock, insertIntoRow));

	byte* name = (byte*)"Aleah";
	int row = deviceDirectory->addOrReplaceDevice(name, DeviceDirectoryRow(6, 7, 8, 9));

	ASSERT_EQ(row, 23);
	Verify(Method(mock, insertIntoRow).Using(23, name, DeviceDirectoryRow(6, 7, 8, 9))).Once();
}

TEST_F(DeviceDirectoryTests, filterDevicesForSlave_empty)
{
	deviceDirectory->_maxDevices = 10;
	deviceDirectory->_devices = new DeviceDirectoryRow[10]
	{
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int numDeleted = deviceDirectory->filterDevicesForSlave(
		new byte*[3]{ (byte*)"Aleah", (byte*)"Chloe", (byte*)"Asher" },
		3, 4);

	ASSERT_EQ(numDeleted, 0);
}

TEST_F(DeviceDirectoryTests, findNextDevice_NoCondition_Case0_0)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	byte name[4];
	name[0] = 'N';
	name[1] = 'm';
	name[2] = 'e';
	When(Method(mock, getDeviceName)).AlwaysDo([&name](int row)
	{
		name[3] = '0' + row;
		return name;
	});

	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_deviceNameLength = 4;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int row = 0;
	byte* resultName;
	auto result = deviceDirectory->findNextDevice(resultName, row);
	ASSERT_EQ(resultName, name);
	ASSERT_EQ(row, 1);
	ASSERT_EQ(result, deviceDirectory->_devices + 0);
	assertArrayEq(resultName, 'N', 'm', 'e', '0');
}

TEST_F(DeviceDirectoryTests, findNextDevice_NoCondition_Case0_1)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	byte name[4];
	name[0] = 'N';
	name[1] = 'm';
	name[2] = 'e';
	When(Method(mock, getDeviceName)).AlwaysDo([&name](int row)
	{
		name[3] = '0' + row;
		return name;
	});

	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_deviceNameLength = 4;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int row = 1;
	byte* resultName;
	auto result = deviceDirectory->findNextDevice(resultName, row);
	ASSERT_EQ(resultName, name);
	ASSERT_EQ(row, 2);
	ASSERT_EQ(result, deviceDirectory->_devices + 1);
	assertArrayEq(resultName, 'N', 'm', 'e', '1');
}

TEST_F(DeviceDirectoryTests, findNextDevice_NoCondition_Case0_2)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	byte name[4];
	name[0] = 'N';
	name[1] = 'm';
	name[2] = 'e';
	When(Method(mock, getDeviceName)).AlwaysDo([&name](int row)
	{
		name[3] = '0' + row;
		return name;
	});

	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_deviceNameLength = 4;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int row = 2;
	byte* resultName;
	auto result = deviceDirectory->findNextDevice(resultName, row);
	ASSERT_EQ(resultName, name);
	ASSERT_EQ(row, -1);
	ASSERT_EQ(result, nullptr);
}

TEST_F(DeviceDirectoryTests, findNextDevice_NoCondition_Case1_0)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	byte name[4];
	name[0] = 'N';
	name[1] = 'm';
	name[2] = 'e';
	When(Method(mock, getDeviceName)).AlwaysDo([&name](int row)
	{
		name[3] = '0' + row;
		return name;
	});

	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_deviceNameLength = 4;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int row = 0;
	byte* resultName;
	auto result = deviceDirectory->findNextDevice(resultName, row);
	ASSERT_EQ(resultName, name);
	ASSERT_EQ(row, 1);
	ASSERT_EQ(result, deviceDirectory->_devices + 0);
	assertArrayEq(resultName, 'N', 'm', 'e', '0');
}

TEST_F(DeviceDirectoryTests, findNextDevice_NoCondition_Case1_1a)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	byte name[4];
	name[0] = 'N';
	name[1] = 'm';
	name[2] = 'e';
	When(Method(mock, getDeviceName)).AlwaysDo([&name](int row)
	{
		name[3] = '0' + row;
		return name;
	});

	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_deviceNameLength = 4;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int row = 1;
	byte* resultName;
	auto result = deviceDirectory->findNextDevice(resultName, row);
	ASSERT_EQ(resultName, name);
	ASSERT_EQ(row, 3);
	ASSERT_EQ(result, deviceDirectory->_devices + 2);
	assertArrayEq(resultName, 'N', 'm', 'e', '2');
}

TEST_F(DeviceDirectoryTests, findNextDevice_NoCondition_Case1_1b)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	byte name[4];
	name[0] = 'N';
	name[1] = 'm';
	name[2] = 'e';
	When(Method(mock, getDeviceName)).AlwaysDo([&name](int row)
	{
		name[3] = '0' + row;
		return name;
	});

	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_deviceNameLength = 4;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int row = 2;
	byte* resultName;
	auto result = deviceDirectory->findNextDevice(resultName, row);
	ASSERT_EQ(resultName, name);
	ASSERT_EQ(row, 3);
	ASSERT_EQ(result, deviceDirectory->_devices + 2);
	assertArrayEq(resultName, 'N', 'm', 'e', '2');
}

TEST_F(DeviceDirectoryTests, findNextDevice_NoCondition_Case1_2)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	byte name[4];
	name[0] = 'N';
	name[1] = 'm';
	name[2] = 'e';
	When(Method(mock, getDeviceName)).AlwaysDo([&name](int row)
	{
		name[3] = '0' + row;
		return name;
	});

	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_deviceNameLength = 4;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int row = 3;
	byte* resultName;
	auto result = deviceDirectory->findNextDevice(resultName, row);
	ASSERT_EQ(resultName, name);
	ASSERT_EQ(row, -1);
	ASSERT_EQ(result, nullptr);
}

TEST_F(DeviceDirectoryTests, findNextDevice_NoCondition_Case2_0a)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	byte name[4];
	name[0] = 'N';
	name[1] = 'm';
	name[2] = 'e';
	When(Method(mock, getDeviceName)).AlwaysDo([&name](int row)
	{
		name[3] = '0' + row;
		return name;
	});

	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_deviceNameLength = 4;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int row = 0;
	byte* resultName;
	auto result = deviceDirectory->findNextDevice(resultName, row);
	ASSERT_EQ(resultName, name);
	ASSERT_EQ(row, 2);
	ASSERT_EQ(result, deviceDirectory->_devices + 1);
	assertArrayEq(resultName, 'N', 'm', 'e', '1');
}

TEST_F(DeviceDirectoryTests, findNextDevice_NoCondition_Case2_0b)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	byte name[4];
	name[0] = 'N';
	name[1] = 'm';
	name[2] = 'e';
	When(Method(mock, getDeviceName)).AlwaysDo([&name](int row)
	{
		name[3] = '0' + row;
		return name;
	});

	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_deviceNameLength = 4;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int row = 1;
	byte* resultName;
	auto result = deviceDirectory->findNextDevice(resultName, row);
	ASSERT_EQ(resultName, name);
	ASSERT_EQ(row, 2);
	ASSERT_EQ(result, deviceDirectory->_devices + 1);
	assertArrayEq(resultName, 'N', 'm', 'e', '1');
}

TEST_F(DeviceDirectoryTests, findNextDevice_NoCondition_Case2_1)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	byte name[4];
	name[0] = 'N';
	name[1] = 'm';
	name[2] = 'e';
	When(Method(mock, getDeviceName)).AlwaysDo([&name](int row)
	{
		name[3] = '0' + row;
		return name;
	});

	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_deviceNameLength = 4;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int row = 2;
	byte* resultName;
	auto result = deviceDirectory->findNextDevice(resultName, row);
	ASSERT_EQ(resultName, name);
	ASSERT_EQ(row, 3);
	ASSERT_EQ(result, deviceDirectory->_devices + 2);
	assertArrayEq(resultName, 'N', 'm', 'e', '2');
}

TEST_F(DeviceDirectoryTests, findNextDevice_NoCondition_Case2_2)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	byte name[4];
	name[0] = 'N';
	name[1] = 'm';
	name[2] = 'e';
	When(Method(mock, getDeviceName)).AlwaysDo([&name](int row)
	{
		name[3] = '0' + row;
		return name;
	});

	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_deviceNameLength = 4;
	deviceDirectory->_devices = new DeviceDirectoryRow[7]
	{
		DeviceDirectoryRow(0, 0, 1, 0),
		DeviceDirectoryRow(1, 0, 1, 0),
		DeviceDirectoryRow(2, 0, 2, 0),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow(),
		DeviceDirectoryRow()
	};

	int row = 3;
	byte* resultName;
	auto result = deviceDirectory->findNextDevice(resultName, row);
	ASSERT_EQ(resultName, name);
	ASSERT_EQ(row, -1);
	ASSERT_EQ(result, nullptr);
}