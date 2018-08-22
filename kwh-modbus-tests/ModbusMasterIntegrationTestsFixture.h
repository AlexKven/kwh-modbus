#pragma once
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

template<typename TMaster, typename TSlave>
class ModbusMasterIntegrationTestsFixture
{
protected:
	TSlave *slave = new TSlave;
	TMaster *master = new TMaster;
	queue<byte> *slaveIn;
	queue<byte> *masterIn;
	MockSerialStream *slaveSerial;
	MockSerialStream *masterSerial;
	bool masterSuccess = false;
	bool slaveSuccess = false;

	static WindowsSystemFunctions *system;

public:
	ModbusMasterIntegrationTestsFixture() {}
	~ModbusMasterIntegrationTestsFixture() {}

	virtual void SetUp()
	{
		system = new WindowsSystemFunctions();
		slaveIn = new queue<byte>();
		masterIn = new queue<byte>();
		slaveSerial = new MockSerialStream(slaveIn, masterIn);
		masterSerial = new MockSerialStream(masterIn, slaveIn);

		slave->config(slaveSerial, system, 1200);
		master->config(masterSerial, system, 1200);
	}

	virtual void TearDown()
	{
		delete slaveIn;
		delete masterIn;
		delete slaveSerial;
		delete masterSerial;
	}

	virtual void slaveThread()
	{
		this->slaveSuccess = false;
		TIMEOUT_START(5000);
		while (!this->slave->task())
			TIMEOUT_CHECK;
		this->slaveSuccess = true;
	}

	virtual void masterThread()
	{
		this->masterSuccess = false;
		TIMEOUT_START(5000);

		this->master->send();
		while (!this->master->receive())
			TIMEOUT_CHECK;
		this->masterSuccess = true;
	}
};

