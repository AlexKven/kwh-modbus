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

		modbusMaster->config(masterSerial, system, 1200);
		modbusSlave->config(slaveSerial, system, 1200);
		seedRandom(masterSerial);
		seedRandom(slaveSerial);

		if (contains(errorType, InboundError))
		{
			masterSerial->setPerBitErrorProb(.020);
		}

		if (contains(errorType, OutboundError))
		{
			slaveSerial->setPerBitErrorProb(0.012);
			modbusMaster->setMaxTimePerTryMicros(100000);
		}

		if (contains(errorType, InboundDelays))
		{
			masterSerial->setReadDelays(7000, 2000);
		}

		if (contains(errorType, OutboundDelays))
		{
			slaveSerial->setReadDelays(7000, 2000);
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
		TIMEOUT_START(3000);
		if (this->errorType == 0)
		{
			while (!this->modbusSlave->task(processed, broadcast))
				TIMEOUT_CHECK;
		}
		else
		{
			while (this->modbusMaster->getStatus() < 4)
			{
				this->modbusSlave->task(processed, broadcast);
				TIMEOUT_CHECK;
			}
		}
		this->slaveSuccess = true;
	}

	void masterThread()
	{
		this->masterSuccess = false;
		TIMEOUT_START(3000);

		if (this->errorType == 0)
		{
			this->modbusMaster->send();
			while (!this->modbusMaster->receive())
				TIMEOUT_CHECK;
		}
		else
		{
			while (!this->modbusMaster->work())
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