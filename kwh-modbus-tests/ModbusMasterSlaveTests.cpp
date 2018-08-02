#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/Modbus.h"
#include "../kwh-modbus/noArduino/ModbusMemory.h"
#include "../kwh-modbus/libraries/modbus/ModbusSerial.hpp"
#include "../kwh-modbus/interfaces/ISerialStream.h"
#include "../kwh-modbus/noArduino/SystemFunctions.h"
#include "../kwh-modbus/mock/MockSerialStream.h"
#include "../kwh-modbus/libraries/modbusSlave/ModbusSlave.hpp"
#include "../kwh-modbus/libraries/modbusMaster/ModbusMaster.hpp"
#include "test_helpers.h"
#include "WindowsFunctions.h"
#include "WindowsSystemFunctions.h"
#include <queue>

#define TIMEOUT_START(MILLIS_MAX) unsigned long time_start = system->millis(); \
unsigned long time_max = MILLIS_MAX

#define TIMEOUT_CHECK if (system->millis() - time_start > time_max) return
#define TIMEOUT_CHECK_RETURN(RETURN_VALUE) if (system->millis() - time_start > time_max) return RETURN_VALUE

using namespace fakeit;

class ModbusMasterSlaveTests : public ::testing::Test
{
protected:
	ModbusSlave<ISerialStream, ISystemFunctions, ModbusMemory> *slave = new ModbusSlave<ISerialStream, ISystemFunctions, ModbusMemory>();
	ModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory> *master = new ModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory>();
	queue<byte> *slaveIn;
	queue<byte> *masterIn;
	MockSerialStream *slaveSerial;
	MockSerialStream *masterSerial;

public:
	static WindowsSystemFunctions *system;

	void SetUp()
	{
		system = new WindowsSystemFunctions();
		slaveIn = new queue<byte>();
		masterIn = new queue<byte>();
		slaveSerial = new MockSerialStream(slaveIn, masterIn);
		masterSerial = new MockSerialStream(masterIn, slaveIn);
		slave->config(slaveSerial, system, 1200);
		slave->setSlaveId(23);
		master->config(masterSerial, system, 1200);
	}

	void TearDown()
	{
		delete slaveIn;
		delete masterIn;
		delete slaveSerial;
		delete masterSerial;
	}

	static void slave_thread(void *param)
	{
		TIMEOUT_START(2000);
		ModbusMasterSlaveTests* fixture = (ModbusMasterSlaveTests*)param;
		fixture->slave->addHreg(3, 703);
		fixture->slave->addHreg(4, 513);
		while (!fixture->slave->task())
			TIMEOUT_CHECK;
	}

	static void master_thread(void *param)
	{
		TIMEOUT_START(2000);
		ModbusMasterSlaveTests* fixture = (ModbusMasterSlaveTests*)param;
		fixture->master->resetFrame(6);
		auto frame = fixture->master->getFramePtr();
		frame[0] = 23;
		frame[1] = MB_FC_READ_REGS;
		frame[2] = 0;
		frame[3] = 3;
		frame[4] = 0;
		frame[5] = 2;

		fixture->master->send();
		while (!fixture->master->receive())
			TIMEOUT_CHECK;

		frame = fixture->master->getFramePtr();
		word frameLength = fixture->master->getFrameLength();
	}
};

WindowsSystemFunctions *ModbusMasterSlaveTests::system;

TEST_F(ModbusMasterSlaveTests, ModbusSlave)
{

}

TEST_F(ModbusMasterSlaveTests, ModbusMasterSlaveTests_FullTest)
{
	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);
}