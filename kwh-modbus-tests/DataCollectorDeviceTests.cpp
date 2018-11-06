#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/device/DataCollectorDevice.h"
#include "../kwh-modbus/libraries/bitFunctions/bitFunctions.h"
#include "test_helpers.h"

using namespace fakeit;

#define USE_MOCK Mock<_DataCollectorDevice> mock(*device)

class _DataCollectorDevice : public DataCollectorDevice
{
public:
	virtual bool readDataPoint(uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte bufferSizeBits)
	{
		return false;
	}
};

class DataCollectorDeviceTests : public ::testing::Test
{
protected:
	_DataCollectorDevice *device;

public:
	void SetUp()
	{
		device = new _DataCollectorDevice();
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

TEST_F(DataCollectorDeviceTests, readData_Success_1page_1sec_8bits)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec1, 8);
	byte* buffer = new byte[6];
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;


	bool success = device->readData(0, 6, 0, buffer, 6, numDataPointsInPage, pagesRemaining);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(buffer[0], 255);
	ASSERT_EQ(buffer[1], 1);
	ASSERT_EQ(buffer[2], 2);
	ASSERT_EQ(buffer[3], 3);
	ASSERT_EQ(buffer[4], 4);
	ASSERT_EQ(buffer[5], 5);

	delete[] buffer;
}

TEST_F(DataCollectorDeviceTests, readData_Success_1page_15sec_8bits)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec15, 8);
	byte* buffer = new byte[6];
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;


	bool success = device->readData(0, 6, 0, buffer, 6, numDataPointsInPage, pagesRemaining);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(buffer[0], 255);
	ASSERT_EQ(buffer[1], 15);
	ASSERT_EQ(buffer[2], 30);
	ASSERT_EQ(buffer[3], 45);
	ASSERT_EQ(buffer[4], 60);
	ASSERT_EQ(buffer[5], 75);

	delete[] buffer;
}

TEST_F(DataCollectorDeviceTests, readData_Success_1page_250ms_8bits)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)(time * 5) + quarterSecondOffset;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (quarterSecondOffset != 0);
	});

	device->init(false, TimeScale::ms250, 8);
	byte* buffer = new byte[6];
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;


	bool success = device->readData(0, 6, 0, buffer, 6, numDataPointsInPage, pagesRemaining);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(buffer[0], 255);
	ASSERT_EQ(buffer[1], 1);
	ASSERT_EQ(buffer[2], 2);
	ASSERT_EQ(buffer[3], 3);
	ASSERT_EQ(buffer[4], 255);
	ASSERT_EQ(buffer[5], 6);

	delete[] buffer;
}

TEST_F(DataCollectorDeviceTests, readData_Success_1page_1sec_8bits_largeBuffer)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec1, 8);
	byte* buffer = new byte[9];
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;


	bool success = device->readData(0, 6, 0, buffer, 9, numDataPointsInPage, pagesRemaining);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(buffer[0], 255);
	ASSERT_EQ(buffer[1], 1);
	ASSERT_EQ(buffer[2], 2);
	ASSERT_EQ(buffer[3], 3);
	ASSERT_EQ(buffer[4], 4);
	ASSERT_EQ(buffer[5], 5);

	delete[] buffer;
}

TEST_F(DataCollectorDeviceTests, readData_Success_3page_1sec_8bits_page0)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec1, 8);
	byte* buffer = new byte[6];
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;


	bool success = device->readData(0, 18, 0, buffer, 6, numDataPointsInPage, pagesRemaining);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 2);
	ASSERT_EQ(buffer[0], 255);
	ASSERT_EQ(buffer[1], 1);
	ASSERT_EQ(buffer[2], 2);
	ASSERT_EQ(buffer[3], 3);
	ASSERT_EQ(buffer[4], 4);
	ASSERT_EQ(buffer[5], 5);

	delete[] buffer;
}

TEST_F(DataCollectorDeviceTests, readData_Success_3page_1sec_8bits_page1)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec1, 8);
	byte* buffer = new byte[6];
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;


	bool success = device->readData(0, 18, 1, buffer, 6, numDataPointsInPage, pagesRemaining);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 1);
	ASSERT_EQ(buffer[0], 6);
	ASSERT_EQ(buffer[1], 7);
	ASSERT_EQ(buffer[2], 8);
	ASSERT_EQ(buffer[3], 9);
	ASSERT_EQ(buffer[4], 10);
	ASSERT_EQ(buffer[5], 11);

	delete[] buffer;
}

TEST_F(DataCollectorDeviceTests, readData_Success_3page_1sec_8bits_page2)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec1, 8);
	byte* buffer = new byte[6];
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;


	bool success = device->readData(0, 18, 2, buffer, 6, numDataPointsInPage, pagesRemaining);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(buffer[0], 12);
	ASSERT_EQ(buffer[1], 13);
	ASSERT_EQ(buffer[2], 14);
	ASSERT_EQ(buffer[3], 15);
	ASSERT_EQ(buffer[4], 16);
	ASSERT_EQ(buffer[5], 17);

	delete[] buffer;
}

TEST_F(DataCollectorDeviceTests, readData_Success_3page_1sec_8bits_partialPage)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec1, 8);
	byte* buffer = new byte[6];
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;


	bool success = device->readData(0, 14, 2, buffer, 6, numDataPointsInPage, pagesRemaining);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 2);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(buffer[0], 12);
	ASSERT_EQ(buffer[1], 13);

	delete[] buffer;
}

TEST_F(DataCollectorDeviceTests, readData_Success_2page_1sec_3bits_page0)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return true;
	});

	device->init(false, TimeScale::sec1, 3);
	byte* buffer = new byte[2];
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;


	bool success = device->readData(0, 7, 0, buffer, 2, numDataPointsInPage, pagesRemaining);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 5);
	ASSERT_EQ(pagesRemaining, 1);
	ASSERT_EQ(buffer[0], 136);
	ASSERT_EQ(buffer[1], 198);

	delete[] buffer;
}