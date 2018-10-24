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

	bool runTaskStack(stack<IAsyncTask*> &tasks)
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
		master->config(system, modbusMaster, deviceDirectory);
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

		slave->init(2, 5, devices, names);

		if (contains(errorType, InboundError))
		{
			masterSerial->setPerBitErrorProb(.020);
		}

		if (contains(errorType, OutboundError))
		{
			slaveSerial->setPerBitErrorProb(0.012);
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

	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	word count;
	word *regs;
	ASSERT_TRUE(modbusMaster->isReadRegsResponse(count, regs));
	ASSERT_EQ(count, 7);
	assertArrayEq<word, byte, byte, word, word, word, word, word>(regs,
		sIdle, 0, 1, 2, 5, 0, 0, 0);
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
	T_Master::processNewSlave_Task task1(&T_Master::processNewSlave, master, false);

	stack<IAsyncTask*> tasks;
	tasks.push(&task1);
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

	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);

	ASSERT_EQ(modbusSlave->getSlaveId(), 2);

	word tpe;
	byte slv;
	int row;
	ASSERT_TRUE(deviceDirectory->findDeviceForName((byte*)"dev00", tpe, slv, row));
	ASSERT_EQ(tpe, 1);
	ASSERT_EQ(slv, 2);
	ASSERT_EQ(row, 0);
	ASSERT_TRUE(deviceDirectory->findDeviceForName((byte*)"dev01", tpe, slv, row));
	ASSERT_EQ(tpe, 2);
	ASSERT_EQ(slv, 2);
	ASSERT_EQ(row, 1);
}

TEST_P(MasterSlaveIntegrationTests, MasterSlaveIntegrationTests_processNewSlave_JustReject)
{
	T_Master::checkForNewSlaves_Task task0(&T_Master::checkForNewSlaves, master);
	T_Master::processNewSlave_Task task1(&T_Master::processNewSlave, master, true);

	stack<IAsyncTask*> tasks;
	tasks.push(&task1);
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

	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);

	ASSERT_EQ(modbusSlave->getSlaveId(), 255);
}

INSTANTIATE_TEST_CASE_P(NoErrors, MasterSlaveIntegrationTests, ::testing::Values(None));
INSTANTIATE_TEST_CASE_P(InboundError, MasterSlaveIntegrationTests, ::testing::Values(InboundError));
INSTANTIATE_TEST_CASE_P(OutboundError, MasterSlaveIntegrationTests, ::testing::Values(OutboundError));
INSTANTIATE_TEST_CASE_P(InboundDelays, MasterSlaveIntegrationTests, ::testing::Values(InboundDelays));
INSTANTIATE_TEST_CASE_P(OutboundDelays, MasterSlaveIntegrationTests, ::testing::Values(OutboundDelays));
INSTANTIATE_TEST_CASE_P(AllErrors, MasterSlaveIntegrationTests, ::testing::Values(InboundError & OutboundError));
INSTANTIATE_TEST_CASE_P(AllDelays, MasterSlaveIntegrationTests, ::testing::Values(InboundDelays & OutboundDelays));
INSTANTIATE_TEST_CASE_P(AllDelaysAndErrors, MasterSlaveIntegrationTests, ::testing::Values(InboundError & OutboundError & InboundDelays & OutboundDelays));