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

class ModbusIntegrationTests : public ::testing::Test
{
protected:
	ModbusSlave<ISerialStream, ISystemFunctions, ModbusMemory> *slave = new ModbusSlave<ISerialStream, ISystemFunctions, ModbusMemory>();
	ModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory> *master = new ModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory>();
	queue<byte> *slaveIn;
	queue<byte> *masterIn;
	MockSerialStream *slaveSerial;
	MockSerialStream *masterSerial;
	bool masterSuccess = false;
	bool slaveSuccess = false;

	static WindowsSystemFunctions *system;

public:
	void SetUp()
	{
		system = new WindowsSystemFunctions();
		slaveIn = new queue<byte>();
		masterIn = new queue<byte>();
		slaveSerial = new MockSerialStream(slaveIn, masterIn);
		masterSerial = new MockSerialStream(masterIn, slaveIn);
		slave->config(slaveSerial, system, 1200);
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
		ModbusIntegrationTests* fixture = (ModbusIntegrationTests*)param;
		fixture->slaveSuccess = false;
		TIMEOUT_START(2000);
		while (!fixture->slave->task())
			TIMEOUT_CHECK;
		fixture->slaveSuccess = true;
	}

	static void master_thread(void *param)
	{
		ModbusIntegrationTests* fixture = (ModbusIntegrationTests*)param;
		fixture->masterSuccess = false;
		TIMEOUT_START(2000);

		fixture->master->send();
		while (!fixture->master->receive())
			TIMEOUT_CHECK;
		fixture->masterSuccess = true;
	}
};

WindowsSystemFunctions *ModbusIntegrationTests::system;

TEST_F(ModbusIntegrationTests, ModbusIntegrationTests_ReadRegs)
{
	// Set slave
	slave->setSlaveId(23);
	slave->addHreg(3, 703);
	slave->addHreg(4, 513);

	// Set master
	master->setRequest_ReadRegisters(23, 3, 2);

	// Start both master and slave
	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);

	bool integrity = master->verifyResponseIntegrity();
	word regCount;
	word* regPtr;
	bool isReadRegs = master->isReadRegsResponse(regCount, regPtr);
	assertArrayEq<word, word>(regPtr, 703, 513);
}

TEST_F(ModbusIntegrationTests, ModbusIntegrationTests_ReadRegs_Success)
{
	// Set slave
	slave->setSlaveId(23);
	slave->addHreg(3, 703);
	slave->addHreg(4, 513);

	// Set master
	master->setRequest_ReadRegisters(23, 3, 2);

	// Start both master and slave
	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);

	bool integrity = master->verifyResponseIntegrity();
	word regCount;
	word* regPtr;
	bool isReadRegs = master->isReadRegsResponse(regCount, regPtr);
	ASSERT_EQ(regCount, 2);
	assertArrayEq<word, word>(regPtr, 703, 513);
}

TEST_F(ModbusIntegrationTests, ModbusIntegrationTests_ReadRegs_Failure)
{
	// Set slave
	slave->setSlaveId(23);
	slave->addHreg(3, 703);
	slave->addHreg(4, 513);

	// Set master
	master->setRequest_ReadRegisters(23, 3, 3);

	// Start both master and slave
	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);

	bool integrity = master->verifyResponseIntegrity();
	byte fcode;
	byte excode;
	bool isReadRegs = master->isExceptionResponse(fcode, excode);
	ASSERT_EQ(fcode, MB_FC_READ_REGS);
	ASSERT_EQ(excode, MB_EX_ILLEGAL_ADDRESS);
}