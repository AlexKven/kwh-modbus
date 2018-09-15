#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/modbus/ModbusArray.h"
#include "../kwh-modbus/libraries/slave/Slave.hpp"
#include "../kwh-modbus/libraries/modbusSlave/ModbusSlave.hpp"
#include "../kwh-modbus/mock/MockSerialStream.h"
#include "WindowsSystemFunctions.h"
#include "test_helpers.h"

using namespace fakeit;

typedef ModbusSlave<MockSerialStream, WindowsSystemFunctions, ModbusArray> T_MODBUS;
typedef Slave<ModbusSlave<MockSerialStream, WindowsSystemFunctions, ModbusArray>, WindowsSystemFunctions> T_SLAVE;

class SlaveTests : public ::testing::Test
{
protected:
	word *registerArray;
	MockSerialStream *serial = new MockSerialStream();
	WindowsSystemFunctions *system = new WindowsSystemFunctions();
	T_MODBUS *modbus = new T_MODBUS();
	T_SLAVE *slave = new T_SLAVE();
	Mock<Device> *mockDevices = nullptr;

	void SetupOutOfRangeRegisterArray()
	{
		delete modbus;
		modbus = new T_MODBUS();
		modbus->init(registerArray, 20, 12, 20);
		slave->config(system, modbus);
	}

	void SetupDevices(Device **devices, word count)
	{
		if (mockDevices != nullptr)
			delete[] mockDevices;
		mockDevices = new Mock<Device>[count];
		for (word i = 0; i < count; i++)
		{
			When(Method(mockDevices[i], getType)).AlwaysReturn(7);
			devices[i] = &mockDevices[i].get();
		}
	}
public:
	void SetUp()
	{
		registerArray = new word[12];
		modbus->init(registerArray, 0, 12, 20);
		slave->config(system, modbus);
	}

	void TearDown()
	{
		delete modbus;
		delete[] registerArray;
		if (mockDevices != nullptr)
			delete[] mockDevices;
	}
};

TEST_F(SlaveTests, SlaveTests_setOutgoingState_Idle_Success)
{
	slave->_state = sIdle;
	slave->_deviceCount = 4;
	slave->_deviceNameLength = 703;
	bool success = slave->setOutgoingState();

	ASSERT_TRUE(success);
	ASSERT_EQ(registerArray[0], sIdle);
	ASSERT_EQ(registerArray[1], 1 << 8);
	ASSERT_EQ(registerArray[2], 4);
	ASSERT_EQ(registerArray[3], 703);
}

TEST_F(SlaveTests, SlaveTests_setOutgoingState_Idle_Failure)
{
	SetupOutOfRangeRegisterArray();
	slave->_state = sIdle;
	slave->_deviceCount = 4;
	slave->_deviceNameLength = 703;
	bool success = slave->setOutgoingState();

	ASSERT_FALSE(success);
}

TEST_F(SlaveTests, SlaveTests_setOutgoingState_DisplayDevInfo_Success)
{
	slave->_state = sDisplayDevInfo;

	Device **devices = new Device*[3];
	SetupDevices(devices, 3);
	byte **names = new byte*[3];
	names[0] = (byte*)"dev01";
	names[1] = (byte*)"dev02";
	names[2] = (byte*)"dev03";

	slave->init(3, 5, devices, names);

	// Select device #1
	registerArray[2] = 1;

	delete[] devices;
	delete[] names;

	bool success = slave->setOutgoingState();

	ASSERT_TRUE(success);
	assertArrayEq(registerArray,
		sDisplayDevInfo,
		(word)1,
		(word)7,
		(byte)'e',
		(byte)'d',
		(byte)'0',
		(byte)'v',
		(byte)0,
		(byte)'2',
		(word)0,
		(word)0);
}

TEST_F(SlaveTests, SlaveTests_processIncomingState_Idle)
{
	slave->_state = sIdle;
	slave->_deviceCount = 4;
	slave->_deviceNameLength = 703;
	slave->_modbus->setSlaveId(14);

	registerArray[0] = sReceivedRequest;
	registerArray[1] = 0;

	bool success = slave->processIncomingState();

	ASSERT_TRUE(success);
	ASSERT_EQ(registerArray[0], sIdle);
	ASSERT_EQ(registerArray[1], 1 << 8);
	ASSERT_EQ(registerArray[2], 4);
	ASSERT_EQ(registerArray[3], 703);
}

TEST_F(SlaveTests, SlaveTests_processIncomingState_ChangeSlaveId)
{
	slave->_state = sIdle;
	slave->_deviceCount = 4;
	slave->_deviceNameLength = 703;
	slave->_modbus->setSlaveId(14);

	registerArray[0] = sReceivedRequest;
	registerArray[1] = 1;
	registerArray[2] = 41;

	bool success = slave->processIncomingState();

	ASSERT_TRUE(success);
	ASSERT_EQ(slave->_state, sIdle);
	ASSERT_EQ(slave->getSlaveId(), 41);
}

TEST_F(SlaveTests, SlaveTests_processIncomingState_Nothing)
{
	slave->_state = sIdle;
	slave->_deviceCount = 4;
	slave->_deviceNameLength = 703;
	slave->_modbus->setSlaveId(14);

	registerArray[0] = 0;

	bool success = slave->processIncomingState();

	ASSERT_FALSE(success);
}

TEST_F(SlaveTests, SlaveTests_processIncomingState_ChangeSlaveId_Failure)
{
	SetupOutOfRangeRegisterArray();

	slave->_state = sIdle;
	slave->_deviceCount = 4;
	slave->_deviceNameLength = 703;
	slave->_modbus->setSlaveId(14);

	registerArray[0] = sReceivedRequest;
	registerArray[1] = 1;
	registerArray[2] = 41;

	bool success = slave->processIncomingState();

	ASSERT_FALSE(success);
	ASSERT_EQ(slave->_state, sIdle);
	ASSERT_EQ(slave->_modbus->getSlaveId(), 14);
}

TEST_F(SlaveTests, SlaveTests_init)
{
	slave->_state = sIdle;
	slave->_modbus->setSlaveId(17);

	Device **devices = new Device*[3];
	SetupDevices(devices, 3);
	byte **names = new byte*[3];
	names[0] = (byte*)"dev01";
	names[1] = (byte*)"dev02";
	names[2] = (byte*)"dev03";

	slave->init(3, 5, devices, names);

	delete[] names;

	ASSERT_EQ(slave->getDeviceCount(), 3);
	ASSERT_EQ(slave->getDeviceNameLength(), 5);
	assertArrayEq(slave->_devices,
		devices[0],
		devices[1],
		devices[2]);
	delete[] devices;
	for (int i = 0; i < 3; i++)
	{
		assertArrayEq(slave->_deviceNames[i],
			'd', 'e', 'v', '0', (byte)('0' + i + 1));
	}
}

TEST_F(SlaveTests, SlaveTests_clearDevices)
{
	slave->_state = sIdle;
	slave->_modbus->setSlaveId(17);

	Device **devices = new Device*[3];
	SetupDevices(devices, 3);
	byte **names = new byte*[3];
	names[0] = (byte*)"dev01";
	names[1] = (byte*)"dev02";
	names[2] = (byte*)"dev03";

	slave->init(3, 5, devices, names);
	slave->clearDevices();

	delete[] devices;
	delete[] names;

	ASSERT_EQ(slave->getDeviceCount(), 0);
	ASSERT_EQ(slave->getDeviceNameLength(), 5); // Clearing doesn't change name length
	ASSERT_EQ(slave->_devices, nullptr);
	ASSERT_EQ(slave->_deviceNames, nullptr);
}