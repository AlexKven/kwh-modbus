#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/device/DataCollectorDevice.h"
#include "test_helpers.h"

using namespace fakeit;

class DataCollectorDeviceTests : public ::testing::Test
{
protected:
	DataCollectorDevice *device;

public:
	void SetUp()
	{
		device = new DataCollectorDevice();
	}

	void TearDown()
	{
		delete device;
	}
};

TEST_F(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_Case0)
{
	TimeScale scale = TimeScale::ms250;
	bool accumulate = false;
	byte dataSize = 1;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_TRUE(success);
	ASSERT_EQ(devType, 0x4010);
}

TEST_F(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_Case1)
{
	TimeScale scale = TimeScale::min10;
	bool accumulate = false;
	byte dataSize = 63;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_TRUE(success);
	ASSERT_EQ(devType, 0x53F0);
}

TEST_F(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_Case2)
{
	TimeScale scale = TimeScale::min10;
	bool accumulate = true;
	byte dataSize = 63;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_TRUE(success);
	ASSERT_EQ(devType, 0x73F0);
}

TEST_F(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_Case3)
{
	TimeScale scale = TimeScale::hr24;
	bool accumulate = true;
	byte dataSize = 63;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_TRUE(success);
	ASSERT_EQ(devType, 0x7FF0);
}

TEST_F(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_BadTimeScale)
{
	TimeScale scale = (TimeScale)8;
	bool accumulate = false;
	byte dataSize = 1;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_FALSE(success);
}

TEST_F(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_DataPacketSizeZero)
{
	TimeScale scale = TimeScale::ms250;
	bool accumulate = false;
	byte dataSize = 0;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_FALSE(success);
}

TEST_F(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_DataPacketSizeTooLarge)
{
	TimeScale scale = TimeScale::ms250;
	bool accumulate = false;
	byte dataSize = 64;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_FALSE(success);
}

TEST_F(DataCollectorDeviceTests, getParametersFromDataCollectorDeviceType_Case0)
{
	TimeScale scale;
	bool accumulate;
	byte dataSize;
	word devType = 0x4010;
	bool success = DataCollectorDevice::getParametersFromDataCollectorDeviceType(devType, accumulate, scale, dataSize);

	ASSERT_TRUE(success);
	ASSERT_FALSE(accumulate);
	ASSERT_EQ(scale, TimeScale::ms250);
	ASSERT_EQ(dataSize, 1);
}

TEST_F(DataCollectorDeviceTests, getParametersFromDataCollectorDeviceType_Case1)
{
	TimeScale scale;
	bool accumulate;
	byte dataSize;
	word devType = 0x53F0;
	bool success = DataCollectorDevice::getParametersFromDataCollectorDeviceType(devType, accumulate, scale, dataSize);

	ASSERT_TRUE(success);
	ASSERT_FALSE(accumulate);
	ASSERT_EQ(scale, TimeScale::min10);
	ASSERT_EQ(dataSize, 63);
}

TEST_F(DataCollectorDeviceTests, getParametersFromDataCollectorDeviceType_Case2)
{
	TimeScale scale;
	bool accumulate;
	byte dataSize;
	word devType = 0x73F0;
	bool success = DataCollectorDevice::getParametersFromDataCollectorDeviceType(devType, accumulate, scale, dataSize);

	ASSERT_TRUE(success);
	ASSERT_TRUE(accumulate);
	ASSERT_EQ(scale, TimeScale::min10);
	ASSERT_EQ(dataSize, 63);
}

TEST_F(DataCollectorDeviceTests, getParametersFromDataCollectorDeviceType_Case3)
{
	TimeScale scale;
	bool accumulate;
	byte dataSize;
	word devType = 0x7FF0;
	bool success = DataCollectorDevice::getParametersFromDataCollectorDeviceType(devType, accumulate, scale, dataSize);

	ASSERT_TRUE(success);
	ASSERT_TRUE(accumulate);
	ASSERT_EQ(scale, TimeScale::hr24);
	ASSERT_EQ(dataSize, 63);
}

TEST_F(DataCollectorDeviceTests, getParametersFromDataCollectorDeviceType_Failure_NotPadded)
{
	TimeScale scale;
	bool accumulate;
	byte dataSize;
	word devType = 0x7FF1;
	bool success = DataCollectorDevice::getParametersFromDataCollectorDeviceType(devType, accumulate, scale, dataSize);

	ASSERT_FALSE(success);
}

TEST_F(DataCollectorDeviceTests, getParametersFromDataCollectorDeviceType_Failure_NotDataCollector)
{
	TimeScale scale;
	bool accumulate;
	byte dataSize;
	word devType = 0xBFF0;
	bool success = DataCollectorDevice::getParametersFromDataCollectorDeviceType(devType, accumulate, scale, dataSize);

	ASSERT_FALSE(success);
}

TEST_F(DataCollectorDeviceTests, init_failure)
{
	device->_accumulateData = false;
	device->_dataPacketSize = 1;
	device->_timeScale = TimeScale::min10;

	bool success = device->init(true, TimeScale::ms250, 255);

	ASSERT_FALSE(success);
	ASSERT_FALSE(device->_accumulateData);
	ASSERT_EQ(device->_dataPacketSize, 1);
	ASSERT_EQ(device->_timeScale, TimeScale::min10);
}

TEST_F(DataCollectorDeviceTests, init_success)
{
	device->_accumulateData = false;
	device->_dataPacketSize = 1;
	device->_timeScale = TimeScale::min10;

	bool success = device->init(true, TimeScale::ms250, 5);

	ASSERT_TRUE(success);
	ASSERT_TRUE(device->_accumulateData);
	ASSERT_EQ(device->_dataPacketSize, 5);
	ASSERT_EQ(device->_timeScale, TimeScale::ms250);
}

TEST_F(DataCollectorDeviceTests, getType_itworks)
{
	bool success = device->init(false, TimeScale::min10, 63);

	ASSERT_TRUE(success);
	ASSERT_EQ(device->getType(), 0x53F0);
}