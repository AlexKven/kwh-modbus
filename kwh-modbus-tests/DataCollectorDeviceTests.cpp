#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/device/DataCollectorDevice.h"
#include "../kwh-modbus/libraries/bitFunctions/BitFunctions.hpp"
#include "test_helpers.h"
#include "PointerTracker.h"

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

	PointerTracker tracker;

public:
	void SetUp()
	{
		device = new _DataCollectorDevice();
		tracker.addPointer(device);
	}

	void TearDown()
	{
	}
};

TEST_F_TRAITS(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_Case0,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	TimeScale scale = TimeScale::ms250;
	bool accumulate = false;
	byte dataSize = 1;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_TRUE(success);
	ASSERT_EQ(devType, 0x4010);
}

TEST_F_TRAITS(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_Case1,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	TimeScale scale = TimeScale::min10;
	bool accumulate = false;
	byte dataSize = 63;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_TRUE(success);
	ASSERT_EQ(devType, 0x53F0);
}

TEST_F_TRAITS(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_Case2,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	TimeScale scale = TimeScale::min10;
	bool accumulate = true;
	byte dataSize = 63;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_TRUE(success);
	ASSERT_EQ(devType, 0x73F0);
}

TEST_F_TRAITS(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_Case3,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	TimeScale scale = TimeScale::hr24;
	bool accumulate = true;
	byte dataSize = 63;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_TRUE(success);
	ASSERT_EQ(devType, 0x7FF0);
}

TEST_F_TRAITS(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_BadTimeScale,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	TimeScale scale = (TimeScale)8;
	bool accumulate = false;
	byte dataSize = 1;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_FALSE(success);
}

TEST_F_TRAITS(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_DataPacketSizeZero,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	TimeScale scale = TimeScale::ms250;
	bool accumulate = false;
	byte dataSize = 0;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_FALSE(success);
}

TEST_F_TRAITS(DataCollectorDeviceTests, getDataCollectorDeviceTypeFromParameters_DataPacketSizeTooLarge,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	TimeScale scale = TimeScale::ms250;
	bool accumulate = false;
	byte dataSize = 64;
	word devType;
	bool success = DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulate, scale, dataSize, devType);

	ASSERT_FALSE(success);
}

TEST_F_TRAITS(DataCollectorDeviceTests, getParametersFromDataCollectorDeviceType_Case0,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(DataCollectorDeviceTests, getParametersFromDataCollectorDeviceType_Case1,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(DataCollectorDeviceTests, getParametersFromDataCollectorDeviceType_Case2,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(DataCollectorDeviceTests, getParametersFromDataCollectorDeviceType_Case3,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(DataCollectorDeviceTests, getParametersFromDataCollectorDeviceType_Failure_NotPadded,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	TimeScale scale;
	bool accumulate;
	byte dataSize;
	word devType = 0x7FF1;
	bool success = DataCollectorDevice::getParametersFromDataCollectorDeviceType(devType, accumulate, scale, dataSize);

	ASSERT_FALSE(success);
}

TEST_F_TRAITS(DataCollectorDeviceTests, getParametersFromDataCollectorDeviceType_Failure_NotDataCollector,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	TimeScale scale;
	bool accumulate;
	byte dataSize;
	word devType = 0xBFF0;
	bool success = DataCollectorDevice::getParametersFromDataCollectorDeviceType(devType, accumulate, scale, dataSize);

	ASSERT_FALSE(success);
}

TEST_F_TRAITS(DataCollectorDeviceTests, init_failure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
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

TEST_F_TRAITS(DataCollectorDeviceTests, init_success,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(DataCollectorDeviceTests, getType_itworks,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	bool success = device->init(false, TimeScale::min10, 63);

	ASSERT_TRUE(success);
	ASSERT_EQ(device->getType(), 0x53F0);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_1page_1sec_8bits,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec1, 8);
	byte* buffer = tracker.addArray(new byte[6]);
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 6, 0, buffer, 6, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(dataPointSize, 8);
	ASSERT_EQ(buffer[0], 255);
	ASSERT_EQ(buffer[1], 1);
	ASSERT_EQ(buffer[2], 2);
	ASSERT_EQ(buffer[3], 3);
	ASSERT_EQ(buffer[4], 4);
	ASSERT_EQ(buffer[5], 5);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_1page_15sec_8bits,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec15, 8);
	byte* buffer = tracker.addArray(new byte[6]);
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 6, 0, buffer, 6, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(dataPointSize, 8);
	ASSERT_EQ(buffer[0], 255);
	ASSERT_EQ(buffer[1], 15);
	ASSERT_EQ(buffer[2], 30);
	ASSERT_EQ(buffer[3], 45);
	ASSERT_EQ(buffer[4], 60);
	ASSERT_EQ(buffer[5], 75);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_1page_250ms_8bits,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)(time * 5) + quarterSecondOffset;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (quarterSecondOffset != 0);
	});

	device->init(false, TimeScale::ms250, 8);
	byte* buffer = tracker.addArray(new byte[6]);
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 6, 0, buffer, 6, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(dataPointSize, 8);
	ASSERT_EQ(buffer[0], 255);
	ASSERT_EQ(buffer[1], 1);
	ASSERT_EQ(buffer[2], 2);
	ASSERT_EQ(buffer[3], 3);
	ASSERT_EQ(buffer[4], 255);
	ASSERT_EQ(buffer[5], 6);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_1page_1sec_8bits_largeBuffer,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec1, 8);
	byte* buffer = tracker.addArray(new byte[9]);
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 6, 0, buffer, 9, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(dataPointSize, 8);
	ASSERT_EQ(buffer[0], 255);
	ASSERT_EQ(buffer[1], 1);
	ASSERT_EQ(buffer[2], 2);
	ASSERT_EQ(buffer[3], 3);
	ASSERT_EQ(buffer[4], 4);
	ASSERT_EQ(buffer[5], 5);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_3page_1sec_8bits_page0,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec1, 8);
	byte* buffer = tracker.addArray(new byte[6]);
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 18, 0, buffer, 6, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 2);
	ASSERT_EQ(dataPointSize, 8);
	ASSERT_EQ(buffer[0], 255);
	ASSERT_EQ(buffer[1], 1);
	ASSERT_EQ(buffer[2], 2);
	ASSERT_EQ(buffer[3], 3);
	ASSERT_EQ(buffer[4], 4);
	ASSERT_EQ(buffer[5], 5);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_3page_1sec_8bits_page1,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec1, 8);
	byte* buffer = tracker.addArray(new byte[6]);
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 18, 1, buffer, 6, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 1);
	ASSERT_EQ(dataPointSize, 8);
	ASSERT_EQ(buffer[0], 6);
	ASSERT_EQ(buffer[1], 7);
	ASSERT_EQ(buffer[2], 8);
	ASSERT_EQ(buffer[3], 9);
	ASSERT_EQ(buffer[4], 10);
	ASSERT_EQ(buffer[5], 11);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_3page_1sec_8bits_page2,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec1, 8);
	byte* buffer = tracker.addArray(new byte[6]);
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 18, 2, buffer, 6, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 6);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(dataPointSize, 8);
	ASSERT_EQ(buffer[0], 12);
	ASSERT_EQ(buffer[1], 13);
	ASSERT_EQ(buffer[2], 14);
	ASSERT_EQ(buffer[3], 15);
	ASSERT_EQ(buffer[4], 16);
	ASSERT_EQ(buffer[5], 17);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_3page_1sec_8bits_partialPage,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src > 0);
	});

	device->init(false, TimeScale::sec1, 8);
	byte* buffer = tracker.addArray(new byte[6]);
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 14, 2, buffer, 6, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 2);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(dataPointSize, 8);
	ASSERT_EQ(buffer[0], 12);
	ASSERT_EQ(buffer[1], 13);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_2page_1sec_3bits_page0,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return true;
	});

	device->init(false, TimeScale::sec1, 3);
	byte* buffer = tracker.addArray(new byte[2]);
	buffer[0] = 0;
	buffer[1] = 0;
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 7, 0, buffer, 2, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 5);
	ASSERT_EQ(pagesRemaining, 1);
	ASSERT_EQ(dataPointSize, 3);
	Verify(Method(mock, readDataPoint)).Exactly(5);
	ASSERT_EQ(buffer[0], 136);
	ASSERT_EQ(buffer[1], 70);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_2page_1sec_3bits_page1,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return true;
	});

	device->init(false, TimeScale::sec1, 3);
	byte* buffer = tracker.addArray(new byte[2]);
	buffer[0] = 0;
	buffer[1] = 0;
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 7, 1, buffer, 2, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 2);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(dataPointSize, 3);
	Verify(Method(mock, readDataPoint)).Exactly(2);
	ASSERT_EQ(buffer[0], 53);
	ASSERT_EQ(buffer[1], 0);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_3page_24hr_5bits_page0,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)(time / 43200);
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src != 4);
	});

	device->init(false, TimeScale::hr24, 5);
	byte* buffer = tracker.addArray(new byte[3]);
	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = 0;
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 10, 0, buffer, 3, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 4);
	ASSERT_EQ(pagesRemaining, 2);
	ASSERT_EQ(dataPointSize, 5);
	Verify(Method(mock, readDataPoint)).Exactly(4);
	ASSERT_EQ(buffer[0], 0x40);
	ASSERT_EQ(buffer[1], 0x7C);
	ASSERT_EQ(buffer[2], 3);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_3page_24hr_5bits_page1,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)(time / 43200);
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src != 4);
	});

	device->init(false, TimeScale::hr24, 5);
	byte* buffer = tracker.addArray(new byte[3]);
	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = 0;
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 10, 1, buffer, 3, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 4);
	ASSERT_EQ(pagesRemaining, 1);
	ASSERT_EQ(dataPointSize, 5);
	Verify(Method(mock, readDataPoint)).Exactly(4);
	ASSERT_EQ(buffer[0], 0x48);
	ASSERT_EQ(buffer[1], 0x31);
	ASSERT_EQ(buffer[2], 7);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_3page_24hr_5bits_page2,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)(time / 43200);
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return (src != 4);
	});

	device->init(false, TimeScale::hr24, 5);
	byte* buffer = tracker.addArray(new byte[3]);
	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = 0;
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 10, 2, buffer, 3, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 2);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(dataPointSize, 5);
	Verify(Method(mock, readDataPoint)).Exactly(2);
	ASSERT_EQ(buffer[0], 0x50);
	ASSERT_EQ(buffer[1], 2);
	ASSERT_EQ(buffer[2], 0);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_1page_1min_8bits_timeOffset,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return true;
	});

	device->init(false, TimeScale::min1, 8);
	byte* buffer = tracker.addArray(new byte[20]);
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(70, 8, 0, buffer, 20, 0, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 8);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(dataPointSize, 8);
	ASSERT_EQ(buffer[0], 70);
	ASSERT_EQ(buffer[1], 130);
	ASSERT_EQ(buffer[2], 190);
	ASSERT_EQ(buffer[3], 250);
	ASSERT_EQ(buffer[4], 54);
	ASSERT_EQ(buffer[5], 114);
	ASSERT_EQ(buffer[6], 174);
	ASSERT_EQ(buffer[7], 234);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_3page_withMaxPoints_1sec_3bits_page0,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return true;
	});

	device->init(false, TimeScale::sec1, 3);
	byte* buffer = tracker.addArray(new byte[2]);
	buffer[0] = 0;
	buffer[1] = 0;
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 7, 0, buffer, 2, 3, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 3);
	ASSERT_EQ(pagesRemaining, 2);
	ASSERT_EQ(dataPointSize, 3);
	Verify(Method(mock, readDataPoint)).Exactly(3);
	ASSERT_EQ(buffer[0], 136);
	ASSERT_EQ(buffer[1], 0);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_3page_withMaxPoints_1sec_3bits_page1,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return true;
	});

	device->init(false, TimeScale::sec1, 3);
	byte* buffer = tracker.addArray(new byte[2]);
	buffer[0] = 0;
	buffer[1] = 0;
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 7, 1, buffer, 2, 3, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 3);
	ASSERT_EQ(pagesRemaining, 1);
	ASSERT_EQ(dataPointSize, 3);
	Verify(Method(mock, readDataPoint)).Exactly(3);
	ASSERT_EQ(buffer[0], 0x63);
	ASSERT_EQ(buffer[1], 1);
}

TEST_F_TRAITS(DataCollectorDeviceTests, readData_Success_3page_withMaxPoints_1sec_3bits_page2,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_MOCK;
	When(Method(mock, readDataPoint)).AlwaysDo([](uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits) {
		byte src = (byte)time;
		BitFunctions::copyBits(&src, dataBuffer, (byte)0, (byte)0, dataSizeBits);
		return true;
	});

	device->init(false, TimeScale::sec1, 3);
	byte* buffer = tracker.addArray(new byte[2]);
	buffer[0] = 0;
	buffer[1] = 0;
	byte dummy;
	byte numDataPointsInPage;
	byte pagesRemaining;
	byte dataPointSize;

	bool success = device->readData(0, 7, 2, buffer, 2, 3, numDataPointsInPage, pagesRemaining, dataPointSize);

	ASSERT_TRUE(success);
	ASSERT_EQ(numDataPointsInPage, 1);
	ASSERT_EQ(pagesRemaining, 0);
	ASSERT_EQ(dataPointSize, 3);
	Verify(Method(mock, readDataPoint)).Exactly(1);
	ASSERT_EQ(buffer[0], 6);
	ASSERT_EQ(buffer[1], 0);
}