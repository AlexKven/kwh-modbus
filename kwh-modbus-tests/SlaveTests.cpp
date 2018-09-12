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

	void SetupOutOfRangeRegisterArray()
	{
		delete modbus;
		modbus = new T_MODBUS();
		modbus->init(registerArray, 20, 12, 20);
		slave->config(system, modbus);
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
	ASSERT_EQ(slave->_modbus->getSlaveId(), 41);
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