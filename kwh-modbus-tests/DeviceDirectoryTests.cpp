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
		deviceDirectory->_deviceTypes[i] = i;
		deviceDirectory->_slaveIds[i] = i;
	}

	ASSERT_EQ(deviceDirectory->_maxDevices, 5);
	ASSERT_EQ(deviceDirectory->_deviceNameLength, 11);

	ASSERT_EQ(deviceDirectory->_deviceNames[0], 0);
	ASSERT_EQ(deviceDirectory->_deviceNames[54], 54);
	ASSERT_EQ(deviceDirectory->_deviceTypes[0], 0);
	ASSERT_EQ(deviceDirectory->_deviceTypes[4], 4);
	ASSERT_EQ(deviceDirectory->_slaveIds[0], 0);
	ASSERT_EQ(deviceDirectory->_slaveIds[4], 4);
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
	ASSERT_EQ(deviceDirectory->_deviceTypes[0], 0);
	ASSERT_EQ(deviceDirectory->_deviceTypes[4], 0);
	ASSERT_EQ(deviceDirectory->_slaveIds[0], 0);
	ASSERT_EQ(deviceDirectory->_slaveIds[4], 0);
	ASSERT_EQ(deviceDirectory->_persistentStore, &persistentStore);
}

TEST_F(DeviceDirectoryTests, init_maxMemory)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	Fake(OverloadedMethod(mock, init, void(word, word, byte**)));

	word numDevices;
	deviceDirectory->init(500, 11, numDevices);

	ASSERT_EQ(numDevices, 35);
	Verify(OverloadedMethod(mock, init, void(word, word, byte**)).Using(11, 35, nullptr)).Once();
}

TEST_F(DeviceDirectoryTests, findDeviceForName_empty)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	setupCompareName_ReturnTrueOn(mock, 3);
	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceTypes = new word(0);
	deviceDirectory->_slaveIds = new byte(0);

	word devType;
	byte slaveId;
	int row;
	bool found = deviceDirectory->findDeviceForName((byte*)"fiver", devType, slaveId, row);

	ASSERT_FALSE(found);
	ASSERT_EQ(slaveId, 0);
	ASSERT_EQ(devType, 0);
	ASSERT_EQ(row, -1);
	Verify(Method(mock, compareName).Using(Any<word>(), Any<byte*>())).Never();
}

TEST_F(DeviceDirectoryTests, findDeviceForName_emptyish)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	setupCompareName_ReturnTrueOn(mock, 3);
	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceTypes = new word[2]{ 1, 0 };
	deviceDirectory->_slaveIds = new byte[2]{ 0, 0 };

	word devType;
	byte slaveId;
	int row;
	bool found = deviceDirectory->findDeviceForName((byte*)"fiver", devType, slaveId, row);

	ASSERT_FALSE(found);
	ASSERT_EQ(slaveId, 0);
	ASSERT_EQ(devType, 0);
	ASSERT_EQ(row, -1);
	Verify(Method(mock, compareName).Using(Any<word>(), Any<byte*>())).Never();
}

TEST_F(DeviceDirectoryTests, findDeviceForName_exhaustedList)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	setupCompareName_ReturnTrueOn(mock, 6);
	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceTypes = new word[6]{ 1, 4, 7, 10, 13, 16 };
	deviceDirectory->_slaveIds = new byte[6]{ 0, 3, 6, 9, 12, 15 };

	word devType;
	byte slaveId;
	int row;
	bool found = deviceDirectory->findDeviceForName((byte*)"fiver", devType, slaveId, row);

	ASSERT_FALSE(found);
	ASSERT_EQ(slaveId, 0);
	ASSERT_EQ(devType, 0);
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
	deviceDirectory->_deviceTypes = new word[6]{ 1, 4, 7, 10, 13, 16 };
	deviceDirectory->_slaveIds = new byte[6]{ 0, 3, 6, 9, 12, 15 };

	word devType;
	byte slaveId;
	int row;
	bool found = deviceDirectory->findDeviceForName((byte*)"fiver", devType, slaveId, row);

	ASSERT_TRUE(found);
	ASSERT_EQ(slaveId, 9);
	ASSERT_EQ(devType, 10);
	ASSERT_EQ(row, 3);
	Verify(Method(mock, compareName).Using(0, Any<byte*>())).Never();
	Verify(Method(mock, compareName).Using(1, Any<byte*>())).Once();
	Verify(Method(mock, compareName).Using(3, Any<byte*>())).Once();
	Verify(Method(mock, compareName).Using(5, Any<byte*>())).Never();
}