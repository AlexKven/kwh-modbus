#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/deviceDirectory/DeviceDirectory.hpp"
#include "test_helpers.h"

using namespace fakeit;

class DeviceDirectoryTests : public ::testing::Test
{
protected:
	word * registerArray;
	DeviceDirectory<byte*> *deviceDirectory = new DeviceDirectory<byte*>();

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