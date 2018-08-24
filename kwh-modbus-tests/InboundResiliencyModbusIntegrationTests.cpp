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
	return a | b == a;
}

class InboundResiliencyModbusIntegrationTests :
	public ::testing::Test,
	public ::testing::WithParamInterface<ModbusTransmissionError>
{
protected:
	ModbusSlave<ISerialStream, ISystemFunctions, ModbusMemory> *slave = new ModbusSlave<ISerialStream, ISystemFunctions, ModbusMemory>();
	ResilientModbus<ModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory>, ISystemFunctions> *master = new ResilientModbus<ModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory>, ISystemFunctions>();
	queue<byte> *slaveIn;
	queue<byte> *masterIn;
	MockSerialStream *slaveSerial;
	MockSerialStream *masterSerial;
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
		
		master->config(masterSerial, system, 1200);
		slave->config(slaveSerial, system, 1200);

		if (contains(errorType, InboundError))
		{
			seedRandom(masterSerial);
			masterSerial->setPerBitErrorProb(.05);
		}

		if (contains(errorType, OutboundError))
		{
			seedRandom(slaveSerial);
			masterSerial->setPerBitErrorProb(.02);
			master->setMaxTimePerTryMicros(100000);
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
		TIMEOUT_START(50000);
		if (this->errorType == 0)
		{
			while (!this->slave->task())
				TIMEOUT_CHECK;
		}
		else
		{
			while (this->master->getStatus() < 4)
			{
				this->slave->task();
				TIMEOUT_CHECK;
			}
		}
		this->slaveSuccess = true;
	}

	void masterThread()
	{
		this->masterSuccess = false;
		TIMEOUT_START(50000);

		if (this->errorType == 0)
		{
			this->master->send();
			while (!this->master->receive())
				TIMEOUT_CHECK;
		}
		else
		{
			while (!this->master->work())
				TIMEOUT_CHECK;
		}
		this->masterSuccess = true;
	}

	static void slave_thread(void *param)
	{
		((InboundResiliencyModbusIntegrationTests*)param)->slaveThread();
	}

	static void master_thread(void *param)
	{
		((InboundResiliencyModbusIntegrationTests*)param)->masterThread();
	}
};

WindowsSystemFunctions *InboundResiliencyModbusIntegrationTests::system;

TEST_P(InboundResiliencyModbusIntegrationTests, InboundResiliencyModbusIntegrationTests_ReadRegs_Success)
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
	ASSERT_TRUE(integrity);
	ASSERT_TRUE(isReadRegs);
	ASSERT_EQ(regCount, 2);
	assertArrayEq<word, word>(regPtr, 703, 513);
}

TEST_P(InboundResiliencyModbusIntegrationTests, InboundResiliencyModbusIntegrationTests_ReadRegs_Failure)
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
	bool isException = master->isExceptionResponse(fcode, excode);
	ASSERT_TRUE(integrity);
	ASSERT_TRUE(isException);
	ASSERT_EQ(fcode, MB_FC_READ_REGS);
	ASSERT_EQ(excode, MB_EX_ILLEGAL_ADDRESS);
}

TEST_P(InboundResiliencyModbusIntegrationTests, InboundResiliencyModbusIntegrationTests_WriteReg_Success)
{
	// Set slave
	slave->setSlaveId(23);
	slave->addHreg(3, 0);

	// Set master
	master->setRequest_WriteRegister(23, 3, 703);

	// Start both master and slave
	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);

	bool integrity = master->verifyResponseIntegrity();
	bool isWriteReg = master->isWriteRegResponse();
	word reg = slave->Hreg(3);
	ASSERT_TRUE(integrity);
	ASSERT_TRUE(isWriteReg);
	ASSERT_EQ(reg, 703);
}

TEST_P(InboundResiliencyModbusIntegrationTests, InboundResiliencyModbusIntegrationTests_WriteReg_Failure)
{
	// Set slave
	slave->setSlaveId(23);
	slave->addHreg(3, 0);

	// Set master
	master->setRequest_WriteRegister(23, 4, 703);

	// Start both master and slave
	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);

	bool integrity = master->verifyResponseIntegrity();
	byte fcode;
	byte excode;
	bool isException = master->isExceptionResponse(fcode, excode);
	ASSERT_TRUE(integrity);
	ASSERT_TRUE(isException);
	ASSERT_EQ(fcode, MB_FC_WRITE_REG);
	ASSERT_EQ(excode, MB_EX_ILLEGAL_ADDRESS);
}

TEST_P(InboundResiliencyModbusIntegrationTests, InboundResiliencyModbusIntegrationTests_WriteRegs_Success)
{
	// Set slave
	slave->setSlaveId(23);
	slave->addHreg(3, 0);
	slave->addHreg(4, 0);
	slave->addHreg(5, 0);

	// Create register array
	word *regs = new word[3];
	setArray<word, word, word>(regs, 701, 702, 703);

	// Set master
	master->setRequest_WriteRegisters(23, 3, 3, regs);

	// Start both master and slave
	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	ASSERT_TRUE(slaveSuccess);
	ASSERT_TRUE(masterSuccess);

	bool integrity = master->verifyResponseIntegrity();
	bool isWriteRegs = master->isWriteRegsResponse();
	word reg3 = slave->Hreg(3);
	word reg4 = slave->Hreg(4);
	word reg5 = slave->Hreg(5);
	ASSERT_TRUE(integrity);
	ASSERT_TRUE(isWriteRegs);
	ASSERT_EQ(reg3, 701);
	ASSERT_EQ(reg4, 702);
	ASSERT_EQ(reg5, 703);
}

TEST_P(InboundResiliencyModbusIntegrationTests, InboundResiliencyModbusIntegrationTests_WriteRegs_Failure)
{
	// Set slave
	slave->setSlaveId(23);
	slave->addHreg(3, 0);
	slave->addHreg(4, 0);
	slave->addHreg(5, 0);

	// Create register array
	word *regs = new word[4];
	setArray<word, word, word>(regs, 701, 702, 703, 704);

	// Set master
	master->setRequest_WriteRegisters(23, 3, 4, regs);

	// Start both master and slave
	auto t_master = system->createThread(master_thread, this);
	auto t_slave = system->createThread(slave_thread, this);
	system->waitForThreads(2, t_master, t_slave);

	ASSERT_TRUE(slaveSuccess) << "Total tries: " << master->_currentTries << endl;
	ASSERT_TRUE(masterSuccess) << "Total tries: " << master->_currentTries << endl;

	bool integrity = master->verifyResponseIntegrity();
	byte fcode;
	byte excode;
	bool isException = master->isExceptionResponse(fcode, excode);
	ASSERT_TRUE(integrity);
	ASSERT_TRUE(isException);
	ASSERT_EQ(fcode, MB_FC_WRITE_REGS);
	ASSERT_EQ(excode, MB_EX_ILLEGAL_ADDRESS);
}

INSTANTIATE_TEST_CASE_P(Inst, InboundResiliencyModbusIntegrationTests, ::testing::Values(None, InboundError, OutboundError, InboundError & OutboundError));