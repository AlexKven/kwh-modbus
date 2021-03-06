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
#include "test_helpers.h"
#include "WindowsFunctions.h"
#include "WindowsSystemFunctions.h"
#include "PointerTracker.h"
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
class ModbusIntegrationTests :
	public ::testing::Test,
	public ::testing::WithParamInterface<ModbusTransmissionError>
{
protected:
	T_ModbusSlave *slave = new T_ModbusSlave();
	T_ModbusMaster *master = new T_ModbusMaster();
	queue<byte> *slaveIn;
	queue<byte> *masterIn;
	MockSerialStream *slaveSerial;
	MockSerialStream *masterSerial;
	bool masterSuccess = false;
	bool slaveSuccess = false;
	ModbusTransmissionError errorType;

	PointerTracker tracker;

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

		tracker.addPointers(slave, master, slaveIn, masterIn, slaveSerial, masterSerial);

		master->config(masterSerial, system, 1200);
		slave->config(slaveSerial, system, 1200);
		seedRandom(masterSerial);
		seedRandom(slaveSerial);

		if (contains(errorType, InboundError))
		{
			masterSerial->setPerBitErrorProb(.020);
		}

		if (contains(errorType, OutboundError))
		{
			slaveSerial->setPerBitErrorProb(0.012);
			master->setMaxTimePerTryMicros(100000);
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
	}

	void slaveThread()
	{
		this->slaveSuccess = false;
		bool processed;
		bool broadcast;
		TIMEOUT_START(3000);
		if (this->errorType == 0)
		{
			while (!this->slave->task(processed, broadcast))
				TIMEOUT_CHECK;
		}
		else
		{
			while (this->master->getStatus() < 4)
			{
				this->slave->task(processed, broadcast);
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
		((ModbusIntegrationTests*)param)->slaveThread();
	}

	static void master_thread(void *param)
	{
		((ModbusIntegrationTests*)param)->masterThread();
	}
};

WindowsSystemFunctions *ModbusIntegrationTests::system;

TEST_P_TRAITS(ModbusIntegrationTests, ModbusIntegrationTests_ReadRegs_Success,
	Type, Integration, Threading, Multi, Determinism, Volatile, Case, Typical)
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

TEST_P_TRAITS(ModbusIntegrationTests, ModbusIntegrationTests_ReadRegs_Failure,
	Type, Integration, Threading, Multi, Determinism, Volatile, Case, Failure)
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

TEST_P_TRAITS(ModbusIntegrationTests, ModbusIntegrationTests_WriteReg_Success,
	Type, Integration, Threading, Multi, Determinism, Volatile, Case, Typical)
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

TEST_P_TRAITS(ModbusIntegrationTests, ModbusIntegrationTests_WriteReg_Failure,
	Type, Integration, Threading, Multi, Determinism, Volatile, Case, Failure)
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

TEST_P_TRAITS(ModbusIntegrationTests, ModbusIntegrationTests_WriteRegs_Success,
	Type, Integration, Threading, Multi, Determinism, Volatile, Case, Typical)
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

TEST_P_TRAITS(ModbusIntegrationTests, ModbusIntegrationTests_WriteRegs_Failure,
	Type, Integration, Threading, Multi, Determinism, Volatile, Case, Failure)
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

INSTANTIATE_TEST_CASE_P(NoErrors, ModbusIntegrationTests, ::testing::Values(None));
INSTANTIATE_TEST_CASE_P(InboundError, ModbusIntegrationTests, ::testing::Values(InboundError));
INSTANTIATE_TEST_CASE_P(OutboundError, ModbusIntegrationTests, ::testing::Values(OutboundError));
INSTANTIATE_TEST_CASE_P(InboundDelays, ModbusIntegrationTests, ::testing::Values(InboundDelays));
INSTANTIATE_TEST_CASE_P(OutboundDelays, ModbusIntegrationTests, ::testing::Values(OutboundDelays));
INSTANTIATE_TEST_CASE_P(AllErrors, ModbusIntegrationTests, ::testing::Values(InboundError & OutboundError));
INSTANTIATE_TEST_CASE_P(AllDelays, ModbusIntegrationTests, ::testing::Values(InboundDelays & OutboundDelays));
INSTANTIATE_TEST_CASE_P(AllDelaysAndErrors, ModbusIntegrationTests, ::testing::Values(InboundError & OutboundError & InboundDelays & OutboundDelays));