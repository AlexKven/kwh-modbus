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

using namespace fakeit;

class ModbusMasterSlaveTests : public ::testing::Test
{
protected:
	Mock<ISystemFunctions> fakeSystem;
	ModbusSlave<ISerialStream, ISystemFunctions, ModbusMemory> *slave = new ModbusSlave<ISerialStream, ISystemFunctions, ModbusMemory>();
	ModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory> *master = new ModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory>();
	queue<byte> *slaveIn;
	queue<byte> *masterIn;
	MockSerialStream *slaveSerial;
	MockSerialStream *masterSerial;

public:
	void SetUp()
	{
		slaveIn = new queue<byte>();
		masterIn = new queue<byte>();
		slaveSerial = new MockSerialStream(slaveIn, masterIn);
		masterSerial = new MockSerialStream(masterIn, slaveIn);
	}

	void TearDown()
	{
		delete slaveIn;
		delete masterIn;
		delete slaveSerial;
		delete masterSerial;
	}


	static void wait_thread(void* milliseconds)
	{
		WindowsFunctions func;
		func.Windows_Sleep(*(int*)milliseconds);
	}
};

TEST_F(ModbusMasterSlaveTests, ModbusSlave)
{
	Fake(Method(fakeSystem, delay));
	Fake(Method(fakeSystem, delayMicroseconds));
	Fake(Method(fakeSystem, digitalWrite));
	Fake(Method(fakeSystem, pinMode));
	When(Method(fakeSystem, millis)).AlwaysReturn(0);


}

TEST_F(ModbusMasterSlaveTests, MultithreadingTest)
{
	WindowsSystemFunctions wsf;
	auto t1 = wsf.createThread(wait_thread, new int(2000));
	auto t2 = wsf.createThread(wait_thread, new int(9000));
	auto t3 = wsf.createThread(wait_thread, new int(4000));
	auto t4 = wsf.createThread(wait_thread, new int(15000));
	auto t5 = wsf.createThread(wait_thread, new int(6000));
	wsf.waitForThreads(t1, t2, t3, t4, t5);
}