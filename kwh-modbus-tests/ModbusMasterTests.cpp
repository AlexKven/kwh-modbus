#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/Modbus.h"
#include "../kwh-modbus/noArduino/ModbusMemory.h"
#include "../kwh-modbus/libraries/modbus/ModbusSerial.hpp"
#include "../kwh-modbus/interfaces/ISerialStream.h"
#include "../kwh-modbus/noArduino/SystemFunctions.h"
#include "../kwh-modbus/mock/MockSerialStream.h"
#include "../kwh-modbus/libraries/modbusMaster/ModbusMaster.hpp"
#include "test_helpers.h"
#include "WindowsFunctions.h"
#include <queue>

#define USE_FAKE_SYSTEM Mock<ISystemFunctions> fakeSystem
#define USE_FAKE_SERIAL Mock<ISerialStream> fakeSerial

#define USE_MOCK Mock<ModbusMemory> mock = Mock<ModbusMemory>(*modbus)
#define USE_MOCK_SERIAL std::queue<uint8_t> readQueue; \
std::queue<uint8_t> writeQueue; \
MockSerialStream mockSerial = MockSerialStream(&readQueue, &writeQueue)

using namespace fakeit;

class ModbusMasterTests : public ::testing::Test
{
protected:
	ModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory> *modbus = new ModbusMaster<ISerialStream, ISystemFunctions, ModbusMemory>();
};

TEST_F(ModbusMasterTests, ModbusMaster_sendPDU)
{
	USE_FAKE_SYSTEM;
	USE_MOCK_SERIAL;
	Fake(Method(fakeSystem, pinMode));
	Fake(Method(fakeSystem, digitalWrite));
	Fake(Method(fakeSystem, delay));
	Fake(Method(fakeSystem, delayMicroseconds));
	modbus->config(&mockSerial, &fakeSystem.get(), 9600, 4);

	modbus->resetFrame(8);
	setArray<byte, byte, byte, byte, byte, byte, byte, byte>(modbus->getFramePtr(),
		1, 1, 2, 3, 5, 8, 13, 21);
	word crc = modbus->calcCrc(1, modbus->getFramePtr() + 1, 7);
	modbus->sendPDU();

	Verify(Method(fakeSystem, digitalWrite).Using(4, LOW)).Twice();
	Verify(Method(fakeSystem, digitalWrite).Using(4, HIGH)).Once();

	ASSERT_EQ(writeQueue.front(), 1);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 1);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 2);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 3);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 5);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 8);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 13);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 21);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), crc >> 8);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), crc & 0xFF);
}

TEST_F(ModbusMasterTests, ModbusMaster_setRequest_ReadRegisters)
{
	bool success = modbus->setRequest_ReadRegisters(29, 7, 266);

	ASSERT_TRUE(success);
	assertArrayEq<byte, byte, byte, byte, byte, byte>
		(modbus->getFramePtr(), 29, MB_FC_READ_REGS, 0, 7, 1, 10);
}

TEST_F(ModbusMasterTests, ModbusMaster_setRequest_WriteRegister)
{
	bool success = modbus->setRequest_WriteRegister(29, 7, 266);

	ASSERT_TRUE(success);
	assertArrayEq<byte, byte, byte, byte, byte, byte>
		(modbus->getFramePtr(), 29, MB_FC_WRITE_REG, 0, 7, 1, 10);
}

TEST_F(ModbusMasterTests, ModbusMaster_setRequest_WriteRegisters)
{
	word registers[4] = { 2, 3, 5, 257 };
	bool success = modbus->setRequest_WriteRegisters(29, 7, 4, registers);

	ASSERT_TRUE(success);
	assertArrayEq<byte, byte, byte, byte, byte, byte, byte, byte, byte, byte, byte, byte, byte, byte, byte>
		(modbus->getFramePtr(), 29, MB_FC_WRITE_REGS, 0, 7, 0, 4, 8, 0, 2, 0, 3, 0, 5, 1, 1);
}