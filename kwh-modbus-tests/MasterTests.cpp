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

#include <queue>
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

	typedef queue<tuple<word, word*>> RegsQueue;

	void SetupOutOfRangeRegisterArray()
	{
		modbus->init(registerArray, 20, 12, 20);
		master->config(system, modbus, &mockDeviceDirectory.get(), 10, 10);
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
		devices->push_back(make_tuple((byte*)"device4", DeviceDirectoryRow(6, 2, DataTransmitterDeviceType(), 10)));
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

	void isRegsResponse_UseMockData(Mock<T_MODBUS_BASE> &mock, RegsQueue &regsQueue)
	{
		When(Method(mock, isReadRegsResponse)).AlwaysDo([&regsQueue](word &regCount, word *&regs) {
			auto next = regsQueue.front();
			regsQueue.pop();
			regCount = get<0>(next);
			regs = get<1>(next);
			return true;
		});
	}

	void completeWriteRegs_UseRegsQueue_Passthrough(Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> &mock, RegsQueue &queue, function<bool(byte, word, word, word*)> alwaysDo)
	{
		When(Method(mock, func)).AlwaysDo([&queue, this, alwaysDo](byte slaveId, word start, word count, word* data)
		{
			word *regs = tracker.addArray(new word[count]);
			BitFunctions::copyBits(data, regs, 0, 0, 16 * count);
			queue.push(make_tuple(count, regs));
			return alwaysDo(slaveId, start, count, data);
		});
	}

	void completeWriteRegs_UseRegsQueue(Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> &mock, RegsQueue &queue)
	{
		completeWriteRegs_UseRegsQueue_Passthrough(mock, queue,
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

	void assertPopRegsQueue(RegsQueue &regsQueue, tuple<int, word*> regs)
	{
		auto top = regsQueue.front();
		regsQueue.pop();

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
		master->config(system, modbus, &mockDeviceDirectory.get(), 10, 10);
		modbus->config(serial, system, 1200);
		tracker.addPointers(modbus, master, serial, system);
		tracker.addArrays(registerArray);
	}

	void TearDown()
	{
		// Null the mocks so they don't interfere with other tests.
		T_MASTER::sendDataToSlaves_Task::mock = nullptr;
		T_MASTER::completeModbusReadRegisters_Task::mock = nullptr;
		T_MASTER::completeModbusWriteRegisters_Task::mock = nullptr;
		T_MASTER::readAndSendDeviceData_Task::mock = nullptr;
	}
};

TEST_F_TRAITS(MasterTests, ensureTaskNotStarted_NeedsReset_Doesnt,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(MasterTests, ensureTaskNotStarted_NeedsReset_Does,
	Type, Unit, Threading, Single, Determinism, Static, Case, Rare)
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

TEST_F_TRAITS(MasterTests, ensureTaskNotStarted_DoesntNeedReset,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	MOCK_MODBUS;
	Fake(Method(modbusTaskMock, reset));
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskNotStarted);

	T_MASTER::ensureTaskNotStarted_Task task(&T_MASTER::ensureTaskNotStarted, master);
	ASSERT_TRUE(task());

	Verify(Method(modbusTaskMock, getStatus)).Once();
	Verify(Method(modbusTaskMock, reset)).Never();
}

TEST_F_TRAITS(MasterTests, completeModbusReadRegisters_CompletesImmediately,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(MasterTests, completeModbusReadRegisters_CompletesWithSomeAttempts,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(MasterTests, completeModbusReadRegisters_MasterFailure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
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

TEST_F_TRAITS(MasterTests, completeModbusReadRegisters_TaskFailure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskFatal);
	When(Method(modbusBaseMock, setRequest_ReadRegisters)).AlwaysReturn(true);

	T_MASTER::completeModbusReadRegisters_Task task(&T_MASTER::completeModbusReadRegisters, master, 2, 3, 5);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), taskFailure);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_ReadRegisters).Using(2, 3, 5)).Once();
}

TEST_F_TRAITS(MasterTests, completeModbusReadRegisters_IncorrectResponseSize,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
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

TEST_F_TRAITS(MasterTests, completeModbusReadRegisters_ExceptionResponse,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
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

TEST_F_TRAITS(MasterTests, completeModbusReadRegisters_OtherResponse,
	Type, Unit, Threading, Single, Determinism, Static, Case, Rare)
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

TEST_F_TRAITS(MasterTests, completeModbusReadRegisters_NoResponse,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskFullyAttempted);
	When(Method(modbusBaseMock, setRequest_ReadRegisters)).AlwaysReturn(true);

	T_MASTER::completeModbusReadRegisters_Task task(&T_MASTER::completeModbusReadRegisters, master, 2, 3, 5);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), noResponse);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_ReadRegisters).Using(2, 3, 5)).Once();
}

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_single_CompletesImmediately,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_single_CompletesWithSomeAttempts,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_single_MasterFailure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
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

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_single_TaskFailure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
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

	ASSERT_EQ(task.result(), taskFailure);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_WriteRegister).Using(2, 3, 6)).Once();
}

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_single_ExceptionResponse,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
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

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_single_OtherResponse,
	Type, Unit, Threading, Single, Determinism, Static, Case, Rare)
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

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_single_NoResponse,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskFullyAttempted);
	When(Method(modbusBaseMock, setRequest_WriteRegister)).AlwaysReturn(true);

	word val = 6;
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 1, &val);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), noResponse);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_WriteRegister).Using(2, 3, 6)).Once();
}

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_multiple_CompletesImmediately,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_multiple_CompletesWithSomeAttempts,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_multiple_MasterFailure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
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

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_multiple_TaskFailure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
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

	ASSERT_EQ(task.result(), taskFailure);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_WriteRegisters).Using(2, 3, 3, (word*)vals)).Once();
}

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_multiple_ExceptionResponse,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
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

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_multiple_OtherResponse,
	Type, Unit, Threading, Single, Determinism, Static, Case, Rare)
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

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_multiple_NoResponse,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	When(OverloadedMethod(masterMock, ensureTaskNotStarted, bool(T_MASTER::ensureTaskNotStarted_Param&))).Return(true);
	When(OverloadedMethod(modbusTaskMock, work, bool())).AlwaysReturn(true);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskFullyAttempted);
	When(Method(modbusBaseMock, setRequest_WriteRegisters)).AlwaysReturn(true);

	word vals[3] = { 1, 2, 3 };
	T_MASTER::completeModbusWriteRegisters_Task task(&T_MASTER::completeModbusWriteRegisters, master, 2, 3, 3, (word*)vals);
	ASSERT_TRUE(task());

	ASSERT_EQ(task.result(), noResponse);
	Verify(OverloadedMethod(modbusTaskMock, work, bool())).Once();
	Verify(Method(modbusBaseMock, setRequest_WriteRegisters).Using(2, 3, 3, (word*)vals)).Once();
}

TEST_F_TRAITS(MasterTests, completeModbusWriteRegisters_broadcast_succeeds,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(MasterTests, checkForNewSlaves_Found,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).Return(true);
	When(Method(completeReadRegsMock, result)).Return(success);

	T_MASTER::checkForNewSlaves_Task task(&T_MASTER::checkForNewSlaves, master, 71);
	ASSERT_TRUE(task());

	Verify(Method(completeReadRegsMock, func).Using(71, 0, 8)).Once();
	ASSERT_EQ(task.result(), found);
}

TEST_F_TRAITS(MasterTests, checkForNewSlaves_NotFound,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	MOCK_MODBUS;
	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).Return(true);
	When(Method(completeReadRegsMock, result)).Return(taskFailure);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskTimeOut);

	T_MASTER::checkForNewSlaves_Task task(&T_MASTER::checkForNewSlaves, master, 53);
	ASSERT_TRUE(task());
	Verify(Method(completeReadRegsMock, func).Using(53, 0, 8)).Once();
	ASSERT_EQ(task.result(), notFound);
}

TEST_F_TRAITS(MasterTests, checkForNewSlaves_Error_TaskFailure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	MOCK_MASTER;
	MOCK_MODBUS;
	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).Return(true);
	When(Method(completeReadRegsMock, result)).Return(taskFailure);
	When(Method(modbusTaskMock, getStatus)).AlwaysReturn(TaskFatal);
	Fake(Method(masterMock, reportMalfunction));

	T_MASTER::checkForNewSlaves_Task task(&T_MASTER::checkForNewSlaves, master, 80);
	ASSERT_TRUE(task());
	Verify(Method(completeReadRegsMock, func).Using(80, 0, 8)).Once();
	ASSERT_EQ(task.result(), error);
	Verify(Method(masterMock, reportMalfunction)).Once();
}

TEST_F_TRAITS(MasterTests, checkForNewSlaves_Error_MasterFailure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	MOCK_MASTER;
	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).Return(true);
	When(Method(completeReadRegsMock, result)).Return(masterFailure);
	Fake(Method(masterMock, reportMalfunction));

	T_MASTER::checkForNewSlaves_Task task(&T_MASTER::checkForNewSlaves, master, 32);
	ASSERT_TRUE(task());
	Verify(Method(completeReadRegsMock, func).Using(32, 0, 8)).Once();
	ASSERT_EQ(task.result(), error);
	Verify(Method(masterMock, reportMalfunction)).Once();
}

TEST_F_TRAITS(MasterTests, checkForNewSlaves_BadSlave,
	Type, Unit, Threading, Single, Determinism, Static, Case, Rare)
{
	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).Return(true);
	When(Method(completeReadRegsMock, result)).Return(exceptionResponse);

	T_MASTER::checkForNewSlaves_Task task(&T_MASTER::checkForNewSlaves, master, 71);
	ASSERT_TRUE(task());
	Verify(Method(completeReadRegsMock, func).Using(71, 0, 8)).Once();
	ASSERT_EQ(task.result(), badSlave);
}

TEST_F_TRAITS(MasterTests, processNewSlave_Reject_DeviceStopsResponding,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	MOCK_MODBUS;
	MockNewMethod(addDeviceName, string name);
	When(Method(modbusBaseMock, getRecipientId)).Return(5);

	When(Method(mockDeviceDirectory, findFreeSlaveID)).Return(13);
	When(Method(mockDeviceDirectory, addOrReplaceDevice)).AlwaysDo([&addDeviceName](byte* name, DeviceDirectoryRow row)
	{
		addDeviceName.get().method(stringifyCharArray(6, (char*)name));
		return 0;
	});
	When(Method(mockDeviceDirectory, filterDevicesForSlave)).Return(1);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(noResponse);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(8, 0, 1 << 8, 3, 6, 22, 0, 0, 0));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	T_MASTER::processNewSlave_Task task(&T_MASTER::processNewSlave, master, false);
	ASSERT_TRUE(task());
	ASSERT_FALSE(master->_timeUpdatePending);



	// One write to look for active slaves, and one to set slave ID.
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 3, Any<word*>())).Twice();


	// Device directory filtered
	Verify(Method(mockDeviceDirectory, filterDevicesForSlave).Using(nullptr, 0, 13));

	// Slave ID set to 255, rejected
	assertPopRegsQueue(writtenRegs, REGS(3, 1, 2, 0));
	assertPopRegsQueue(writtenRegs, REGS(3, 1, 1, 255));
}

TEST_F_TRAITS(MasterTests, processNewSlave_Reject_ByRequest,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	MOCK_MODBUS;

	When(Method(mockDeviceDirectory, findFreeSlaveID)).Return(13);
	When(Method(modbusBaseMock, getRecipientId)).Return(5);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue regsQueue;
	regsQueue.push(REGS(7, 0, 1 << 8, 3, 6, 0, 0, 0));
	isRegsResponse_UseMockData(modbusBaseMock, regsQueue);

	T_MASTER::processNewSlave_Task task(&T_MASTER::processNewSlave, master, true);
	ASSERT_TRUE(task());
	ASSERT_FALSE(master->_timeUpdatePending);

	// Three requests for device data, plus write new slave ID
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 3, Any<word*>())).Once();

	// Slave ID set to 255, rejected
	assertPopRegsQueue(writtenRegs, REGS(3, 1, 1, 255));
}

TEST_F_TRAITS(MasterTests, processNewSlave_Reject_DirectoryAlreadyFull,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	MOCK_MODBUS;

	When(Method(mockDeviceDirectory, findFreeSlaveID)).Return(0);
	When(Method(modbusBaseMock, getRecipientId)).Return(5);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue regsQueue;
	regsQueue.push(REGS(7, 0, 1 << 8, 3, 6, 0, 0, 0 ));
	isRegsResponse_UseMockData(modbusBaseMock, regsQueue);

	T_MASTER::processNewSlave_Task task(&T_MASTER::processNewSlave, master, false);
	ASSERT_TRUE(task());
	ASSERT_FALSE(master->_timeUpdatePending);

	// Three requests for device data, plus write new slave ID
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 3, Any<word*>())).Once();

	// Slave ID set to 255, rejected
	assertPopRegsQueue(writtenRegs, REGS(3, 1, 1, 255));
}

TEST_F_TRAITS(MasterTests, processNewSlave_Reject_SlaveVersionMismatch,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	MOCK_MODBUS;

	When(Method(mockDeviceDirectory, findFreeSlaveID)).Return(13);
	When(Method(modbusBaseMock, getRecipientId)).Return(5);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue regsQueue;
	regsQueue.push(REGS(7, 0, 0, 3, 6, 0, 0, 0));
	isRegsResponse_UseMockData(modbusBaseMock, regsQueue);

	T_MASTER::processNewSlave_Task task(&T_MASTER::processNewSlave, master, false);
	ASSERT_TRUE(task());
	ASSERT_FALSE(master->_timeUpdatePending);

	// Three requests for device data, plus write new slave ID
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 3, Any<word*>())).Once();

	// Slave ID set to 255, rejected
	assertPopRegsQueue(writtenRegs, REGS(3, 1, 1, 255));
}

TEST_F_TRAITS(MasterTests, processNewSlave_Reject_ZeroDevices,
	Type, Unit, Threading, Single, Determinism, Static, Case, Rare)
{
	MOCK_MODBUS;

	When(Method(mockDeviceDirectory, findFreeSlaveID)).Return(13);
	When(Method(modbusBaseMock, getRecipientId)).Return(5);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue regsQueue;
	regsQueue.push(REGS(7,0, 1 << 8, 0, 6, 0, 0, 0));
	isRegsResponse_UseMockData(modbusBaseMock, regsQueue);

	T_MASTER::processNewSlave_Task task(&T_MASTER::processNewSlave, master, false);
	ASSERT_TRUE(task());
	ASSERT_FALSE(master->_timeUpdatePending);

	// Three requests for device data, plus write new slave ID
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 3, Any<word*>())).Once();

	// Slave ID set to 255, rejected
	assertPopRegsQueue(writtenRegs, REGS(3, 1, 1, 255));
}

TEST_F_TRAITS(MasterTests, processNewSlave_Reject_DirectoryGetsFilled,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	MOCK_MODBUS;
	MockNewMethod(addDeviceName, string name);

	When(Method(mockDeviceDirectory, findFreeSlaveID)).Return(13);
	When(Method(modbusBaseMock, getRecipientId)).Return(5);

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
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue regsQueue;
	regsQueue.push(REGS(8, 0, 1 << 8, 3, 6, 47, 0, 0, 0));
	regsQueue.push(withString(REGS(3, 2, 1, 7), "TEAM A"));
	regsQueue.push(withString(REGS(3, 2, 1, 8), "TEAM C"));
	regsQueue.push(withString(REGS(3, 2, 1, 9), "TEAM B"));
	isRegsResponse_UseMockData(modbusBaseMock, regsQueue);

	T_MASTER::processNewSlave_Task task(&T_MASTER::processNewSlave, master, false);
	ASSERT_TRUE(task());
	ASSERT_FALSE(master->_timeUpdatePending);

	// Two requests for device data, plus write new slave ID
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 3, Any<word*>())).Exactly(3);

	// Add two new devices to device directory, failed one never gets added
	Verify(Method(mockDeviceDirectory, addOrReplaceDevice).Using(Any<byte*>(), DeviceDirectoryRow(13, 0, 7, 47))).Once();
	Verify(Method(mockDeviceDirectory, addOrReplaceDevice).Using(Any<byte*>(), DeviceDirectoryRow(13, 1, 8, 47))).Once();
	Verify(Method(mockDeviceDirectory, addOrReplaceDevice).Using(Any<byte*>(), DeviceDirectoryRow(13, 2, 9, 47))).Never();

	Verify(Method(mockDeviceDirectory, filterDevicesForSlave).Using(nullptr, 0, 13)).Once();

	Verify(Method(addDeviceName, method).Using("TEAM A") +
		Method(addDeviceName, method).Using("TEAM C")).Once();
	Verify(Method(addDeviceName, method).Using("TEAM B")).Never();

	// Slave ID set to 255, rejected
	assertPopRegsQueue(writtenRegs, REGS(3, 1, 2, 0));
	assertPopRegsQueue(writtenRegs, REGS(3, 1, 2, 1));
	assertPopRegsQueue(writtenRegs, REGS(3, 1, 1, 255));
}

TEST_F_TRAITS(MasterTests, broadcastTime_success,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	MOCK_MODBUS;

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue regsQueue;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, regsQueue);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	master->setClock(537408000);
	auto result = master->broadcastTime();

	ASSERT_EQ(result, true);
	assertPopRegsQueue(regsQueue, REGS(4, 1, 32770, 0x3200, 0x2008));
}

TEST_F_TRAITS(MasterTests, broadcastTime_failure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	MOCK_MODBUS;

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue regsQueue;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, regsQueue);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(taskFailure);

	master->setClock(537408000);
	auto result = master->broadcastTime();

	ASSERT_EQ(result, false);
	assertPopRegsQueue(regsQueue, REGS(4, 1, 32770, 0x3200, 0x2008));
}

TEST_F_TRAITS(MasterTests, setClock_SetsTimeUpdatePending,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	master->setClock(0);

	ASSERT_TRUE(master->_timeUpdatePending);
}

TEST_F_TRAITS(MasterTests, transferPendingData_AllTimescales_Success,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(MasterTests, transferPendingData_30MinMax_Success,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(MasterTests, transferPendingData_1SecMax_Success,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(MasterTests, transferPendingData_1SecMax_NoDevices,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
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

TEST_F_TRAITS(MasterTests, readAndSendDeviceData_Success_OneReadPage,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	// Arrange
	MOCK_MODBUS;
	MockNewMethod(deviceNameSentToSlaves, string name);
	MockNewMethod(dataByteSentToSlaves, byte data);

	Mock<IMockedTask<void, uint32_t, byte, TimeScale, word, byte*, byte*>> sendDataToSlavesMock;
	T_MASTER::sendDataToSlaves_Task::mock = &sendDataToSlavesMock.get();
	When(Method(sendDataToSlavesMock, func)).AlwaysDo([&deviceNameSentToSlaves, &dataByteSentToSlaves](uint32_t startTime,
		byte dataSize, TimeScale timeScale, word numDataPoints, byte* name, byte* data)
	{
		deviceNameSentToSlaves.get().method(stringifyCharArray(9, (char*)name));
		for (int i = 0; i < 8; i++)
			dataByteSentToSlaves.get().method(data[i]);
		return true;
	});
	Fake(Method(sendDataToSlavesMock, result));

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(6, 3, 0, 15000, 0, 8, 0));
	readRegs.push(REGS(4, 0x4182, 0xB0E1, 0x4468, 0x26));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	// Act
	DeviceDirectoryRow inputDeviceRow = DeviceDirectoryRow(5, 3, DataCollectorDeviceType(true, TimeScale::min10, 7), 14);
	auto name = (byte*)"Meter 001";
	T_MASTER::readAndSendDeviceData_Task task(&T_MASTER::readAndSendDeviceData, master, &inputDeviceRow, 9, name, 15000, 20000);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, REGS(7, 1, 3, 3, 15000, 0, 8, 11 << 8));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 7, Any<word*>())).Once();
	Verify(Method(sendDataToSlavesMock, func).Using(15000, 7, TimeScale::min10, 8, Any<byte*>(), Any<byte*>())).Once();
	Verify(Method(deviceNameSentToSlaves, method).Using("Meter 001")).Once();
	Verify(Method(dataByteSentToSlaves, method).Using(0x82) +
		Method(dataByteSentToSlaves, method).Using(0x41) +
		Method(dataByteSentToSlaves, method).Using(0xE1) +
		Method(dataByteSentToSlaves, method).Using(0xB0) +
		Method(dataByteSentToSlaves, method).Using(0x68) +
		Method(dataByteSentToSlaves, method).Using(0x44) +
		Method(dataByteSentToSlaves, method).Using(0x26)).Once();
}

TEST_F_TRAITS(MasterTests, readAndSendDeviceData_Success_TwoReadPages,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	// Arrange
	MOCK_MODBUS;
	MockNewMethod(deviceNameSentToSlaves, string name);
	MockNewMethod(dataByteSentToSlaves, byte data);

	Mock<IMockedTask<void, uint32_t, byte, TimeScale, word, byte*, byte*>> sendDataToSlavesMock;
	T_MASTER::sendDataToSlaves_Task::mock = &sendDataToSlavesMock.get();
	When(Method(sendDataToSlavesMock, func)).AlwaysDo([&deviceNameSentToSlaves, &dataByteSentToSlaves](uint32_t startTime,
		byte dataSize, TimeScale timeScale, word numDataPoints, byte* name, byte* data)
	{
		deviceNameSentToSlaves.get().method(stringifyCharArray(9, (char*)name));
		for (int i = 0; i < BitFunctions::bitsToBytes(dataSize * numDataPoints); i++)
			dataByteSentToSlaves.get().method(data[i]);
		return true;
	});
	Fake(Method(sendDataToSlavesMock, result));

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(6, 3, 0, 15000, 0, 5, 1 << 8));
	readRegs.push(REGS(3, 0x4182, 0xB0E1, 0x0));
	readRegs.push(REGS(6, 3, 0, 15000, 0, 3, 1));
	readRegs.push(REGS(2, 0xC88D, 0x4));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	// Act
	DeviceDirectoryRow inputDeviceRow = DeviceDirectoryRow(5, 3, DataCollectorDeviceType(true, TimeScale::min10, 7), 14);
	auto name = (byte*)"Meter 001";
	T_MASTER::readAndSendDeviceData_Task task(&T_MASTER::readAndSendDeviceData, master, &inputDeviceRow, 9, name, 15000, 20000);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, REGS(7, 1, 3, 3, 15000, 0, 8, 11 << 8));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 7, Any<word*>())).Twice();
	Verify(Method(sendDataToSlavesMock, func).Using(15000, 7, TimeScale::min10, 5, Any<byte*>(), Any<byte*>()) +
		Method(sendDataToSlavesMock, func).Using(18000, 7, TimeScale::min10, 3, Any<byte*>(), Any<byte*>())).Once();
	Verify(Method(deviceNameSentToSlaves, method).Using("Meter 001")).Twice();
	Verify(Method(dataByteSentToSlaves, method).Using(0x82) +
		Method(dataByteSentToSlaves, method).Using(0x41) +
		Method(dataByteSentToSlaves, method).Using(0xE1) +
		Method(dataByteSentToSlaves, method).Using(0xB0) +
		Method(dataByteSentToSlaves, method).Using(0x0) +
		Method(dataByteSentToSlaves, method).Using(0x8D) +
		Method(dataByteSentToSlaves, method).Using(0xC8) +
		Method(dataByteSentToSlaves, method).Using(0x4)).Once();
}

TEST_F_TRAITS(MasterTests, readAndSendDeviceData_SendEmptyPacket_DeviceNotResponding,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	// Arrange
	MOCK_MODBUS;

	Mock<IMockedTask<void, uint32_t, byte, TimeScale, word, byte*, byte*>> sendDataToSlavesMock;
	T_MASTER::sendDataToSlaves_Task::mock = &sendDataToSlavesMock.get();
	When(Method(sendDataToSlavesMock, func)).AlwaysReturn(true);
	Fake(Method(sendDataToSlavesMock, result));

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(6, 3, 2, 0, 0, 0, 0));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	// Act
	DeviceDirectoryRow inputDeviceRow = DeviceDirectoryRow(5, 3, DataCollectorDeviceType(true, TimeScale::min10, 7), 14);
	auto name = (byte*)"Meter 001";
	T_MASTER::readAndSendDeviceData_Task task(&T_MASTER::readAndSendDeviceData, master, &inputDeviceRow, 9, name, 15000, 20000);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, REGS(7, 1, 3, 3, 15000, 0, 8, 11 << 8));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 7, Any<word*>())).Once();
	Verify(Method(sendDataToSlavesMock, func)).Never();
}

TEST_F_TRAITS(MasterTests, readAndSendDeviceData_Skip_UnsupportedDevice,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	// Arrange
	MOCK_MODBUS;

	Mock<IMockedTask<void, uint32_t, byte, TimeScale, word, byte*, byte*>> sendDataToSlavesMock;
	T_MASTER::sendDataToSlaves_Task::mock = &sendDataToSlavesMock.get();
	When(Method(sendDataToSlavesMock, func)).AlwaysReturn(true);
	Fake(Method(sendDataToSlavesMock, result));

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(6, 3, 2, 0, 0, 0, 0));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	// Act
	DeviceDirectoryRow inputDeviceRow = DeviceDirectoryRow(5, 3, DataCollectorDeviceType(true, TimeScale::min10, 7), 14);
	auto name = (byte*)"Meter 001";
	T_MASTER::readAndSendDeviceData_Task task(&T_MASTER::readAndSendDeviceData, master, &inputDeviceRow, 9, name, 15000, 20000);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, REGS(7, 1, 3, 3, 15000, 0, 8, 11 << 8));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 7, Any<word*>())).Once();
	Verify(Method(sendDataToSlavesMock, func)).Never();
}

TEST_F_TRAITS(MasterTests, readAndSendDeviceData_Success_OneReadPage_Malfunction,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	// Arrange
	MOCK_MODBUS;
	MOCK_MASTER;
	MockNewMethod(deviceNameSentToSlaves, string name);
	MockNewMethod(dataByteSentToSlaves, byte data);

	Mock<IMockedTask<void, uint32_t, byte, TimeScale, word, byte*, byte*>> sendDataToSlavesMock;
	T_MASTER::sendDataToSlaves_Task::mock = &sendDataToSlavesMock.get();
	When(Method(sendDataToSlavesMock, func)).AlwaysDo([&deviceNameSentToSlaves, &dataByteSentToSlaves](uint32_t startTime,
		byte dataSize, TimeScale timeScale, word numDataPoints, byte* name, byte* data)
	{
		deviceNameSentToSlaves.get().method(stringifyCharArray(9, (char*)name));
		for (int i = 0; i < 8; i++)
			dataByteSentToSlaves.get().method(data[i]);
		return true;
	});
	Fake(Method(sendDataToSlavesMock, result));
	Fake(Method(masterMock, reportMalfunction));

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(6, 3, 8, 0, 0, 0, 0));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	master->setClock(0x12345678);

	// Act
	DeviceDirectoryRow inputDeviceRow = DeviceDirectoryRow(5, 3, DataCollectorDeviceType(true, TimeScale::min10, 7), 14);
	auto name = (byte*)"Meter 001";
	T_MASTER::readAndSendDeviceData_Task task(&T_MASTER::readAndSendDeviceData, master, &inputDeviceRow, 9, name, 15000, 20000);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, REGS(7, 1, 3, 3, 15000, 0, 8, 11 << 8));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 7, Any<word*>())).Once();
	Verify(Method(sendDataToSlavesMock, func)).Never();
	Verify(Method(masterMock, reportMalfunction));
}

TEST_F_TRAITS(MasterTests, readAndSendDeviceData_Success_OneReadPage_TimeRequested,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	// Arrange
	MOCK_MODBUS;
	MockNewMethod(deviceNameSentToSlaves, string name);
	MockNewMethod(dataByteSentToSlaves, byte data);

	Mock<IMockedTask<void, uint32_t, byte, TimeScale, word, byte*, byte*>> sendDataToSlavesMock;
	T_MASTER::sendDataToSlaves_Task::mock = &sendDataToSlavesMock.get();
	When(Method(sendDataToSlavesMock, func)).AlwaysDo([&deviceNameSentToSlaves, &dataByteSentToSlaves](uint32_t startTime,
		byte dataSize, TimeScale timeScale, word numDataPoints, byte* name, byte* data)
	{
		deviceNameSentToSlaves.get().method(stringifyCharArray(9, (char*)name));
		for (int i = 0; i < 8; i++)
			dataByteSentToSlaves.get().method(data[i]);
		return true;
	});
	Fake(Method(sendDataToSlavesMock, result));

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(6, 3, 1, 0, 0, 0, 0));
	readRegs.push(REGS(6, 3, 0, 15000, 0, 8, 0));
	readRegs.push(REGS(4, 0x4182, 0xB0E1, 0x4468, 0x26));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	master->setClock(0x12345678);

	// Act
	DeviceDirectoryRow inputDeviceRow = DeviceDirectoryRow(5, 3, DataCollectorDeviceType(true, TimeScale::min10, 7), 14);
	auto name = (byte*)"Meter 001";
	T_MASTER::readAndSendDeviceData_Task task(&T_MASTER::readAndSendDeviceData, master, &inputDeviceRow, 9, name, 15000, 20000);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, REGS(7, 1, 3, 3, 15000, 0, 8, 11 << 8));
	assertPopRegsQueue(writtenRegs, REGS(4, 1, 32770, 0x5678, 0x1234));
	assertPopRegsQueue(writtenRegs, REGS(7, 1, 3, 3, 15000, 0, 8, 11 << 8));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 7, Any<word*>())).Twice();
	Verify(Method(sendDataToSlavesMock, func).Using(15000, 7, TimeScale::min10, 8, Any<byte*>(), Any<byte*>())).Once();
	Verify(Method(deviceNameSentToSlaves, method).Using("Meter 001")).Once();
	Verify(Method(dataByteSentToSlaves, method).Using(0x82) +
		Method(dataByteSentToSlaves, method).Using(0x41) +
		Method(dataByteSentToSlaves, method).Using(0xE1) +
		Method(dataByteSentToSlaves, method).Using(0xB0) +
		Method(dataByteSentToSlaves, method).Using(0x68) +
		Method(dataByteSentToSlaves, method).Using(0x44) +
		Method(dataByteSentToSlaves, method).Using(0x26)).Once();
}

TEST_F_TRAITS(MasterTests, readAndSendDeviceData_Alert_DeviceNotResponding,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	// Arrange
	MOCK_MODBUS;
	MockNewMethod(deviceNameSentToSlaves, string name);

	Mock<IMockedTask<void, uint32_t, byte, TimeScale, word, byte*, byte*>> sendDataToSlavesMock;
	T_MASTER::sendDataToSlaves_Task::mock = &sendDataToSlavesMock.get();
	When(Method(sendDataToSlavesMock, func)).AlwaysDo([&deviceNameSentToSlaves](uint32_t startTime,
		byte dataSize, TimeScale timeScale, word numDataPoints, byte* name, byte* data)
	{
		deviceNameSentToSlaves.get().method(stringifyCharArray(9, (char*)name));
		return true;
	});
	Fake(Method(sendDataToSlavesMock, result));

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(noResponse);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(noResponse);

	RegsQueue readRegs;
	readRegs.push(REGS(6, 3, 0, 15000, 0, 8, 0));
	readRegs.push(REGS(4, 0x4182, 0xB0E1, 0x4468, 0x26));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);
	master->setClock(5);

	// Act
	DeviceDirectoryRow inputDeviceRow = DeviceDirectoryRow(5, 3, DataCollectorDeviceType(true, TimeScale::min10, 7), 14);
	auto name = (byte*)"Meter 001";
	T_MASTER::readAndSendDeviceData_Task task(&T_MASTER::readAndSendDeviceData, master, &inputDeviceRow, 9, name, 15000, 20000);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, REGS(7, 1, 3, 3, 15000, 0, 8, 11 << 8));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 7, Any<word*>())).Once();
	Verify(Method(sendDataToSlavesMock, func).Using(5, 0, TimeScale::ms250, 0, Any<byte*>(), Any<byte*>())).Once();
	Verify(Method(deviceNameSentToSlaves, method).Using("Meter 001")).Once();
}

TEST_F_TRAITS(MasterTests, readAndSendDeviceData_DoNothing_ZeroTimeSpan,
	Type, Unit, Threading, Single, Determinism, Static, Case, Rare)
{
	// Arrange
	MOCK_MODBUS;
	MockNewMethod(deviceNameSentToSlaves, string name);

	Mock<IMockedTask<void, uint32_t, byte, TimeScale, word, byte*, byte*>> sendDataToSlavesMock;
	T_MASTER::sendDataToSlaves_Task::mock = &sendDataToSlavesMock.get();
	When(Method(sendDataToSlavesMock, func)).AlwaysDo([&deviceNameSentToSlaves](uint32_t startTime,
		byte dataSize, TimeScale timeScale, word numDataPoints, byte* name, byte* data)
	{
		deviceNameSentToSlaves.get().method(stringifyCharArray(9, (char*)name));
		return true;
	});
	Fake(Method(sendDataToSlavesMock, result));

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(noResponse);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(noResponse);

	RegsQueue readRegs;
	readRegs.push(REGS(6, 3, 0, 15000, 0, 8, 0));
	readRegs.push(REGS(4, 0x4182, 0xB0E1, 0x4468, 0x26));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);
	master->setClock(5);

	// Act
	DeviceDirectoryRow inputDeviceRow = DeviceDirectoryRow(5, 3, DataCollectorDeviceType(true, TimeScale::min10, 7), 14);
	auto name = (byte*)"Meter 001";
	T_MASTER::readAndSendDeviceData_Task task(&T_MASTER::readAndSendDeviceData, master, &inputDeviceRow, 9, name, 15000, 15000);
	ASSERT_TRUE(task());

	// Assert
	Verify(Method(completeWriteRegsMock, func)).Never();
	Verify(Method(sendDataToSlavesMock, func)).Never();
	Verify(Method(deviceNameSentToSlaves, method)).Never();
}

TEST_F_TRAITS(MasterTests, readAndSendDeviceData_DoNothing_ZeroPoints,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	// Arrange
	MOCK_MODBUS;
	MockNewMethod(deviceNameSentToSlaves, string name);
	MockNewMethod(dataByteSentToSlaves, byte data);

	Mock<IMockedTask<void, uint32_t, byte, TimeScale, word, byte*, byte*>> sendDataToSlavesMock;
	T_MASTER::sendDataToSlaves_Task::mock = &sendDataToSlavesMock.get();
	When(Method(sendDataToSlavesMock, func)).AlwaysDo([&deviceNameSentToSlaves, &dataByteSentToSlaves](uint32_t startTime,
		byte dataSize, TimeScale timeScale, word numDataPoints, byte* name, byte* data)
	{
		deviceNameSentToSlaves.get().method(stringifyCharArray(9, (char*)name));
		for (int i = 0; i < 8; i++)
			dataByteSentToSlaves.get().method(data[i]);
		return true;
	});
	Fake(Method(sendDataToSlavesMock, result));

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(6, 3, 0, 15000, 0, 0, 0));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	// Act
	DeviceDirectoryRow inputDeviceRow = DeviceDirectoryRow(5, 3, DataCollectorDeviceType(true, TimeScale::min10, 7), 14);
	auto name = (byte*)"Meter 001";
	T_MASTER::readAndSendDeviceData_Task task(&T_MASTER::readAndSendDeviceData, master, &inputDeviceRow, 9, name, 15000, 20000);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, REGS(7, 1, 3, 3, 15000, 0, 8, 11 << 8));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 7, Any<word*>())).Once();
	Verify(Method(sendDataToSlavesMock, func)).Never();
	Verify(Method(deviceNameSentToSlaves, method)).Never();
	Verify(Method(dataByteSentToSlaves, method)).Never();
}

TEST_F_TRAITS(MasterTests, sendDataToSlaves_Success_OnePageEach,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	// Arrange
	MOCK_MODBUS;
	When(Method(mockDeviceDirectory, getDeviceNameLength)).AlwaysReturn(7);
	auto devices = Setup_DataCollectorsAndTransmitters();

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(2, 4, 8 << 8));
	readRegs.push(REGS(2, 4, 8 << 8));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	// Act
	auto name = (byte*)"Meter01";
	auto data = tracker.addArray(new byte[5]{ 0x21, 0x88, 0x51, 0x50, 0xAB });
	T_MASTER::sendDataToSlaves_Task task(&T_MASTER::sendDataToSlaves, master, 0x12345678, 5, TimeScale::hr1, 8, name, data);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 1, 7, 0x5678, 0x1234, 5 + (6 << 8), 8), "Meter01"));
	assertPopRegsQueue(writtenRegs, REGS(8, 1, 5, 1, 8 + (5 << 8), 6, 0x8821, 0x5051, 0xAB));
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 2, 7, 0x5678, 0x1234, 5 + (6 << 8), 8), "Meter01"));
	assertPopRegsQueue(writtenRegs, REGS(8, 1, 5, 2, 8 + (5 << 8), 6, 0x8821, 0x5051, 0xAB));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 12, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(5, 0, 8, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(6, 0, 12, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(6, 0, 8, Any<word*>()) +
		Method(completeWriteRegsMock, result)).Once();
}

TEST_F_TRAITS(MasterTests, sendDataToSlaves_Success_TwoAndThreePages,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	// Arrange
	MOCK_MODBUS;
	When(Method(mockDeviceDirectory, getDeviceNameLength)).AlwaysReturn(7);
	auto devices = Setup_DataCollectorsAndTransmitters();

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(2, 4, 4 << 8));
	readRegs.push(REGS(2, 4, 3 << 8));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	// Act
	auto name = (byte*)"Meter01";
	auto data = tracker.addArray(new byte[5]{ 0x21, 0x88, 0x51, 0x50, 0xAB });
	T_MASTER::sendDataToSlaves_Task task(&T_MASTER::sendDataToSlaves, master, 0x12345678, 5, TimeScale::hr1, 8, name, data);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 1, 7, 0x5678, 0x1234, 5 + (6 << 8), 8), "Meter01"));
	assertPopRegsQueue(writtenRegs, REGS(7, 1, 5, 1, 4 + (5 << 8), 6, 0x8821, 0x1));
	assertPopRegsQueue(writtenRegs, REGS(7, 1, 5, 1, 4 + (5 << 8), 6 + (1 << 8), 0xB505, 0xA));
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 2, 7, 0x5678, 0x1234, 5 + (6 << 8), 8), "Meter01"));
	assertPopRegsQueue(writtenRegs, REGS(6, 1, 5, 2, 3 + (5 << 8), 6, 0x821));
	assertPopRegsQueue(writtenRegs, REGS(6, 1, 5, 2, 3 + (5 << 8), 6 + (1 << 8), 0x20A3));
	assertPopRegsQueue(writtenRegs, REGS(6, 1, 5, 2, 2 + (5 << 8), 6 + (2 << 8), 0x2AD));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 12, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(5, 0, 7, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(5, 0, 7, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(6, 0, 12, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(6, 0, 6, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(6, 0, 6, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(6, 0, 6, Any<word*>()) +
		Method(completeWriteRegsMock, result)).Once();
}

TEST_F_TRAITS(MasterTests, sendDataToSlaves_Success_OnePageEach_TimeRequested,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	// Arrange
	MOCK_MODBUS;
	When(Method(mockDeviceDirectory, getDeviceNameLength)).AlwaysReturn(7);
	auto devices = Setup_DataCollectorsAndTransmitters();

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(2, 4, 3)); // Time requested
	readRegs.push(REGS(2, 4, 8 << 8));
	readRegs.push(REGS(2, 4, 8 << 8));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	master->setClock(0x23456781);

	// Act
	auto name = (byte*)"Meter01";
	auto data = tracker.addArray(new byte[5]{ 0x21, 0x88, 0x51, 0x50, 0xAB });
	T_MASTER::sendDataToSlaves_Task task(&T_MASTER::sendDataToSlaves, master, 0x12345678, 5, TimeScale::hr1, 8, name, data);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 1, 7, 0x5678, 0x1234, 5 + (6 << 8), 8), "Meter01"));
	assertPopRegsQueue(writtenRegs, REGS(4, 1, 32770, 0x6781, 0x2345));
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 1, 7, 0x5678, 0x1234, 5 + (6 << 8), 8), "Meter01"));
	assertPopRegsQueue(writtenRegs, REGS(8, 1, 5, 1, 8 + (5 << 8), 6, 0x8821, 0x5051, 0xAB));
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 2, 7, 0x5678, 0x1234, 5 + (6 << 8), 8), "Meter01"));
	assertPopRegsQueue(writtenRegs, REGS(8, 1, 5, 2, 8 + (5 << 8), 6, 0x8821, 0x5051, 0xAB));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 12, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(0, 0, 4, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(5, 0, 12, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(5, 0, 8, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(6, 0, 12, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(6, 0, 8, Any<word*>()) +
		Method(completeWriteRegsMock, result)).Once();
}

TEST_F_TRAITS(MasterTests, sendDataToSlaves_Skip_OnePageEach_DeviceError,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	// Arrange
	MOCK_MODBUS;
	When(Method(mockDeviceDirectory, getDeviceNameLength)).AlwaysReturn(7);
	auto devices = Setup_DataCollectorsAndTransmitters();

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(2, 4, 2)); // Name is too long. Currently all errors are skipped.
	readRegs.push(REGS(2, 4, 8 << 8));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	master->setClock(0x23456781);

	// Act
	auto name = (byte*)"Meter01";
	auto data = tracker.addArray(new byte[5]{ 0x21, 0x88, 0x51, 0x50, 0xAB });
	T_MASTER::sendDataToSlaves_Task task(&T_MASTER::sendDataToSlaves, master, 0x12345678, 5, TimeScale::hr1, 8, name, data);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 1, 7, 0x5678, 0x1234, 5 + (6 << 8), 8), "Meter01"));
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 2, 7, 0x5678, 0x1234, 5 + (6 << 8), 8), "Meter01"));
	assertPopRegsQueue(writtenRegs, REGS(8, 1, 5, 2, 8 + (5 << 8), 6, 0x8821, 0x5051, 0xAB));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 12, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(6, 0, 12, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(6, 0, 8, Any<word*>()) +
		Method(completeWriteRegsMock, result)).Once();
}

TEST_F_TRAITS(MasterTests, sendDataToSlaves_Failure_OnePageEach_Malfunction,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	// Arrange
	MOCK_MODBUS;
	When(Method(mockDeviceDirectory, getDeviceNameLength)).AlwaysReturn(7);
	auto devices = Setup_DataCollectorsAndTransmitters();

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(2, 4, 6)); // Bad response
	readRegs.push(REGS(2, 4, 8 << 8));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	master->setClock(0x23456781);

	// Act
	auto name = (byte*)"Meter01";
	auto data = tracker.addArray(new byte[5]{ 0x21, 0x88, 0x51, 0x50, 0xAB });
	T_MASTER::sendDataToSlaves_Task task(&T_MASTER::sendDataToSlaves, master, 0x12345678, 5, TimeScale::hr1, 8, name, data);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 1, 7, 0x5678, 0x1234, 5 + (6 << 8), 8), "Meter01"));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 12, Any<word*>())).Once();
}

TEST_F_TRAITS(MasterTests, sendDataToSlaves_Failure_NameTooLongForRegisterBuffer,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	// Arrange
	MOCK_MODBUS;
	MOCK_MASTER;
	When(Method(mockDeviceDirectory, getDeviceNameLength)).AlwaysReturn(20);
	Fake(Method(masterMock, reportMalfunction));
	auto devices = Setup_DataCollectorsAndTransmitters();

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(2, 4, 6)); // Bad response
	readRegs.push(REGS(2, 4, 8 << 8));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	master->setClock(0x23456781);

	// Act
	auto name = (byte*)"01234567899876543210";
	auto data = tracker.addArray(new byte[5]{ 0x21, 0x88, 0x51, 0x50, 0xAB });
	T_MASTER::sendDataToSlaves_Task task(&T_MASTER::sendDataToSlaves, master, 0x12345678, 5, TimeScale::hr1, 8, name, data);
	ASSERT_TRUE(task());

	// Assert
	ASSERT_EQ(writtenRegs.size(), 0);
	Verify(Method(completeWriteRegsMock, func)).Never();
	Verify(Method(masterMock, reportMalfunction)).Once();
}

TEST_F_TRAITS(MasterTests, sendDataToSlaves_Success_ReportDeviceNotResponding,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	// Arrange
	MOCK_MODBUS;
	When(Method(mockDeviceDirectory, getDeviceNameLength)).AlwaysReturn(7);
	auto devices = Setup_DataCollectorsAndTransmitters();

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(2, 4, 0));
	readRegs.push(REGS(2, 4, 0));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	// Act
	auto name = (byte*)"Meter01";
	byte* data = nullptr;
	T_MASTER::sendDataToSlaves_Task task(&T_MASTER::sendDataToSlaves, master, 0x12345678, 0, TimeScale::ms250, 0, name, data);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 1, 7, 0x5678, 0x1234, 0, 0), "Meter01"));
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 2, 7, 0x5678, 0x1234, 0, 0), "Meter01"));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 12, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(6, 0, 12, Any<word*>()) +
		Method(completeWriteRegsMock, result)).Once();
}

TEST_F_TRAITS(MasterTests, sendDataToSlaves_Success_FirstDeviceDoesntRespond,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	// Arrange
	MOCK_MODBUS;
	When(Method(mockDeviceDirectory, getDeviceNameLength)).AlwaysReturn(7);
	auto devices = Setup_DataCollectorsAndTransmitters();

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word>> completeReadRegsMock;
	T_MASTER::completeModbusReadRegisters_Task::mock = &completeReadRegsMock.get();
	When(Method(completeReadRegsMock, func)).AlwaysReturn(true);
	When(Method(completeReadRegsMock, result)).Return(noResponse).AlwaysReturn(success);

	Mock<IMockedTask<ModbusRequestStatus, byte, word, word, word*>> completeWriteRegsMock;
	T_MASTER::completeModbusWriteRegisters_Task::mock = &completeWriteRegsMock.get();
	RegsQueue writtenRegs;
	completeWriteRegs_UseRegsQueue(completeWriteRegsMock, writtenRegs);
	When(Method(completeWriteRegsMock, result)).AlwaysReturn(success);

	RegsQueue readRegs;
	readRegs.push(REGS(2, 4, 8 << 8));
	readRegs.push(REGS(2, 4, 8 << 8));
	isRegsResponse_UseMockData(modbusBaseMock, readRegs);

	// Act
	auto name = (byte*)"Meter01";
	auto data = tracker.addArray(new byte[5]{ 0x21, 0x88, 0x51, 0x50, 0xAB });
	T_MASTER::sendDataToSlaves_Task task(&T_MASTER::sendDataToSlaves, master, 0x12345678, 5, TimeScale::hr1, 8, name, data);
	ASSERT_TRUE(task());

	// Assert
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 1, 7, 0x5678, 0x1234, 5 + (6 << 8), 8), "Meter01"));
	assertPopRegsQueue(writtenRegs, withString(REGS(8, 1, 4, 2, 7, 0x5678, 0x1234, 5 + (6 << 8), 8), "Meter01"));
	assertPopRegsQueue(writtenRegs, REGS(8, 1, 5, 2, 8 + (5 << 8), 6, 0x8821, 0x5051, 0xAB));
	Verify(Method(completeWriteRegsMock, func).Using(5, 0, 12, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(6, 0, 12, Any<word*>()) +
		Method(completeWriteRegsMock, result) +
		Method(completeWriteRegsMock, func).Using(6, 0, 8, Any<word*>()) +
		Method(completeWriteRegsMock, result)).Once();
}