#include "pch.h"
#include "fakeit.hpp"
#include <stack>

#include "../kwh-modbus/libraries/modbus/Modbus.h"
#include "../kwh-modbus/noArduino/ModbusMemory.h"
#include "../kwh-modbus/libraries/modbus/ModbusSerial.hpp"
#include "../kwh-modbus/interfaces/ISerialStream.h"
#include "../kwh-modbus/noArduino/SystemFunctions.h"
#include "../kwh-modbus/mock/MockSerialStream.h"
#include "../kwh-modbus/libraries/modbusSlave/ModbusSlave.hpp"
#include "../kwh-modbus/libraries/modbusMaster/ModbusMaster.hpp"
#include "../kwh-modbus/libraries/resilientModbusMaster/ResilientModbusMaster.hpp"
#include "../kwh-modbus/libraries/deviceDirectory/DeviceDirectory.hpp"
#include "../kwh-modbus/libraries/slave/Slave.hpp"
#include "../kwh-modbus/libraries/master/Master.hpp"

#include "test_helpers.h"
#include "WindowsFunctions.h"
#include "WindowsSystemFunctions.h"
#include <queue>

#define TIMEOUT_START(MILLIS_MAX) unsigned long time_start = system->millis(); \
unsigned long time_max = MILLIS_MAX

#define TIMEOUT_CHECK if (system->millis() - time_start > time_max) return
#define TIMEOUT_CHECK_RETURN(RETURN_VALUE) if (system->millis() - time_start > time_max) return RETURN_VALUE

using namespace fakeit;

enum ModbusTransmissionError
{
	None = 0,
	InboundError = 1,
	OutboundError = 2,
	InboundDelays = 4,
	OutboundDelays = 8
};

inline ModbusTransmissionError operator~ (ModbusTransmissionError a) { return (ModbusTransmissionError)~(int)a; }
inline ModbusTransmissionError operator| (ModbusTransmissionError a, ModbusTransmissionError b) { return (ModbusTransmissionError)((int)a | (int)b); }
inline ModbusTransmissionError operator& (ModbusTransmissionError a, ModbusTransmissionError b) { return (ModbusTransmissionError)((int)a & (int)b); }
inline ModbusTransmissionError operator^ (ModbusTransmissionError a, ModbusTransmissionError b) { return (ModbusTransmissionError)((int)a ^ (int)b); }
inline ModbusTransmissionError& operator|= (ModbusTransmissionError& a, ModbusTransmissionError b) { return (ModbusTransmissionError&)((int&)a |= (int)b); }
inline ModbusTransmissionError& operator&= (ModbusTransmissionError& a, ModbusTransmissionError b) { return (ModbusTransmissionError&)((int&)a &= (int)b); }
inline ModbusTransmissionError& operator^= (ModbusTransmissionError& a, ModbusTransmissionError b) { return (ModbusTransmissionError&)((int&)a ^= (int)b); }
inline bool contains(ModbusTransmissionError a, ModbusTransmissionError b)
{
	return (a | b) == a;
}

typedef ModbusSlave<ISerialStream, ISystemFunctions, ModbusMemory> T_ModbusSlave;
typedef ResilientModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory> T_ModbusMaster;
typedef Slave<T_ModbusSlave, WindowsSystemFunctions> T_Slave;
typedef Master<T_ModbusMaster, WindowsSystemFunctions, DeviceDirectory<byte*>> T_Master;
class MasterSlaveIntegrationTests :
	public ::testing::Test,
	public ::testing::WithParamInterface<ModbusTransmissionError>
{
private:
	class _SetTestConditionsTask : public ITask
	{
	private:
		bool _isRun = false;
		MasterSlaveIntegrationTests *_fixture;

	public:
		_SetTestConditionsTask(MasterSlaveIntegrationTests *fixture)
		{
			_fixture = fixture;
		}

		bool operator()()
		{
			_isRun = true;
			_fixture->setTestConditions();
			return true;
		}

		bool completed()
		{
			return _isRun;
		}
	};

protected:
	T_ModbusSlave *modbusSlave = new T_ModbusSlave();
	T_ModbusMaster *modbusMaster = new T_ModbusMaster();
	queue<byte> *slaveIn;
	queue<byte> *masterIn;
	MockSerialStream *slaveSerial;
	MockSerialStream *masterSerial;
	DeviceDirectory<byte*> *deviceDirectory;
	T_Slave *slave = new T_Slave();
	T_Master *master = new T_Master();

	bool masterSuccess = false;
	bool slaveSuccess = false;
	ModbusTransmissionError errorType;

	static WindowsSystemFunctions *system;

	Mock<Device> device0;
	Mock<Device> device1;

	function<bool()> slaveAction;
	function<bool()> masterAction;

	bool runTaskStack(stack<ITask*> &tasks)
	{
		if ((*tasks.top())())
		{
			tasks.pop();
			return (tasks.empty());
		}
		return false;
	}

public:
	void SetUp()
	{
		errorType = GetParam();

		system = new WindowsSystemFunctions();
		slaveIn = new queue<byte>();
		masterIn = new queue<byte>();
		slaveSerial = new MockSerialStream(slaveIn, masterIn);
		masterSerial = new MockSerialStream(masterIn, slaveIn);
		slaveSerial->setSystem(system);
		masterSerial->setSystem(system);
		deviceDirectory = new DeviceDirectory<byte*>();

		modbusMaster->config(masterSerial, system, 4800);
		modbusSlave->config(slaveSerial, system, 4800);
		modbusSlave->addReg(0, 0);
		modbusSlave->addReg(1, 0);
		modbusSlave->addReg(2, 0);
		modbusSlave->addReg(3, 0);
		modbusSlave->addReg(4, 0);
		modbusSlave->addReg(5, 0);
		modbusSlave->addReg(6, 0);
		modbusSlave->addReg(7, 0);
		modbusSlave->addReg(8, 0);
		modbusSlave->addReg(9, 0);
		modbusSlave->addReg(10, 0);
		modbusSlave->addReg(11, 0);
		modbusSlave->addReg(12, 0);
		master->config(system, modbusMaster, deviceDirectory, 40);
		slave->config(system, modbusSlave);
		deviceDirectory->init(5, 20);
		seedRandom(masterSerial);
		seedRandom(slaveSerial);
		modbusMaster->setMaxTimePerTryMicros(200000);
		modbusMaster->setMaxTimeMicros(3500000);

		When(Method(device0, getType)).AlwaysReturn(1);
		When(Method(device1, getType)).AlwaysReturn(2);
		Device* devices[2];
		devices[0] = &device0.get();
		devices[1] = &device1.get();
		byte* names[2];
		names[0] = (byte*)"dev00";
		names[1] = (byte*)"dev01";

		slave->init(2, 5, 13, 20, devices, names);
	}

	void seedRandom(MockSerialStream *stream)
	{
		WindowsFunctions win;
		uint8_t seed[16];
		bool success = win.Windows_CryptGenRandom(16, seed);
		if (success)
			stream->randomSeed(16, seed);
		else
			stream->randomSeed(time(NULL), time(NULL), time(NULL), time(NULL));
	}

	void TearDown()
	{
		delete slaveIn;
		delete masterIn;
		delete slaveSerial;
		delete masterSerial;
	}

	void setTestConditions()
	{
		if (contains(errorType, InboundError))
		{
			masterSerial->setPerBitErrorProb(.020);
		}

		if (contains(errorType, OutboundError))
		{
			slaveSerial->setPerBitErrorProb(0.010);
		}

		if (contains(errorType, InboundDelays))
		{
			masterSerial->setReadDelays(3000, 1000);
		}

		if (contains(errorType, OutboundDelays))
		{
			slaveSerial->setReadDelays(3000, 1000);
		}
	}

	ITask* getNewSetTestConditionsTask()
	{
		return new _SetTestConditionsTask(this);
	}

	void slaveThread()
	{
		this->slaveSuccess = false;
		bool processed;
		bool broadcast;
		TIMEOUT_START(5000);
		while (!this->slaveAction())
		{
			TIMEOUT_CHECK;
		}
		this->slaveSuccess = true;
	}

	void masterThread()
	{
		this->masterSuccess = false;
		TIMEOUT_START(5000);
		while (!this->masterAction())
		{
			TIMEOUT_CHECK;
		}
		this->masterSuccess = true;
	}

	static void slave_thread(void *param)
	{
		((MasterSlaveIntegrationTests*)param)->slaveThread();
	}

	static void master_thread(void *param)
	{
		((MasterSlaveIntegrationTests*)param)->masterThread();
	}
};

WindowsSystemFunctions *MasterSlaveIntegrationTests::system;

TEST_P(MasterSlaveIntegrationTests, MasterSlaveIntegrationTests_checkForNewSlaves_found)
{
	T_Master::checkForNewSlaves_Task task(&T_Master::checkForNewSlaves, master);
	slaveAction = [this]() {
		slave->loop();
		return masterSuccess;
	};
	masterAction = [this, &task]()
	{
		return task();
	};

	modbusSlave->setSlaveId(1);
	slave->setOutgoingState();
	setTestConditions();

	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	word count;
	word *regs;
	ASSERT_TRUE(modbusMaster->isReadRegsResponse(count, regs));
	ASSERT_EQ(count, 8);
	assertArrayEq<word, byte, byte, word, word, word, word, word, word>(regs,
		sIdle, 0, 1, 2, 5, 13, 0, 0, 0);
	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);
	ASSERT_EQ(task.result(), found);
}

TEST_P(MasterSlaveIntegrationTests, MasterSlaveIntegrationTests_checkForNewSlaves_notFound)
{
	T_Master::checkForNewSlaves_Task task(&T_Master::checkForNewSlaves, master);
	slaveAction = [this]() {
		//return true;
		slave->loop();
		return masterSuccess;
	};
	masterAction = [this, &task]()
	{
		return task();
	};

	modbusSlave->setSlaveId(2);
	slave->setOutgoingState();
	setTestConditions();

	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);
	ASSERT_EQ(task.result(), notFound);
}

TEST_P(MasterSlaveIntegrationTests, MasterSlaveIntegrationTests_processNewSlave)
{
	T_Master::checkForNewSlaves_Task task0(&T_Master::checkForNewSlaves, master);
	ITask* task1 = getNewSetTestConditionsTask();
	T_Master::processNewSlave_Task task2(&T_Master::processNewSlave, master, false);

	stack<ITask*> tasks;
	tasks.push(&task2);
	tasks.push(task1);
	tasks.push(&task0);

	slaveAction = [this]() {
		slave->loop();
		return masterSuccess;
	};
	masterAction = [this, &tasks]()
	{
		return runTaskStack(tasks);
	};

	modbusSlave->setSlaveId(1);
	slave->setOutgoingState();

	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	delete task1;

	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);

	ASSERT_EQ(modbusSlave->getSlaveId(), 2);

	int row;
	auto device = deviceDirectory->findDeviceForName((byte*)"dev00", row);
	ASSERT_NE(device, nullptr);
	ASSERT_EQ(*device, DeviceDirectoryRow(2, 0, 1, 13));
	ASSERT_EQ(row, 0);

	device = deviceDirectory->findDeviceForName((byte*)"dev01", row);
	ASSERT_NE(device, nullptr);
	ASSERT_EQ(*device, DeviceDirectoryRow(2, 1, 2, 13));
	ASSERT_EQ(row, 1);
}

TEST_P(MasterSlaveIntegrationTests, MasterSlaveIntegrationTests_processNewSlave_JustReject)
{
	T_Master::checkForNewSlaves_Task task0(&T_Master::checkForNewSlaves, master);
	ITask* task1 = getNewSetTestConditionsTask();
	T_Master::processNewSlave_Task task2(&T_Master::processNewSlave, master, true);

	stack<ITask*> tasks;
	tasks.push(&task2);
	tasks.push(task1);
	tasks.push(&task0);

	slaveAction = [this]() {
		slave->loop();
		return masterSuccess;
	};
	masterAction = [this, &tasks]()
	{
		return runTaskStack(tasks);
	};

	modbusSlave->setSlaveId(1);
	slave->setOutgoingState();

	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	delete task1;

	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);

	ASSERT_EQ(modbusSlave->getSlaveId(), 255);
}

TEST_P(MasterSlaveIntegrationTests, MasterSlaveIntegrationTests_broadcastTime)
{
	slaveAction = [this]() {
		slave->loop();
		return masterSuccess;
	};
	masterAction = [this]()
	{
		while (!master->broadcastTime());
		system->delay(100);
		return slave->getClock() >= 2000000000;
	};

	modbusSlave->setSlaveId(1);
	slave->setOutgoingState();
	setTestConditions();

	master->setClock(2000000000);
	Fake(Method(device0, setClock));
	Fake(Method(device1, setClock));

	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);
	Verify(Method(device0, setClock).Using(2000000000)).Once();
	Verify(Method(device1, setClock).Using(2000000000)).Once();
}

TEST_P(MasterSlaveIntegrationTests, MasterSlaveIntegrationTests_readDataFromSlave)
{
	MockNewMethod(mockReadData, uint32_t startTime, word numPoints, byte page, word bufferSize, byte maxPoints);
	bool slaveComplete = false;

	Fake(Method(device0, setClock));
	Fake(Method(device1, setClock));
	When(Method(device1, readData)).AlwaysDo([&mockReadData, &slaveComplete] (uint32_t startTime, word numPoints, byte page,
		byte* buffer, word bufferSize, byte maxPoints, byte &outDataPointsCount, byte &outPagesRemaining, byte &outDataPointSize) {
		mockReadData.get().method(startTime, numPoints, page, bufferSize, maxPoints);
		slaveComplete = true;
		outDataPointsCount = 4;
		outPagesRemaining = 1 - page;
		outDataPointSize = 8;
		buffer[0] = 0 + 4 * page;
		buffer[1] = 1 + 4 * page;
		buffer[2] = 2 + 4 * page;
		buffer[3] = 3 + 4 * page;
		return true;
	});

	T_Master::checkForNewSlaves_Task task0(&T_Master::checkForNewSlaves, master);
	T_Master::processNewSlave_Task task1(&T_Master::processNewSlave, master, false);
	ITask* task2 = getNewSetTestConditionsTask();
	T_Master::readAndSendDeviceData_Task task3(&T_Master::readAndSendDeviceData, master, TimeScale::sec1, 10);

	stack<ITask*> tasks;
	tasks.push(&task3);
	tasks.push(task2);
	tasks.push(&task1);
	tasks.push(&task0);

	slaveAction = [this, &slaveComplete]() {
		slave->loop();
		return slaveComplete;
	};
	masterAction = [this, &tasks]()
	{
		return runTaskStack(tasks);
	};

	modbusSlave->setSlaveId(1);
	slave->setClock(2);
	slave->setOutgoingState();
	master->setClock(2);
	master->_curTime = 10000;
	master->lastUpdateTimes[1] = 2;

	word devType;
	DataCollectorDevice::getDataCollectorDeviceTypeFromParameters(false, TimeScale::sec1, 8, devType);
	When(Method(device1, getType)).AlwaysReturn(devType);

	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	delete task2;

	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);
	Verify(Method(mockReadData, method).Using(2, 8, 0, 20, 40)).Once();
}

INSTANTIATE_TEST_CASE_P(NoErrors, MasterSlaveIntegrationTests, ::testing::Values(None));
INSTANTIATE_TEST_CASE_P(InboundError, MasterSlaveIntegrationTests, ::testing::Values(InboundError));
INSTANTIATE_TEST_CASE_P(OutboundError, MasterSlaveIntegrationTests, ::testing::Values(OutboundError));
INSTANTIATE_TEST_CASE_P(InboundDelays, MasterSlaveIntegrationTests, ::testing::Values(InboundDelays));
INSTANTIATE_TEST_CASE_P(OutboundDelays, MasterSlaveIntegrationTests, ::testing::Values(OutboundDelays));
INSTANTIATE_TEST_CASE_P(AllErrors, MasterSlaveIntegrationTests, ::testing::Values(InboundError & OutboundError));
INSTANTIATE_TEST_CASE_P(AllDelays, MasterSlaveIntegrationTests, ::testing::Values(InboundDelays & OutboundDelays));
INSTANTIATE_TEST_CASE_P(AllDelaysAndErrors, MasterSlaveIntegrationTests, ::testing::Values(InboundError & OutboundError & InboundDelays & OutboundDelays));