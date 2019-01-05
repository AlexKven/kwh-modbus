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
#include "PointerTracker.h"

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

#define REGS(COUNT, ...)make_tuple(COUNT, tracker.addArray(new word[COUNT] { __VA_ARGS__}))

class MasterTests : public ::testing::Test
{
protected:
	word *registerArray;
	MockSerialStream *serial = new MockSerialStream();
	WindowsSystemFunctions *system = new WindowsSystemFunctions();
	T_MODBUS *modbus = new T_MODBUS();
	T_MASTER *master = new T_MASTER();
	Mock<DeviceDirectory<byte*>> mockDeviceDirectory;

	PointerTracker tracker;

	typedef stack<tuple<word, word*>> RegsStack;

	void SetupOutOfRangeRegisterArray()
	{
		modbus->init(registerArray, 20, 12, 20);
		master->config(system, modbus, &mockDeviceDirectory.get(), 10);
	}

	word DataCollectorDeviceType(bool accumulateData, TimeScale timeScale, byte dataPacketSize)
	{
		word devType;
		DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(accumulateData, timeScale, dataPacketSize, devType);
		return devType;
	}

	word DataTransmitterDeviceType()
	{
		return 2 << 14;
	}

	vector<tuple<byte*, DeviceDirectoryRow>>* Setup_DataCollectorsAndTransmitters()
	{
		auto devices = new vector<tuple<byte*, DeviceDirectoryRow>>();
		tracker.addPointer(devices);
		devices->push_back(make_tuple((byte*)"device0", DeviceDirectoryRow(6, 1, DataCollectorDeviceType(false, TimeScale::ms250, 3), 10)));
		devices->push_back(make_tuple((byte*)"device1", DeviceDirectoryRow(6, 0, DataCollectorDeviceType(false, TimeScale::sec1, 3), 10)));
		devices->push_back(make_tuple((byte*)"device2", DeviceDirectoryRow(5, 1, DataTransmitterDeviceType(), 10)));
		devices->push_back(make_tuple((byte*)"device3", DeviceDirectoryRow(4, 1, DataCollectorDeviceType(true, TimeScale::hr1, 4), 10)));
		devices->push_back(make_tuple((byte*)"device4", DeviceDirectoryRow(5, 0, DataTransmitterDeviceType(), 10)));
		devices->push_back(make_tuple((byte*)"device5", DeviceDirectoryRow(4, 0, DataCollectorDeviceType(true, TimeScale::min1, 4), 10)));
		devices->push_back(make_tuple((byte*)"device6", DeviceDirectoryRow(4, 2, DataCollectorDeviceType(true, TimeScale::hr24, 4), 10)));
		devices->push_back(make_tuple((byte*)"device7", DeviceDirectoryRow(3, 1, DataCollectorDeviceType(false, TimeScale::sec15, 11), 10)));
		devices->push_back(make_tuple((byte*)"device8", DeviceDirectoryRow(3, 0, DataCollectorDeviceType(false, TimeScale::min30, 7), 10)));

		When(OverloadedMethod(mockDeviceDirectory, findNextDevice, DeviceDirectoryRow*(byte*&, int&)))
			.AlwaysDo([devices](byte* &devName, int &row)
		{
			if (row < devices->size())
			{
				row++;
				devName = get<0>((*devices)[row - 1]);
				return &get<1>((*devices)[row - 1]);
			}
			else
			{
				row = -1;
				return (DeviceDirectoryRow*)nullptr;
			}
		});

		return devices;
	}

	void isRegsResponse_UseMockData(Mock<T_MODBUS_BASE> &mock, RegsStack &regsStack)
	{
		When(Method(mock, isReadRegsResponse)).AlwaysDo([&regsStack](word &regCount, word *&regs) {
			auto next = regsStack.top();
			regsStack.pop();
			regCount = get<0>(next);
			regs = get<1>(next);
			return true;
		});
	}

	void completeWriteRegs_UseRegsStack_Passthrough(Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> &mock, RegsStack &stack, function<bool(byte, word, word, word*)> alwaysDo)
	{
		When(Method(mock, func)).AlwaysDo([&stack, this, alwaysDo](byte slaveId, word start, word count, word* data)
		{
			word *regs = tracker.addArray(new word[count]);
			BitFunctions::copyBits(data, regs, 0, 0, 16 * count);
			stack.push(make_tuple(count, regs));
			return alwaysDo(slaveId, start, count, data);
		});
	}

	void completeWriteRegs_UseRegsStack(Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> &mock, RegsStack &stack)
	{
		completeWriteRegs_UseRegsStack_Passthrough(mock, stack,
			[](byte slaveId, word start, word count, word* data) { return true; });
	}

	tuple<int, word*> withString(tuple<int, word*> regs, const char* str)
	{
		int strLen = 0;
		while (str[strLen] != '\0')
			strLen++;
		int count = get<0>(regs) + strLen / 2 + strLen % 2;
		word* result = tracker.addArray(new word[count]);
		result[count - 1] = 0;
		BitFunctions::copyBits(get<1>(regs), result, 0, 0, 16 * get<0>(regs));
		BitFunctions::copyBits(str, result, 0, 16 * get<0>(regs), strLen * 8);
		return make_tuple(count, result);
	}

	void assertPopRegsStack(RegsStack &regsStack, tuple<int, word*> regs)
	{
		auto top = regsStack.top();
		regsStack.pop();

		ASSERT_EQ(get<0>(top), get<0>(regs));
		for (int i = 0; i < get<0>(top); i++)
		{
			ASSERT_EQ(get<1>(top)[i], get<1>(regs)[i]);
		}
	}

	static DeviceDirectoryRow* getDevicePtr(vector<tuple<byte*, DeviceDirectoryRow>>* devices, int index)
	{
		return &get<1>(devices->at(index));
	}

	static byte* getDeviceNamePtr(vector<tuple<byte*, DeviceDirectoryRow>>* devices, int index)
	{
		return get<0>(devices->at(index));
	}
public:
	void SetUp()
	{
		auto sze = sizeof(T_MASTER);
		registerArray = new word[12];
		modbus->init(registerArray, 0, 12, 20);
		master->config(system, modbus, &mockDeviceDirectory.get(), 10);
		modbus->config(serial, system, 1200);
		tracker.addPointers(modbus, master, serial, system);
		tracker.addArrays(registerArray);
	}

	void TearDown()
	{
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
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

TEST_F(MasterTests, completeModbusWriteRegisters_broadcast_succeeds)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskComplete);
	When(Method(modbusBaseMock, setRequest_WriteRegisters)).AlwaysReturn(true);
	When(Method(modbusBaseMock, isWriteRegsResponse)).Return(false);
	When(Method(modbusBaseMock, isExceptionResponse)).Return(false);

	word vals[3] = { 1, 2, 3 };
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 0, 3, 3, (word*)vals);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), success);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_WriteRegisters).Using(0, 3, 3, (word*)vals)).Once();
	Verify(Method(modbusBaseMock, isWriteRegsResponse)).Never();
	Verify(Method(modbusBaseMock, isExceptionResponse)).Never();
}

TEST_F(MasterTests, checkForNewSlaves_Found)
{
	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).Return(true);
	When(Method(completeReadRegsMock, result)).Return(success);

	T_MASTER::checkForNewSlaves_Task task(&T_MASTER::checkForNewSlaves, master);
	ASSERT_TRUE(task());
	Verify(Method(completeReadRegsMock, func).Using(1, 0, 8)).Once();
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
	Verify(Method(completeReadRegsMock, func).Using(1, 0, 8)).Once();
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
	Verify(Method(completeReadRegsMock, func).Using(1, 0, 8)).Once();
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
	Verify(Method(completeReadRegsMock, func).Using(1, 0, 8)).Once();
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
	Verify(Method(completeReadRegsMock, func).Using(1, 0, 8)).Once();
	ASSERT_EQ(task.result(), badSlave);
}

TEST_F(MasterTests, processNewSlave_Success_ThreeDevices)
{
	byte curSlaveId = 1;
	MOCK_MODBUS;
	MockNewMethod(addDeviceName, string name);

	When(Method(mockDeviceDirectory, findFreeSlaveID)).Return(13);
	When(Method(mockDeviceDirectory, addOrReplaceDevice)).AlwaysDo([&addDeviceName](byte* name, DeviceDirectoryRow row)
	{
		addDeviceName.get().method(stringifyCharArray(6, (char*)name));
		return 0;
	});

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	When(Method(completeWriteRegsMock, func)).AlwaysDo([&curSlaveId](byte slaveId, word start, word count, word* data)
	{
		if ((count == 3) &&
			data[0] == 1 &&
			data[1] == 1)
			curSlaveId = data[2];
		return true;
	});
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsStack regsStack;
	regsStack.push(withString(REGS(3, 2, 1, 9), "TEAM C"));
	regsStack.push(withString(REGS(3, 2, 1, 8), "TEAM B"));
	regsStack.push(withString(REGS(3, 2, 1, 7), "TEAM A"));
	regsStack.push(REGS(8, 0, 1 << 8, 3, 6, 22, 0, 0, 0));
	isRegsResponse_UseMockData(modbusBaseMock, regsStack);

	T_MASTER::processNewSlave_Task task(&T_MASTER::processNewSlave, master, false);
	ASSERT_TRUE(task());
	ASSERT_TRUE(master->_timeUpdatePending);

	// Three requests for device data, plus write new slave ID
	Verify(Method(completeWriteRegsMock, func).Using(1, 0, 3, Any<word*>())).Exactly(4);

	// Add three new devices to device directory
	Verify(Method(mockDeviceDirectory, addOrReplaceDevice).Using(Any<byte*>(), DeviceDirectoryRow(13, 0, 7, 22))).Once();
	Verify(Method(mockDeviceDirectory, addOrReplaceDevice).Using(Any<byte*>(), DeviceDirectoryRow(13, 1, 8, 22))).Once();
	Verify(Method(mockDeviceDirectory, addOrReplaceDevice).Using(Any<byte*>(), DeviceDirectoryRow(13, 2, 9, 22))).Once();
	Verify(Method(addDeviceName, method).Using("TEAM A") +
		Method(addDeviceName, method).Using("TEAM B") +
		Method(addDeviceName, method).Using("TEAM C")).Once();

	// Slave ID set to 13
	ASSERT_EQ(curSlaveId, 13);
}

TEST_F(MasterTests, processNewSlave_Reject_ByRequest)
{
	byte curSlaveId = 1;
	MOCK_MODBUS;

	When(Method(mockDeviceDirectory, findFreeSlaveID)).Return(13);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	When(Method(completeWriteRegsMock, func)).AlwaysDo([&curSlaveId](byte slaveId, word start, word count, word* data)
	{
		if ((count == 3) &&
			data[0] == 1 &&
			data[1] == 1)
			curSlaveId = data[2];
		return true;
	});
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsStack regsStack;
	regsStack.push(REGS(7, 0, 1 << 8, 3, 6, 0, 0, 0));
	isRegsResponse_UseMockData(modbusBaseMock, regsStack);

	T_MASTER::processNewSlave_Task task(&T_MASTER::processNewSlave, master, true);
	ASSERT_TRUE(task());
	ASSERT_FALSE(master->_timeUpdatePending);

	// Three requests for device data, plus write new slave ID
	Verify(Method(completeWriteRegsMock, func).Using(1, 0, 3, Any<word*>())).Once();

	// Slave ID set to 13
	ASSERT_EQ(curSlaveId, 255);
}

TEST_F(MasterTests, processNewSlave_Reject_DirectoryAlreadyFull)
{
	byte curSlaveId = 1;
	MOCK_MODBUS;

	When(Method(mockDeviceDirectory, findFreeSlaveID)).Return(0);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	When(Method(completeWriteRegsMock, func)).AlwaysDo([&curSlaveId](byte slaveId, word start, word count, word* data)
	{
		if ((count == 3) &&
			data[0] == 1 &&
			data[1] == 1)
			curSlaveId = data[2];
		return true;
	});
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsStack regsStack;
	regsStack.push(REGS(7, 0, 1 << 8, 3, 6, 0, 0, 0 ));
	isRegsResponse_UseMockData(modbusBaseMock, regsStack);

	T_MASTER::processNewSlave_Task task(&T_MASTER::processNewSlave, master, false);
	ASSERT_TRUE(task());
	ASSERT_FALSE(master->_timeUpdatePending);

	// Three requests for device data, plus write new slave ID
	Verify(Method(completeWriteRegsMock, func).Using(1, 0, 3, Any<word*>())).Once();

	// Slave ID set to 13
	ASSERT_EQ(curSlaveId, 255);
}

TEST_F(MasterTests, processNewSlave_Reject_SlaveVersionMismatch)
{
	byte curSlaveId = 1;
	MOCK_MODBUS;

	When(Method(mockDeviceDirectory, findFreeSlaveID)).Return(13);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	When(Method(completeWriteRegsMock, func)).AlwaysDo([&curSlaveId](byte slaveId, word start, word count, word* data)
	{
		if ((count == 3) &&
			data[0] == 1 &&
			data[1] == 1)
			curSlaveId = data[2];
		return true;
	});
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsStack regsStack;
	regsStack.push(REGS(7, 0, 0, 3, 6, 0, 0, 0));
	isRegsResponse_UseMockData(modbusBaseMock, regsStack);

	T_MASTER::processNewSlave_Task task(&T_MASTER::processNewSlave, master, false);
	ASSERT_TRUE(task());
	ASSERT_FALSE(master->_timeUpdatePending);

	// Three requests for device data, plus write new slave ID
	Verify(Method(completeWriteRegsMock, func).Using(1, 0, 3, Any<word*>())).Once();

	// Slave ID set to 13
	ASSERT_EQ(curSlaveId, 255);
}

TEST_F(MasterTests, processNewSlave_Reject_ZeroDevices)
{
	byte curSlaveId = 1;
	MOCK_MODBUS;

	When(Method(mockDeviceDirectory, findFreeSlaveID)).Return(13);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	When(Method(completeWriteRegsMock, func)).AlwaysDo([&curSlaveId](byte slaveId, word start, word count, word* data)
	{
		if ((count == 3) &&
			data[0] == 1 &&
			data[1] == 1)
			curSlaveId = data[2];
		return true;
	});
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsStack regsStack;
	regsStack.push(REGS(7,0, 1 << 8, 0, 6, 0, 0, 0));
	isRegsResponse_UseMockData(modbusBaseMock, regsStack);

	T_MASTER::processNewSlave_Task task(&T_MASTER::processNewSlave, master, false);
	ASSERT_TRUE(task());
	ASSERT_FALSE(master->_timeUpdatePending);

	// Three requests for device data, plus write new slave ID
	Verify(Method(completeWriteRegsMock, func).Using(1, 0, 3, Any<word*>())).Once();

	// Slave ID set to 13
	ASSERT_EQ(curSlaveId, 255);
}

TEST_F(MasterTests, processNewSlave_Reject_DirectoryGetsFilled)
{
	byte curSlaveId = 1;
	MOCK_MODBUS;
	MockNewMethod(addDeviceName, string name);

	When(Method(mockDeviceDirectory, findFreeSlaveID)).Return(13);

	int callCount = 0;
	When(Method(mockDeviceDirectory, addOrReplaceDevice)).AlwaysDo(
		[&addDeviceName, &callCount](byte* name, DeviceDirectoryRow row)
	{
		addDeviceName.get().method(stringifyCharArray(6, (char*)name));
		
		// Only work on the first call
		return (callCount++ == 0) ? 0 : -1;
	});
	When(Method(mockDeviceDirectory, filterDevicesForSlave)).Return(1);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	When(Method(completeWriteRegsMock, func)).AlwaysDo([&curSlaveId](byte slaveId, word start, word count, word* data)
	{
		if ((count == 3) &&
			data[0] == 1 &&
			data[1] == 1)
			curSlaveId = data[2];
		return true;
	});
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsStack regsStack;
	regsStack.push(withString(REGS(3, 2, 1, 9), "TEAM B"));
	regsStack.push(withString(REGS(3, 2, 1, 8), "TEAM C"));
	regsStack.push(withString(REGS(3, 2, 1, 7), "TEAM A"));
	regsStack.push(REGS(8, 0, 1 << 8, 3, 6, 47, 0, 0, 0));
	isRegsResponse_UseMockData(modbusBaseMock, regsStack);

	T_MASTER::processNewSlave_Task task(&T_MASTER::processNewSlave, master, false);
	ASSERT_TRUE(task());
	ASSERT_FALSE(master->_timeUpdatePending);

	// Two requests for device data, plus write new slave ID
	Verify(Method(completeWriteRegsMock, func).Using(1, 0, 3, Any<word*>())).Exactly(3);

	// Add two new devices to device directory, failed one never gets added
	Verify(Method(mockDeviceDirectory, addOrReplaceDevice).Using(Any<byte*>(), DeviceDirectoryRow(13, 0, 7, 47))).Once();
	Verify(Method(mockDeviceDirectory, addOrReplaceDevice).Using(Any<byte*>(), DeviceDirectoryRow(13, 1, 8, 47))).Once();
	Verify(Method(mockDeviceDirectory, addOrReplaceDevice).Using(Any<byte*>(), DeviceDirectoryRow(13, 2, 9, 47))).Never();

	Verify(Method(mockDeviceDirectory, filterDevicesForSlave).Using(nullptr, 0, 13)).Once();

	Verify(Method(addDeviceName, method).Using("TEAM A") +
		Method(addDeviceName, method).Using("TEAM C")).Once();
	Verify(Method(addDeviceName, method).Using("TEAM B")).Never();

	// Slave ID set to 13
	ASSERT_EQ(curSlaveId, 255);
}

TEST_F(MasterTests, broadcastTime_success)
{
	MOCK_MODBUS;
	uint32_t curTime;

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	When(Method(completeWriteRegsMock, func)).AlwaysDo([&curTime](byte slaveId, word start, word count, word* data)
	{
		curTime = *(uint32_t*)(data + 2);
		return true;
	});
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	master->setClock(537408000);
	auto result = master->broadcastTime();

	ASSERT_EQ(result, true);
	ASSERT_EQ(curTime, 537408000);
}

TEST_F(MasterTests, broadcastTime_failure)
{
	MOCK_MODBUS;
	uint32_t curTime;

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	When(Method(completeWriteRegsMock, func)).AlwaysDo([&curTime](byte slaveId, word start, word count, word* data)
	{
		curTime = *(uint32_t*)(data + 2);
		return true;
	});
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(taskFailure);

	master->setClock(537408000);
	auto result = master->broadcastTime();

	ASSERT_EQ(result, false);
	ASSERT_EQ(curTime, 537408000);
}

TEST_F(MasterTests, setClock_SetsTimeUpdatePending)
{
	master->setClock(0);

	ASSERT_TRUE(master->_timeUpdatePending);
}

TEST_F(MasterTests, transferPendingData_AllTimescales_Success)
{
	MOCK_MODBUS;
	When(Method(mockDeviceDirectory, getDeviceNameLength)).AlwaysReturn(7);
	auto devices = Setup_DataCollectorsAndTransmitters();

	Mock<IMockedTask<void, DeviceDirectoryRow*, word, byte*, uint32_t, uint32_t>> readAndSendDeviceDataMock;
	T_MASTER::readAndSendDeviceData_Task::mock = &readAndSendDeviceDataMock.get();
	When(Method(readAndSendDeviceDataMock, func)).AlwaysReturn(true);
	Fake(Method(readAndSendDeviceDataMock, result));

	for (int i = 0; i < 8; i++)
	{
		master->lastUpdateTimes[i] = i + 5;
	}

	T_MASTER::transferPendingData_Task task(&T_MASTER::transferPendingData, master, TimeScale::hr24, 20);
	ASSERT_TRUE(task());

	Verify(Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 0), 7, getDeviceNamePtr(devices, 0), 5, 20) +
		Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 1), 7, getDeviceNamePtr(devices, 1), 6, 20) +
		Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 3), 7, getDeviceNamePtr(devices, 3), 11, 20) +
		Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 5), 7, getDeviceNamePtr(devices, 5), 8, 20) +
		Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 6), 7, getDeviceNamePtr(devices, 6), 12, 20) +
		Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 7), 7, getDeviceNamePtr(devices, 7), 7, 20) +
		Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 8), 7, getDeviceNamePtr(devices, 8), 10, 20)).Once();
	assertArrayEq<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
		master->lastUpdateTimes, 20, 20, 20, 20, 20, 20, 20, 20);
}

TEST_F(MasterTests, transferPendingData_30MinMax_Success)
{
	MOCK_MODBUS;
	When(Method(mockDeviceDirectory, getDeviceNameLength)).AlwaysReturn(7);
	auto devices = Setup_DataCollectorsAndTransmitters();

	Mock<IMockedTask<void, DeviceDirectoryRow*, word, byte*, uint32_t, uint32_t>> readAndSendDeviceDataMock;
	T_MASTER::readAndSendDeviceData_Task::mock = &readAndSendDeviceDataMock.get();
	When(Method(readAndSendDeviceDataMock, func)).AlwaysReturn(true);
	Fake(Method(readAndSendDeviceDataMock, result));

	for (int i = 0; i < 8; i++)
	{
		master->lastUpdateTimes[i] = i + 5;
	}

	T_MASTER::transferPendingData_Task task(&T_MASTER::transferPendingData, master, TimeScale::min30, 20);
	ASSERT_TRUE(task());

	Verify(Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 0), 7, getDeviceNamePtr(devices, 0), 5, 20) +
		Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 1), 7, getDeviceNamePtr(devices, 1), 6, 20) +
		Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 5), 7, getDeviceNamePtr(devices, 5), 8, 20) +
		Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 7), 7, getDeviceNamePtr(devices, 7), 7, 20) +
		Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 8), 7, getDeviceNamePtr(devices, 8), 10, 20)).Once();
	assertArrayEq<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
		master->lastUpdateTimes, 20, 20, 20, 20, 20, 20, 11, 12);
}

TEST_F(MasterTests, transferPendingData_1SecMax_Success)
{
	MOCK_MODBUS;
	When(Method(mockDeviceDirectory, getDeviceNameLength)).AlwaysReturn(7);
	auto devices = Setup_DataCollectorsAndTransmitters();

	Mock<IMockedTask<void, DeviceDirectoryRow*, word, byte*, uint32_t, uint32_t>> readAndSendDeviceDataMock;
	T_MASTER::readAndSendDeviceData_Task::mock = &readAndSendDeviceDataMock.get();
	When(Method(readAndSendDeviceDataMock, func)).AlwaysReturn(true);
	Fake(Method(readAndSendDeviceDataMock, result));

	for (int i = 0; i < 8; i++)
	{
		master->lastUpdateTimes[i] = i + 5;
	}

	T_MASTER::transferPendingData_Task task(&T_MASTER::transferPendingData, master, TimeScale::sec1, 20);
	ASSERT_TRUE(task());

	Verify(Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 0), 7, getDeviceNamePtr(devices, 0), 5, 20) +
		Method(readAndSendDeviceDataMock, func).Using(getDevicePtr(devices, 1), 7, getDeviceNamePtr(devices, 1), 6, 20)).Once();
	assertArrayEq<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
		master->lastUpdateTimes, 20, 20, 7, 8, 9, 10, 11, 12);
}

TEST_F(MasterTests, transferPendingData_1SecMax_NoDevices)
{
	MOCK_MODBUS;
	When(Method(mockDeviceDirectory, getDeviceNameLength)).AlwaysReturn(7);
	auto devices = Setup_DataCollectorsAndTransmitters();
	devices->erase(devices->begin(), devices->begin() + 2);

	Mock<IMockedTask<void, DeviceDirectoryRow*, word, byte*, uint32_t, uint32_t>> readAndSendDeviceDataMock;
	T_MASTER::readAndSendDeviceData_Task::mock = &readAndSendDeviceDataMock.get();
	When(Method(readAndSendDeviceDataMock, func)).AlwaysReturn(true);
	Fake(Method(readAndSendDeviceDataMock, result));

	for (int i = 0; i < 8; i++)
	{
		master->lastUpdateTimes[i] = i + 5;
	}

	T_MASTER::transferPendingData_Task task(&T_MASTER::transferPendingData, master, TimeScale::sec1, 20);
	ASSERT_TRUE(task());

	Verify(Method(readAndSendDeviceDataMock, func)).Never();
	assertArrayEq<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
		master->lastUpdateTimes, 20, 20, 7, 8, 9, 10, 11, 12);
}
//
//TEST_F(MasterTests, readAndSendDeviceData_Success)
//{
//	MOCK_MODBUS;
//	When(Method(mockDeviceDirectory, getDeviceNameLength)).AlwaysReturn(7);
//	auto devices = Setup_DataCollectorsAndTransmitters();
//
//	Mock<IMockedTask<void, uint32_t, byte, TimeScale, word, byte*, byte*>> sendDataToSlavesMock;
//	T_MASTER::sendDataToSlaves_Task::mock = &sendDataToSlavesMock.get();
//	When(Method(sendDataToSlavesMock, func)).AlwaysReturn(true);
//	Fake(Method(sendDataToSlavesMock, result));
//
//	for (int i = 0; i < 8; i++)
//	{
//		master->lastUpdateTimes[i] = i + 5;
//	}
//
//	T_MASTER::transferPendingData_Task task(&T_MASTER::transferPendingData, master, TimeScale::sec1, 20);
//	ASSERT_TRUE(task());
//
//	Verify(Method(sendDataToSlavesMock, func)).Never();
//}