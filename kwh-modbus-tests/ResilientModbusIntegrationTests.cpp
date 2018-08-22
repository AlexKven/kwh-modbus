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
#include "../kwh-modbus/libraries/resilientModbus/ResilientModbus.hpp"
#include "test_helpers.h"
#include "WindowsFunctions.h"
#include "WindowsSystemFunctions.h"
#include "ModbusIntegrationTests.cpp"
#include <queue>

using namespace fakeit;

class ResilientModbusIntegrationTests : public ModbusIntegrationTests
{
protected:
	ResilientModbus<ModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory>, ISystemFunctions> *resilientMaster = new ResilientModbus<ModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory>, ISystemFunctions>();
	static WindowsSystemFunctions *system;

public:
	virtual void SetUp()
	{
		ModbusIntegrationTests::SetUp();

		delete master;
		master = (ModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory>*)resilientMaster;

		master->config(masterSerial, system, 1200);

		seedRandom(slaveSerial);
		seedRandom(masterSerial);
		masterSerial->setPerBitErrorProb(.05);
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
		while (this->resilientMaster->getStatus() < 4)
		{
			this->slave->task();
			TIMEOUT_CHECK;
		}
		this->slaveSuccess = true;
	}

	virtual void masterThread()
	{
		this->masterSuccess = false;
		TIMEOUT_START(5000);

		while (!this->resilientMaster->work())
			TIMEOUT_CHECK;
		this->masterSuccess = true;
	}
};