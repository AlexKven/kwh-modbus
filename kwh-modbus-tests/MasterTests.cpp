#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/modbus/ModbusArray.h"
#include "../kwh-modbus/libraries/master/Master.hpp"
#include "../kwh-modbus/libraries/resilientModbusMaster/ResilientModbusMaster.hpp"
#include "../kwh-modbus/mock/MockSerialStream.h"
#include "../kwh-modbus/mock/MockableResilientModbusMaster.hpp"
#include "../kwh-modbus/libraries/deviceDirectory/DeviceDirectory.hpp"
#include "WindowsSystemFunctions.h"
#include "test_helpers.h"

using namespace fakeit;

typedef MockableResilientModbusMaster<MockSerialStream, WindowsSystemFunctions, ModbusArray> T_MODBUS;
typedef Master<T_MODBUS, WindowsSystemFunctions, DeviceDirectory<byte*>> T_MASTER;

#define MOCK_MASTER Mock<T_MASTER> masterMock(*master); \
T_MASTER & mMaster = masterMock.get()
#define MOCK_MODBUS Mock<T_MODBUS> modbusMock(*modbus); \
T_MODBUS & mModbus = modbusMock.get()

class MasterTests : public ::testing::Test
{
protected:
	word *registerArray;
	MockSerialStream *serial = new MockSerialStream();
	WindowsSystemFunctions *system = new WindowsSystemFunctions();
	T_MODBUS *modbus = new T_MODBUS();
	T_MASTER *master = new T_MASTER();
	Mock<DeviceDirectory<byte*>> mockDeviceDirectory;

	void SetupOutOfRangeRegisterArray()
	{
		delete modbus;
		modbus = new T_MODBUS();
		modbus->init(registerArray, 20, 12, 20);
		master->config(system, modbus, &mockDeviceDirectory.get());
	}
public:
	void SetUp()
	{
		registerArray = new word[12];
		modbus->init(registerArray, 0, 12, 20);
		master->config(system, modbus, &mockDeviceDirectory.get());
		modbus->config(serial, system, 1200);
	}

	void TearDown()
	{
		delete modbus;
		delete[] registerArray;
	}
};

TEST_F(MasterTests, ensureTaskNotStarted_NeedsReset_Doesnt)
{
	MOCK_MASTER;
	Fake(Method(masterMock, modbusReset));
	When(Method(masterMock, modbusGetStatus)).AlwaysReturn(TaskInProgress);

	T_MASTER::ensureTaskNotStarted_Task task(&T_MASTER::ensureTaskNotStarted, master);
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());

	Verify(Method(masterMock, modbusGetStatus)).Exactly(3);
	Verify(Method(masterMock, modbusReset)).Once();
}

TEST_F(MasterTests, ensureTaskNotStarted_NeedsReset_Does)
{
	MOCK_MASTER;
	Fake(Method(masterMock, modbusReset));
	When(Method(masterMock, modbusGetStatus))
		.Return(TaskInProgress)
		.Return(TaskComplete)
		.Return(TaskNotStarted);

	T_MASTER::ensureTaskNotStarted_Task task(&T_MASTER::ensureTaskNotStarted, master);
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());
	ASSERT_TRUE(task());

	Verify(Method(masterMock, modbusGetStatus)).Exactly(3);
	Verify(Method(masterMock, modbusReset)).Once();
}

TEST_F(MasterTests, ensureTaskNotStarted_DoesntNeedReset)
{
	MOCK_MASTER;
	Fake(Method(masterMock, modbusReset));
	When(Method(masterMock, modbusGetStatus)).AlwaysReturn(TaskNotStarted);

	T_MASTER::ensureTaskNotStarted_Task task(&T_MASTER::ensureTaskNotStarted, master);
	ASSERT_TRUE(task());

	Verify(Method(masterMock, modbusGetStatus)).Once();
	Verify(Method(masterMock, modbusReset)).Never();
}

TEST_F(MasterTests, completeModbusReadRegisters_CompletesImmediately)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(modbusMock, work)).AlwaysReturn(true);
	When(Method(modbusMock, setRequest_ReadRegisters)).AlwaysReturn(true);

	T_MASTER::completeModbusReadRegisters_Task task(&T_MASTER::completeModbusReadRegisters, master, 2, 3, 5);
	ASSERT_TRUE(task());

	Verify(Method(modbusMock, work)).Once();
	Verify(Method(modbusMock, setRequest_ReadRegisters).Using(2, 3, 5)).Once();
}
