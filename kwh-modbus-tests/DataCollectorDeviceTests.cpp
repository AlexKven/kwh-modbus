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