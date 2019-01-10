#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/modbus/ModbusArray.h"
#include "../kwh-modbus/libraries/slave/Slave.hpp"
#include "../kwh-modbus/libraries/modbusSlave/ModbusSlave.hpp"
#include "../kwh-modbus/mock/MockSerialStream.h"
#include "WindowsSystemFunctions.h"
#include "test_helpers.h"
#include "PointerTracker.h"

using namespace fakeit;

typedef ModbusSlave<MockSerialStream, WindowsSystemFunctions, ModbusArray> T_MODBUS;
typedef Slave<ModbusSlave<MockSerialStream, WindowsSystemFunctions, ModbusArray>, WindowsSystemFunctions> T_SLAVE;

#define MOCK_SLAVE Mock<T_SLAVE> mock(*slave); \
T_SLAVE & mSlave = mock.get()

class SlaveTests : public ::testing::Test
{
protected:
	word *registerArray;
	MockSerialStream *serial = new MockSerialStream();
	WindowsSystemFunctions *system = new WindowsSystemFunctions();
	T_MODBUS *modbus = new T_MODBUS();
	T_SLAVE *slave = new T_SLAVE();
	Mock<Device> *mockDevices = nullptr;

	PointerTracker tracker;

	void SetupOutOfRangeRegisterArray()
	{
		modbus->init(registerArray, 20, 15, 20);
		slave->config(system, modbus);
	}

	void SetupDevices(Device **devices, word count, std::function<void(int, Mock<Device>&)> devMockCallback = nullptr)
	{
		if (mockDevices != nullptr)
			delete mockDevices;
		mockDevices = new Mock<Device>[count];
		for (word i = 0; i < count; i++)
		{
			When(Method(mockDevices[i], getType)).AlwaysReturn(7);
			When(Method(mockDevices[i], readData)).AlwaysReturn(false);
			devices[i] = &mockDevices[i].get();
		}
	}

	void ZeroRegisterArray()
	{
		for (int i = 0; i < 12; i++)
			registerArray[i] = 0;
	}

	void ZeroDataBuffer()
	{
		for (int i = 0; i < slave->_dataBufferSize; i++)
			slave->_dataBuffer[i] = 0;
	}
public:
	void SetUp()
	{
		registerArray = new word[15];
		modbus->init(registerArray, 0, 15, 20);
		slave->config(system, modbus);
		slave->_hregCount = 15;
		tracker.addPointers(modbus, slave, serial, system);
		tracker.addArrays(registerArray);
	}

	void TearDown()
	{
	}
};

TEST_F_TRAITS(SlaveTests, SlaveTests_setOutgoingState_Idle_Success,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	slave->displayedStateInvalid = true;
	slave->_state = sIdle;
	slave->_deviceCount = 4;
	slave->_deviceNameLength = 703;
	bool success = slave->setOutgoingState();

	ASSERT_TRUE(success);
	ASSERT_EQ(slave->displayedStateInvalid, false);
	ASSERT_EQ(registerArray[0], sIdle);
	ASSERT_EQ(registerArray[1], 1 << 8);
	ASSERT_EQ(registerArray[2], 4);
	ASSERT_EQ(registerArray[3], 703);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_setOutgoingState_Idle_Failure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	slave->displayedStateInvalid = true;
	SetupOutOfRangeRegisterArray();
	slave->_state = sIdle;
	slave->_deviceCount = 4;
	slave->_deviceNameLength = 703;
	bool success = slave->setOutgoingState();

	ASSERT_FALSE(success);
	ASSERT_EQ(slave->displayedStateInvalid, true);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_setOutgoingState_DisplayDevInfo_Success,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	slave->_state = sDisplayDevInfo;
	slave->displayedStateInvalid = true;

	Device **devices = tracker.addArray(new Device*[3]);
	SetupDevices(devices, 3);
	byte **names = tracker.addArray(new byte*[3]);
	names[0] = (byte*)"dev01";
	names[1] = (byte*)"dev02";
	names[2] = (byte*)"dev03";

	slave->init(3, 5, 12, 1, devices, names);

	// Select device #1
	registerArray[2] = 1;

	bool success = slave->setOutgoingState();

	ASSERT_EQ(slave->displayedStateInvalid, false);
	ASSERT_TRUE(success);
	assertArrayEq(registerArray,
		sDisplayDevInfo,
		(word)1,
		(word)7,
		(byte)'d',
		(byte)'e',
		(byte)'v',
		(byte)'0',
		(byte)'2',
		(byte)0,
		(word)0,
		(word)0);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_setOutgoingState_DisplayDevData_TimeNotSet,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	slave->_state = sDisplayDevData;
	slave->displayedStateInvalid = true;

	Device **devices = tracker.addArray(new Device*[1]);
	SetupDevices(devices, 1);
	byte **names = tracker.addArray(new byte*[1]);
	names[0] = (byte*)"dev00";

	slave->init(1, 5, 12, 1, devices, names);

	// Select device #0
	registerArray[2] = 0;

	// Select 435 as the requested start time
	registerArray[3] = 179;
	registerArray[4] = 1;

	// Request 16 data points
	registerArray[5] = 16;

	// Page 0, and no limit to how many data points we can process
	registerArray[6] = 0;

	bool success = slave->setOutgoingState();

	ASSERT_EQ(slave->displayedStateInvalid, false);
	ASSERT_TRUE(success);
	assertArrayEq(registerArray,
		sDisplayDevData, (word)1);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_setOutgoingState_DisplayDevData_DeviceDoesntSendData,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	slave->_state = sDisplayDevData;
	slave->displayedStateInvalid = true;

	Device **devices = tracker.addArray(new Device*[1]);
	SetupDevices(devices, 1);
	byte **names = tracker.addArray(new byte*[1]);
	names[0] = (byte*)"dev00";

	slave->init(1, 5, 12, 1, devices, names);
	ZeroRegisterArray();
	ZeroDataBuffer();
	slave->_clockSet = 1; // Time is no longer "never set"

	// Select device #0
	registerArray[2] = 0;

	// Select 65715 as the requested start time
	registerArray[3] = 179;
	registerArray[4] = 1;

	// Request 16 data points
	registerArray[5] = 16;

	// Page 0, and no limit to how many data points we can process
	registerArray[6] = 0;

	bool success = slave->setOutgoingState();

	ASSERT_EQ(slave->displayedStateInvalid, false);
	ASSERT_TRUE(success);
	assertArrayEq(registerArray,
		sDisplayDevData, (word)2);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_setOutgoingState_DisplayDevData_Success_4bitData,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	slave->_state = sDisplayDevData;
	slave->displayedStateInvalid = true;

	uint32_t passedStartTime;
	word passedNumPoints;
	byte passedPage;
	word passedBufferSize;
	byte passedMaxPoints;

	Device **devices = tracker.addArray(new Device*[1]);
	SetupDevices(devices, 1);
	When(Method(mockDevices[0], readData)).AlwaysDo([&](uint32_t startTime, word numPoints, byte page, byte * buffer, word bufferSize,
		byte maxPoints, byte & outDataPointsCount, byte & outPagesRemaining, byte &outDataPointSize)
	{
		passedStartTime = startTime;
		passedNumPoints = numPoints;
		passedPage = page;
		passedBufferSize = bufferSize;
		passedMaxPoints = maxPoints;
		for (int i = 0; i < 8; i++)
		{
			BitFunctions::copyBits(&i, buffer, 0, 4 * i, 4);
		}
		outDataPointsCount = 8;
		outPagesRemaining = 1;
		outDataPointSize = 4;
		return true;
	});
	byte **names = tracker.addArray(new byte*[1]);
	names[0] = (byte*)"dev00";

	slave->init(1, 5, 12, 10, devices, names);
	ZeroRegisterArray();
	ZeroDataBuffer();
	slave->_clockSet = 1; // Time is no longer "never set"

	// Select device #0
	registerArray[2] = 0;

	// Select 65715 as the requested start time
	registerArray[3] = 179;
	registerArray[4] = 1;

	// Request 16 data points
	registerArray[5] = 16;

	// Page 0, and no limit to how many data points we can process
	registerArray[6] = 0;

	bool success = slave->setOutgoingState();

	ASSERT_EQ(slave->displayedStateInvalid, false);
	ASSERT_TRUE(success);
	ASSERT_EQ(passedStartTime, 65715);
	ASSERT_EQ(passedPage, 0);
	ASSERT_EQ(passedBufferSize, 10);
	ASSERT_EQ(passedMaxPoints, 0);
	assertArrayEq(registerArray,
		sDisplayDevData,
		(word)0,
		(word)179,
		(word)1,
		(word)0x0408,
		(word)0x0100,
		(word)0x3210,
		(word)0x7654);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_setOutgoingState_DisplayDevData_Success_5bitData,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	slave->_state = sDisplayDevData;
	slave->displayedStateInvalid = true;

	uint32_t passedStartTime;
	word passedNumPoints;
	byte passedPage;
	word passedBufferSize;
	byte passedMaxPoints;

	Device **devices = tracker.addArray(new Device*[1]);
	SetupDevices(devices, 1);
	When(Method(mockDevices[0], readData)).AlwaysDo([&](uint32_t startTime, word numPoints, byte page, byte * buffer, word bufferSize,
		byte maxPoints, byte & outDataPointsCount, byte & outPagesRemaining, byte &outDataPointSize)
	{
		passedStartTime = startTime;
		passedNumPoints = numPoints;
		passedPage = page;
		passedBufferSize = bufferSize;
		passedMaxPoints = maxPoints;
		for (int i = 0; i < 6; i++)
		{
			BitFunctions::copyBits(&i, buffer, 0, 5 * i, 5);
		}
		outDataPointsCount = 6;
		outPagesRemaining = 2;
		outDataPointSize = 5;
		return true;
	});
	byte **names = tracker.addArray(new byte*[1]);
	names[0] = (byte*)"dev00";

	slave->init(1, 5, 12, 10, devices, names);
	ZeroRegisterArray();
	ZeroDataBuffer();
	slave->_clockSet = 1; // Time is no longer "never set"

	// Select device #0
	registerArray[2] = 0;

	// Select 65715 as the requested start time
	registerArray[3] = 179;
	registerArray[4] = 1;

	// Request 16 data points
	registerArray[5] = 16;

	// Page 1, and no limit to how many data points we can process
	registerArray[6] = 1;

	bool success = slave->setOutgoingState();

	ASSERT_EQ(slave->displayedStateInvalid, false);
	ASSERT_TRUE(success);
	ASSERT_EQ(passedStartTime, 65715);
	ASSERT_EQ(passedPage, 1);
	ASSERT_EQ(passedBufferSize, 10);
	ASSERT_EQ(passedMaxPoints, 0);
	assertArrayEq(registerArray,
		sDisplayDevData,
		(word)0,
		(word)179,
		(word)1,
		(word)0x0506,
		(word)0x0201,
		(word)0x8820,
		(word)0x0A41);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_processIncomingState_Idle,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	MOCK_SLAVE;
	mSlave.displayedStateInvalid = false;

	mSlave._state = sIdle;

	registerArray[0] = sReceivedRequest;
	registerArray[1] = 0;

	bool processed;
	bool success = mSlave.processIncomingState(processed);

	ASSERT_TRUE(processed);
	ASSERT_TRUE(success);
	ASSERT_EQ(mSlave._state, sIdle);
	ASSERT_EQ(mSlave.displayedStateInvalid, true);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_processIncomingState_ChangeSlaveId,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	MOCK_SLAVE;
	mSlave.displayedStateInvalid = false;

	mSlave._state = sIdle;
	mSlave._deviceCount = 4;
	mSlave._deviceNameLength = 703;
	mSlave._modbus->setSlaveId(14);

	registerArray[0] = sReceivedRequest;
	registerArray[1] = 1;
	registerArray[2] = 41;

	bool processed;
	bool success = mSlave.processIncomingState(processed);

	ASSERT_TRUE(processed);
	ASSERT_TRUE(success);
	ASSERT_EQ(mSlave._state, sIdle);
	ASSERT_EQ(mSlave.getSlaveId(), 41);
	ASSERT_EQ(mSlave.displayedStateInvalid, true);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_processIncomingState_Nothing,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	MOCK_SLAVE;
	mSlave.displayedStateInvalid = false;

	mSlave._state = sIdle;

	registerArray[0] = 0;

	bool processed;
	bool success = mSlave.processIncomingState(processed);

	ASSERT_FALSE(processed);
	ASSERT_TRUE(success);
	ASSERT_EQ(mSlave.displayedStateInvalid, false);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_processIncomingState_ChangeSlaveId_Failure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	MOCK_SLAVE;
	SetupOutOfRangeRegisterArray();
	mSlave.displayedStateInvalid = false;

	mSlave._state = sIdle;
	mSlave._modbus->setSlaveId(14);

	registerArray[0] = sReceivedRequest;
	registerArray[1] = 1;
	registerArray[2] = 41;

	bool processed;
	bool success = mSlave.processIncomingState(processed);

	ASSERT_FALSE(processed);
	ASSERT_FALSE(success);
	ASSERT_EQ(mSlave._state, sIdle);
	ASSERT_EQ(mSlave._modbus->getSlaveId(), 14);
	ASSERT_EQ(mSlave.displayedStateInvalid, false);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_processIncomingState_SetClock,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	MOCK_SLAVE;
	Spy(Method(mock, setClock));
	Mock<Device> mDevice0;
	Mock<Device> mDevice1;
	Device **deviceArray = new Device*[2];
	deviceArray[0] = &mDevice0.get();
	deviceArray[1] = &mDevice1.get();

	Fake(Method(mDevice0, setClock));
	Fake(Method(mDevice1, setClock));

	mSlave.displayedStateInvalid = false;

	mSlave._state = sIdle;
	mSlave._deviceCount = 2;
	mSlave._deviceNameLength = 703;
	mSlave._modbus->setSlaveId(14);
	mSlave._devices = deviceArray;

	registerArray[0] = sReceivedRequest;
	registerArray[1] = 32770;
	*(uint32_t*)(registerArray + 2) = 4000000000;

	bool processed;
	bool success = mSlave.processIncomingState(processed);

	ASSERT_TRUE(processed);
	ASSERT_TRUE(success);
	ASSERT_EQ(mSlave._state, sIdle);
	Verify(Method(mock, setClock).Using(4000000000)).Once();
	Verify(Method(mDevice0, setClock).Using(4000000000)).Once();
	Verify(Method(mDevice1, setClock).Using(4000000000)).Once();
	ASSERT_EQ(mSlave.displayedStateInvalid, true);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_processIncomingState_PrepareWriteData,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	MOCK_SLAVE;
	MockNewMethod(prepareReceiveData, word nameLength, uint32_t startTime,
		byte dataPointSize, TimeScale dataTimeScale, word dataPointsCount);
	Mock<Device> mDevice0;
	Mock<Device> mDevice1;
	Device **deviceArray = new Device*[2];
	deviceArray[0] = &mDevice0.get();
	deviceArray[1] = &mDevice1.get();
	string actualName;

	When(Method(mDevice1, prepareReceiveData)).Do([&actualName, &prepareReceiveData](word nameLength, byte* name, uint32_t startTime,
		byte dataPointSize, TimeScale dataTimeScale, word dataPointsCount, byte &outDataPointsPerPage)
	{
		prepareReceiveData.get().method(nameLength, startTime, dataPointSize, dataTimeScale, dataPointsCount);
		actualName = stringifyCharArray(nameLength, (char*)name);
		outDataPointsPerPage = 3;
		return RecieveDataStatus::success;
	});

	mSlave.displayedStateInvalid = false;

	mSlave._state = sIdle;
	mSlave._deviceCount = 2;
	mSlave._deviceNameLength = 703;
	mSlave._modbus->setSlaveId(14);
	mSlave._devices = deviceArray;
	mSlave._dataBuffer = new byte[15];
	mSlave._dataBufferSize = 15;

	registerArray[0] = sReceivedRequest;
	registerArray[1] = 4;
	registerArray[2] = 1;
	registerArray[3] = 7;
	registerArray[4] = 50;
	registerArray[5] = 4;
	registerArray[6] = 7 + ((word)TimeScale::sec15 << 8);
	registerArray[7] = 15;
	registerArray[8] = (word)'D' + ((word)'e' << 8);
	registerArray[9] = (word)'v' + ((word)'i' << 8);
	registerArray[10] = (word)'c' + ((word)'e' << 8);
	registerArray[11] = (word)'0';

	bool processed;
	bool success = mSlave.processIncomingState(processed);

	ASSERT_TRUE(processed);
	ASSERT_TRUE(success);
	Verify(Method(prepareReceiveData, method).Using(7, 262194, 7, TimeScale::sec15, 15)).Once();
	ASSERT_EQ(actualName, "Device0");
	ASSERT_EQ(mSlave.displayedStateInvalid, true);
	ASSERT_EQ(mSlave._state, sPreparingToReceiveDevData);
	ASSERT_EQ(mSlave._stateDetail, 768);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_processIncomingState_PrepareWriteData_nameTooLong,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	MOCK_SLAVE;
	MockNewMethod(prepareReceiveData, word nameLength, uint32_t startTime,
		byte dataPointSize, TimeScale dataTimeScale, word dataPointsCount);
	Mock<Device> mDevice0;
	Mock<Device> mDevice1;
	Device **deviceArray = new Device*[2];
	deviceArray[0] = &mDevice0.get();
	deviceArray[1] = &mDevice1.get();

	When(Method(mDevice1, prepareReceiveData)).Do([&prepareReceiveData](word nameLength, byte* name, uint32_t startTime,
		byte dataPointSize, TimeScale dataTimeScale, word dataPointsCount, byte &outDataPointsPerPage)
	{
		prepareReceiveData.get().method(nameLength, startTime, dataPointSize, dataTimeScale, dataPointsCount);
		outDataPointsPerPage = 3;
		return RecieveDataStatus::success;
	});

	mSlave.displayedStateInvalid = false;

	mSlave._state = sIdle;
	mSlave._deviceCount = 2;
	mSlave._deviceNameLength = 703;
	mSlave._modbus->setSlaveId(14);
	mSlave._devices = deviceArray;
	mSlave._dataBuffer = new byte[15];
	mSlave._dataBufferSize = 15;

	registerArray[0] = sReceivedRequest;
	registerArray[1] = 4;
	registerArray[2] = 1;
	registerArray[3] = 500;
	registerArray[4] = 50;
	registerArray[5] = 4;
	registerArray[6] = 7 + ((word)TimeScale::sec15 << 8);
	registerArray[7] = 15;
	registerArray[8] = (word)'D' + ((word)'e' << 8);
	registerArray[9] = (word)'v' + ((word)'i' << 8);
	registerArray[10] = (word)'c' + ((word)'e' << 8);
	registerArray[11] = (word)'0';

	bool processed;
	bool success = mSlave.processIncomingState(processed);

	ASSERT_TRUE(processed);
	ASSERT_TRUE(success);
	Verify(Method(prepareReceiveData, method)).Never();
	ASSERT_EQ(mSlave.displayedStateInvalid, true);
	ASSERT_EQ(mSlave._state, sPreparingToReceiveDevData);
	ASSERT_EQ(mSlave._stateDetail, 2);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_processIncomingState_PrepareWriteData_otherError,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	MOCK_SLAVE;
	MockNewMethod(prepareReceiveData, word nameLength, uint32_t startTime,
		byte dataPointSize, TimeScale dataTimeScale, word dataPointsCount);
	Mock<Device> mDevice0;
	Mock<Device> mDevice1;
	Device **deviceArray = new Device*[2];
	deviceArray[0] = &mDevice0.get();
	deviceArray[1] = &mDevice1.get();

	When(Method(mDevice1, prepareReceiveData)).Do([&prepareReceiveData](word nameLength, byte* name, uint32_t startTime,
		byte dataPointSize, TimeScale dataTimeScale, word dataPointsCount, byte &outDataPointsPerPage)
	{
		prepareReceiveData.get().method(nameLength, startTime, dataPointSize, dataTimeScale, dataPointsCount);
		return RecieveDataStatus::failure;
	});

	mSlave.displayedStateInvalid = false;

	mSlave._state = sIdle;
	mSlave._deviceCount = 2;
	mSlave._deviceNameLength = 703;
	mSlave._modbus->setSlaveId(14);
	mSlave._devices = deviceArray;
	mSlave._dataBuffer = new byte[15];
	mSlave._dataBufferSize = 15;

	registerArray[0] = sReceivedRequest;
	registerArray[1] = 4;
	registerArray[2] = 1;
	registerArray[3] = 7;
	registerArray[4] = 50;
	registerArray[5] = 4;
	registerArray[6] = 7 + ((word)TimeScale::sec15 << 8);
	registerArray[7] = 15;
	registerArray[8] = (word)'D' + ((word)'e' << 8);
	registerArray[9] = (word)'v' + ((word)'i' << 8);
	registerArray[10] = (word)'c' + ((word)'e' << 8);
	registerArray[11] = (word)'0';

	bool processed;
	bool success = mSlave.processIncomingState(processed);

	ASSERT_TRUE(processed);
	ASSERT_TRUE(success);
	Verify(Method(prepareReceiveData, method).Using(7, 262194, 7, TimeScale::sec15, 15)).Once();
	ASSERT_EQ(mSlave.displayedStateInvalid, true);
	ASSERT_EQ(mSlave._state, sPreparingToReceiveDevData);
	ASSERT_EQ(mSlave._stateDetail, 4);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_setOutgoingState_PrepareWriteData,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	slave->displayedStateInvalid = true;
	slave->_state = sPreparingToReceiveDevData;
	slave->_stateDetail = 703;
	bool success = slave->setOutgoingState();

	ASSERT_TRUE(success);
	ASSERT_EQ(slave->displayedStateInvalid, false);
	ASSERT_EQ(registerArray[0], sPreparingToReceiveDevData);
	ASSERT_EQ(registerArray[1], 703);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_processIncomingState_WriteData,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	MOCK_SLAVE;
	MockNewMethod(mockReceiveData, byte dataPointsInPage, byte dataPointSize,
		TimeScale timesScale, byte pageNumber);
	Mock<Device> mDevice0;
	Mock<Device> mDevice1;
	Device **deviceArray = new Device*[2];
	deviceArray[0] = &mDevice0.get();
	deviceArray[1] = &mDevice1.get();
	byte actualData[4];
	actualData[3] = 0;

	When(Method(mDevice1, receiveDeviceData)).Do([&actualData, &mockReceiveData](byte dataPointsInPage, byte dataPointSize,
		TimeScale timeScale, byte pageNumber, byte* dataPoints)
	{
		mockReceiveData.get().method(dataPointsInPage, dataPointSize, timeScale, pageNumber);
		BitFunctions::copyBits(dataPoints, actualData, 0, 0, 30);
		return RecieveDataStatus::success;
	});

	mSlave.displayedStateInvalid = false;

	mSlave._state = sIdle;
	mSlave._deviceCount = 2;
	mSlave._deviceNameLength = 703;
	mSlave._modbus->setSlaveId(14);
	mSlave._devices = deviceArray;
	mSlave._dataBuffer = new byte[15];
	mSlave._dataBufferSize = 15;

	registerArray[0] = sReceivedRequest;
	registerArray[1] = 5;
	registerArray[2] = 1;
	registerArray[3] = 6 + (word)(5 << 8);
	registerArray[4] = (word)TimeScale::sec15 + (word)(2 << 8);
	registerArray[5] = 0x25FE;
	registerArray[6] = 0xE68C;

	bool processed;
	bool success = mSlave.processIncomingState(processed);

	ASSERT_TRUE(processed);
	ASSERT_TRUE(success);
	ASSERT_EQ(mSlave._state, sIdle);
	Verify(Method(mockReceiveData, method).Using(6, 5, TimeScale::sec15, 2)).Once();
	assertArrayEq<byte, byte, byte, byte>(actualData,
		0xFE,
		0x25,
		0x8C,
		0x26);
	ASSERT_EQ(mSlave.displayedStateInvalid, true);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_processIncomingState_WriteData_DataTooLong,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	MOCK_SLAVE;
	MockNewMethod(mockReceiveData, byte dataPointsInPage, byte dataPointSize,
		TimeScale timesScale, byte pageNumber);
	Mock<Device> mDevice0;
	Mock<Device> mDevice1;
	Device **deviceArray = new Device*[2];
	deviceArray[0] = &mDevice0.get();
	deviceArray[1] = &mDevice1.get();
	byte actualData[4];
	actualData[3] = 0;

	When(Method(mDevice1, receiveDeviceData)).Do([&actualData, &mockReceiveData](byte dataPointsInPage, byte dataPointSize,
		TimeScale timeScale, byte pageNumber, byte* dataPoints)
	{
		mockReceiveData.get().method(dataPointsInPage, dataPointSize, timeScale, pageNumber);
		BitFunctions::copyBits(dataPoints, actualData, 0, 0, 30);
		return RecieveDataStatus::success;
	});

	mSlave.displayedStateInvalid = false;

	mSlave._state = sIdle;
	mSlave._deviceCount = 2;
	mSlave._deviceNameLength = 703;
	mSlave._modbus->setSlaveId(14);
	mSlave._devices = deviceArray;
	mSlave._dataBuffer = new byte[15];
	mSlave._dataBufferSize = 15;

	registerArray[0] = sReceivedRequest;
	registerArray[1] = 5;
	registerArray[2] = 1;
	registerArray[3] = 99 + (word)(5 << 8);
	registerArray[4] = (word)TimeScale::sec15 + (word)(2 << 8);
	registerArray[5] = 0x25FE;
	registerArray[6] = 0xE68C;

	bool processed;
	bool success = mSlave.processIncomingState(processed);

	ASSERT_TRUE(processed);
	ASSERT_TRUE(success);
	ASSERT_EQ(mSlave._state, sIdle);
	Verify(Method(mockReceiveData, method)).Never();
	ASSERT_EQ(mSlave.displayedStateInvalid, false);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_init,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	slave->_state = sIdle;
	slave->_modbus->setSlaveId(17);

	Device **devices = tracker.addArray(new Device*[3]);
	SetupDevices(devices, 3);
	byte **names = tracker.addArray(new byte*[3]);
	names[0] = (byte*)"dev01";
	names[1] = (byte*)"dev02";
	names[2] = (byte*)"dev03";

	slave->init(3, 5, 20, 30, devices, names);

	ASSERT_EQ(slave->getDeviceCount(), 3);
	ASSERT_EQ(slave->getDeviceNameLength(), 5);
	ASSERT_EQ(slave->getHregCount(), 20);
	assertArrayEq(slave->_devices,
		devices[0],
		devices[1],
		devices[2]);

	for (int i = 0; i < 3; i++)
	{
		assertArrayEq(slave->_deviceNames[i],
			'd', 'e', 'v', '0', (byte)('0' + i + 1));
	}
	ASSERT_TRUE(slave->_dataBuffer != nullptr);
	ASSERT_EQ(slave->_dataBufferSize, 30);
}

TEST_F_TRAITS(SlaveTests, SlaveTests_clearDevices,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	slave->_state = sIdle;
	slave->_modbus->setSlaveId(17);

	Device **devices = tracker.addArray(new Device*[3]);
	SetupDevices(devices, 3);
	byte **names = tracker.addArray(new byte*[3]);
	names[0] = (byte*)"dev01";
	names[1] = (byte*)"dev02";
	names[2] = (byte*)"dev03";

	slave->init(3, 5, 20, 1, devices, names);
	slave->clearDevices();

	ASSERT_EQ(slave->getDeviceCount(), 0);
	ASSERT_EQ(slave->getDeviceNameLength(), 5); // Clearing doesn't change name length
	ASSERT_EQ(slave->_devices, nullptr);
	ASSERT_EQ(slave->_deviceNames, nullptr);
}