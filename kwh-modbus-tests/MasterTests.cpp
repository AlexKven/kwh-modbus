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

#include <stack>
#include <tuple>

using namespace fakeit;

typedef MockableResilientModbusMaster<MockSerialStream, WindowsSystemFunctions, ModbusArray> T_MODBUS;
typedef Master<T_MODBUS, WindowsSystemFunctions, DeviceDirectory<byte*>> T_MASTER;
typedef ModbusMaster<MockSerialStream, WindowsSystemFunctions, ModbusArray> T_MODBUS_BASE;
typedef ResilientTask<WindowsSystemFunctions> T_MODBUS_TASK;

#define MOCK_MASTER Mock<T_MASTER> masterMock(*master); \
T_MASTER & mMaster = masterMock.get()
#define MOCK_MODBUS Mock<T_MODBUS_BASE> modbusBaseMock(*(T_MODBUS_BASE*)modbus); \
T_MODBUS_BASE & mModbusBase = modbusBaseMock.get(); \
Mock<T_MODBUS_TASK> modbusTaskMock(*(T_MODBUS_TASK*)modbus); \
T_MODBUS_TASK & mModbusTask = modbusTaskMock.get()

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
	MOCK_MODBUS;
	Fake(Method(modbusTaskMock, reset));
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskInProgress);

	T_MASTER::ensureTaskNotStarted_Task task(&T_MASTER::ensureTaskNotStarted, master);
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());

	Verify(Method(modbusTaskMock, getStatus)).Exactly(3);
	Verify(Method(modbusTaskMock, reset)).Once();
}

TEST_F(MasterTests, ensureTaskNotStarted_NeedsReset_Does)
{
	MOCK_MODBUS;
	Fake(Method(modbusTaskMock, reset));
	When(Method(modbusTaskMock, getStatus))
		.Return(TaskInProgress)
		.Return(TaskComplete)
		.Return(TaskNotStarted);

	T_MASTER::ensureTaskNotStarted_Task task(&T_MASTER::ensureTaskNotStarted, master);
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());
	ASSERT_TRUE(task());

	Verify(Method(modbusTaskMock, getStatus)).Exactly(3);
	Verify(Method(modbusTaskMock, reset)).Once();
}

TEST_F(MasterTests, ensureTaskNotStarted_DoesntNeedReset)
{
	MOCK_MODBUS;
	Fake(Method(modbusTaskMock, reset));
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskNotStarted);

	T_MASTER::ensureTaskNotStarted_Task task(&T_MASTER::ensureTaskNotStarted, master);
	ASSERT_TRUE(task());

	Verify(Method(modbusTaskMock, getStatus)).Once();
	Verify(Method(modbusTaskMock, reset)).Never();
}

TEST_F(MasterTests, completeModbusReadRegisters_CompletesImmediately)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_ReadRegisters)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isReadRegsResponse)).AlwaysDo([](word &regCount, word *regs) {
		regCount = 5;
		return true;
	});
	T_MASTER::completeModbusReadRegisters_Task task(&T_MASTER::completeModbusReadRegisters, master, 2, 3, 5);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), success);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_ReadRegisters).Using(2, 3, 5)).Once();
	Verify(Method(modbusBaseMock, isReadRegsResponse)).Once();
}

TEST_F(MasterTests, completeModbusReadRegisters_CompletesWithSomeAttempts)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool()))
		.Return(false)
		.Return(false)
		.Return(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_ReadRegisters)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isReadRegsResponse)).AlwaysDo([](word &regCount, word *regs) {
		regCount = 5;
		return true;
	});
	T_MASTER::completeModbusReadRegisters_Task task(&T_MASTER::completeModbusReadRegisters, master, 2, 3, 5);
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), success);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Exactly(3);
	Verify(Method(modbusBaseMock, setRequest_ReadRegisters).Using(2, 3, 5)).Once();
	Verify(Method(modbusBaseMock, isReadRegsResponse)).Once();
}

TEST_F(MasterTests, completeModbusReadRegisters_MasterFailure)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(Method(modbusBaseMock, setRequest_ReadRegisters)).AlwaysReturn(false);

	T_MASTER::completeModbusReadRegisters_Task task(&T_MASTER::completeModbusReadRegisters, master, 2, 3, 5);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), masterFailure);
	Verify(Method(modbusBaseMock, setRequest_ReadRegisters).Using(2, 3, 5)).Once();
}

TEST_F(MasterTests, completeModbusReadRegisters_TaskFailure)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskFatal);
	When(Method(modbusBaseMock, setRequest_ReadRegisters)).AlwaysReturn(true);

	T_MASTER::completeModbusReadRegisters_Task task(&T_MASTER::completeModbusReadRegisters, master, 2, 3, 5);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), TaskFailure);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_ReadRegisters).Using(2, 3, 5)).Once();
}

TEST_F(MasterTests, completeModbusReadRegisters_IncorrectResponseSize)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_ReadRegisters)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isReadRegsResponse)).AlwaysDo([](word &regCount, word *regs) {
		regCount = 7;
		return true;
	});
	T_MASTER::completeModbusReadRegisters_Task task(&T_MASTER::completeModbusReadRegisters, master, 2, 3, 5);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), incorrectResponseSize);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_ReadRegisters).Using(2, 3, 5)).Once();
	Verify(Method(modbusBaseMock, isReadRegsResponse)).Once();
}

TEST_F(MasterTests, completeModbusReadRegisters_ExceptionResponse)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_ReadRegisters)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isReadRegsResponse)).Return(false);
	When(Method(modbusBaseMock, isExceptionResponse)).Return(true);
	T_MASTER::completeModbusReadRegisters_Task task(&T_MASTER::completeModbusReadRegisters, master, 2, 3, 5);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), exceptionResponse);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_ReadRegisters).Using(2, 3, 5)).Once();
	Verify(Method(modbusBaseMock, isReadRegsResponse)).Once();
}

TEST_F(MasterTests, completeModbusReadRegisters_OtherResponse)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_ReadRegisters)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isReadRegsResponse)).Return(false);
	When(Method(modbusBaseMock, isExceptionResponse)).Return(false);
	T_MASTER::completeModbusReadRegisters_Task task(&T_MASTER::completeModbusReadRegisters, master, 2, 3, 5);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), otherResponse);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_ReadRegisters).Using(2, 3, 5)).Once();
	Verify(Method(modbusBaseMock, isReadRegsResponse)).Once();
}

TEST_F(MasterTests, completeModbusWriteRegisters_single_CompletesImmediately)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_WriteRegister)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isWriteRegResponse)).Return(true);

	word val = 6;
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 1, &val);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), success);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_WriteRegister).Using(2, 3, 6)).Once();
	Verify(Method(modbusBaseMock, isWriteRegResponse)).Once();
}

TEST_F(MasterTests, completeModbusWriteRegisters_single_CompletesWithSomeAttempts)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool()))
		.Return(false)
		.Return(false)
		.Return(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_WriteRegister)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isWriteRegResponse)).Return(true);

	word val = 6;
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 1, &val);
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), success);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Exactly(3);
	Verify(Method(modbusBaseMock, setRequest_WriteRegister).Using(2, 3, 6)).Once();
	Verify(Method(modbusBaseMock, isWriteRegResponse)).Once();
}

TEST_F(MasterTests, completeModbusWriteRegisters_single_MasterFailure)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(Method(modbusBaseMock, setRequest_WriteRegister)).AlwaysReturn(false);

	word val = 6;
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 1, &val);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), masterFailure);
	Verify(Method(modbusBaseMock, setRequest_WriteRegister).Using(2, 3, 6)).Once();
}

TEST_F(MasterTests, completeModbusWriteRegisters_single_TaskFailure)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskFatal);
	When(Method(modbusBaseMock, setRequest_WriteRegister)).AlwaysReturn(true);

	word val = 6;
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 1, &val);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), TaskFailure);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_WriteRegister).Using(2, 3, 6)).Once();
}

TEST_F(MasterTests, completeModbusWriteRegisters_single_ExceptionResponse)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_WriteRegister)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isWriteRegResponse)).Return(false);
	When(Method(modbusBaseMock, isExceptionResponse)).Return(true);

	word val = 6;
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 1, &val);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), exceptionResponse);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_WriteRegister).Using(2, 3, 6)).Once();
	Verify(Method(modbusBaseMock, isWriteRegResponse)).Once();
	Verify(Method(modbusBaseMock, isExceptionResponse)).Once();
}

TEST_F(MasterTests, completeModbusWriteRegisters_single_OtherResponse)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_WriteRegister)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isWriteRegResponse)).Return(false);
	When(Method(modbusBaseMock, isExceptionResponse)).Return(false);

	word val = 6;
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 1, &val);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), otherResponse);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_WriteRegister).Using(2, 3, 6)).Once();
	Verify(Method(modbusBaseMock, isWriteRegResponse)).Once();
	Verify(Method(modbusBaseMock, isExceptionResponse)).Once();
}

TEST_F(MasterTests, completeModbusWriteRegisters_multiple_CompletesImmediately)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_WriteRegisters)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isWriteRegsResponse)).Return(true);

	word vals[3] = { 1, 2, 3 };
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 3, (word*)vals);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), success);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_WriteRegisters).Using(2, 3, 3, (word*)vals)).Once();
	Verify(Method(modbusBaseMock, isWriteRegsResponse)).Once();
}

TEST_F(MasterTests, completeModbusWriteRegisters_multiple_CompletesWithSomeAttempts)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool()))
		.Return(false)
		.Return(false)
		.Return(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_WriteRegisters)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isWriteRegsResponse)).Return(true);

	word vals[3] = { 1, 2, 3 };
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 3, (word*)vals);
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), success);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Exactly(3);
	Verify(Method(modbusBaseMock, setRequest_WriteRegisters).Using(2, 3, 3, (word*)vals)).Once();
	Verify(Method(modbusBaseMock, isWriteRegsResponse)).Once();
}

TEST_F(MasterTests, completeModbusWriteRegisters_multiple_MasterFailure)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(Method(modbusBaseMock, setRequest_WriteRegisters)).AlwaysReturn(false);

	word vals[3] = { 1, 2, 3 };
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 3, (word*)vals);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), masterFailure);
	Verify(Method(modbusBaseMock, setRequest_WriteRegisters).Using(2, 3, 3, (word*)vals)).Once();
}

TEST_F(MasterTests, completeModbusWriteRegisters_multiple_TaskFailure)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskFatal);
	When(Method(modbusBaseMock, setRequest_WriteRegisters)).AlwaysReturn(true);

	word vals[3] = { 1, 2, 3 };
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 3, (word*)vals);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), TaskFailure);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_WriteRegisters).Using(2, 3, 3, (word*)vals)).Once();
}

TEST_F(MasterTests, completeModbusWriteRegisters_multiple_ExceptionResponse)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_WriteRegisters)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isWriteRegsResponse)).Return(false);
	When(Method(modbusBaseMock, isExceptionResponse)).Return(true);

	word vals[3] = { 1, 2, 3 };
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 3, (word*)vals);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), exceptionResponse);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_WriteRegisters).Using(2, 3, 3, (word*)vals)).Once();
	Verify(Method(modbusBaseMock, isWriteRegsResponse)).Once();
	Verify(Method(modbusBaseMock, isExceptionResponse)).Once();
}

TEST_F(MasterTests, completeModbusWriteRegisters_multiple_OtherResponse)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(Method(masterMock, ensureTaskNotStarted)).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_WriteRegisters)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isWriteRegsResponse)).Return(false);
	When(Method(modbusBaseMock, isExceptionResponse)).Return(false);

	word vals[3] = { 1, 2, 3 };
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 3, (word*)vals);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), otherResponse);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_WriteRegisters).Using(2, 3, 3, (word*)vals)).Once();
	Verify(Method(modbusBaseMock, isWriteRegsResponse)).Once();
	Verify(Method(modbusBaseMock, isExceptionResponse)).Once();
}

TEST_F(MasterTests, checkForNewSlaves_Found)
{
	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).Return(true);
	When(Method(completeReadRegsMock, result)).Return(success);

	T_MASTER::checkForNewSlaves_Task task(&T_MASTER::checkForNewSlaves, master);
	ASSERT_TRUE(task());
	Verify(Method(completeReadRegsMock, func).Using(1, 0, 7)).Once();
	ASSERT_EQ(task.result(), found);
}

TEST_F(MasterTests, checkForNewSlaves_NotFound)
{
	MOCK_MODBUS;
	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).Return(true);
	When(Method(completeReadRegsMock, result)).Return(taskFailure);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskTimeOut);

	T_MASTER::checkForNewSlaves_Task task(&T_MASTER::checkForNewSlaves, master);
	ASSERT_TRUE(task());
	Verify(Method(completeReadRegsMock, func).Using(1, 0, 7)).Once();
	ASSERT_EQ(task.result(), notFound);
}

TEST_F(MasterTests, checkForNewSlaves_Error_TaskFailure)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).Return(true);
	When(Method(completeReadRegsMock, result)).Return(taskFailure);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskFatal);
	Fake(Method(masterMock, reportMalfunction));

	T_MASTER::checkForNewSlaves_Task task(&T_MASTER::checkForNewSlaves, master);
	ASSERT_TRUE(task());
	Verify(Method(completeReadRegsMock, func).Using(1, 0, 7)).Once();
	ASSERT_EQ(task.result(), error);
	Verify(Method(masterMock, reportMalfunction)).Once();
}

TEST_F(MasterTests, checkForNewSlaves_Error_MasterFailure)
{
	MOCK_MASTER;
	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).Return(true);
	When(Method(completeReadRegsMock, result)).Return(masterFailure);
	Fake(Method(masterMock, reportMalfunction));

	T_MASTER::checkForNewSlaves_Task task(&T_MASTER::checkForNewSlaves, master);
	ASSERT_TRUE(task());
	Verify(Method(completeReadRegsMock, func).Using(1, 0, 7)).Once();
	ASSERT_EQ(task.result(), error);
	Verify(Method(masterMock, reportMalfunction)).Once();
}

TEST_F(MasterTests, checkForNewSlaves_BadSlave)
{
	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).Return(true);
	When(Method(completeReadRegsMock, result)).Return(exceptionResponse);

	T_MASTER::checkForNewSlaves_Task task(&T_MASTER::checkForNewSlaves, master);
	ASSERT_TRUE(task());
	Verify(Method(completeReadRegsMock, func).Using(1, 0, 7)).Once();
	ASSERT_EQ(task.result(), badSlave);
}

TEST_F(MasterTests, processNewSlave_Success_ThreeDevices)
{
	MOCK_MODBUS;

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).Return(true);
	When(Method(completeReadRegsMock, result)).Return(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeWriteRegsMock, func)).Return(true);
	When(Method(completeWriteRegsMock, result)).Return(success);
	
	stack<tuple<word, word*>> regsStack;
	regsStack.push(make_tuple(7, new word[7]{ 0, 1 << 8, 3, 6, 0, 0, 0 }));
	When(Method(modbusBaseMock, isReadRegsResponse)).AlwaysDo([&regsStack](word &regCount, word *&regs) {
		auto next = regsStack.top();
		regsStack.pop();
		regCount = get<0>(next);
		regs = get<1>(next);
		return true;
	});

	T_MASTER::processNewSlave_Task task(&T_MASTER::processNewSlave, master, false);
	ASSERT_TRUE(task());
	Verify(Method(completeReadRegsMock, func).Using(1, 0, 7)).Once();
}