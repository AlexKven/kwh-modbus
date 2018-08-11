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

TEST_F(ModbusMasterTests, ModbusMaster_verifyResponseIntegrity_Success)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte>(frame, 13, 41, 43);
	word crc = modbus->calcCrc(13, frame + 1, 2);
	setArray<word>(frame + 3, revBytes(crc));
	modbus->_recipientId = 13;

	bool success = modbus->verifyResponseIntegrity();
	ASSERT_TRUE(success);
}

TEST_F(ModbusMasterTests, ModbusMaster_verifyResponseIntegrity_Fail_WrongRecipient)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte>(frame, 13, 41, 43);
	word crc = modbus->calcCrc(13, frame + 1, 2);
	setArray<word>(frame + 3, revBytes(crc));
	modbus->_recipientId = 11;

	bool success = modbus->verifyResponseIntegrity();
	ASSERT_FALSE(success);
}

TEST_F(ModbusMasterTests, ModbusMaster_verifyResponseIntegrity_Fail_BadCRC)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte>(frame, 13, 41, 43);
	word crc = modbus->calcCrc(13, frame + 1, 2) - 1234;
	setArray<word>(frame + 3, revBytes(crc));
	modbus->_recipientId = 13;

	bool success = modbus->verifyResponseIntegrity();
	ASSERT_FALSE(success);
}

TEST_F(ModbusMasterTests, ModbusMaster_isReadRegsResponse_True)
{
	modbus->resetFrame(6);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte, byte, byte, byte>(
		frame, 13, MB_FC_READ_REGS, 43, 42, 41, 40);

	word count = 0;
	word *regs = nullptr;

	bool result = modbus->isReadRegsResponse(count, regs);
	ASSERT_TRUE(result);
	ASSERT_EQ(count, 1);
	assertArrayEq<word>(regs, 42 * 256 + 43);
}

TEST_F(ModbusMasterTests, ModbusMaster_isReadRegsResponse_False_OddLength)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte, byte, byte>(
		frame, 13, MB_FC_READ_REGS, 43, 42, 41);

	word count = 0;
	word *regs = nullptr;

	bool result = modbus->isReadRegsResponse(count, regs);
	ASSERT_FALSE(result);
	ASSERT_EQ(count, 0);
	ASSERT_EQ(regs, nullptr);
}

TEST_F(ModbusMasterTests, ModbusMaster_isReadRegsResponse_FALSE_WrongFCode)
{
	modbus->resetFrame(6);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte, byte, byte, byte>(
		frame, 13, MB_FC_WRITE_REGS, 43, 42, 41, 40);

	word count = 0;
	word *regs = nullptr;

	bool result = modbus->isReadRegsResponse(count, regs);
	ASSERT_FALSE(result);
	ASSERT_EQ(count, 0);
	ASSERT_EQ(regs, nullptr);
}

TEST_F(ModbusMasterTests, ModbusMaster_isExceptionResponse_True)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte, byte, byte>(
		frame, 13, MB_FC_READ_REGS + 128, MB_EX_SLAVE_FAILURE, 42, 41);

	byte fCode = 0;
	byte exCode = 0;

	bool result = modbus->isExceptionResponse(fCode, exCode);
	ASSERT_TRUE(result);
	ASSERT_EQ(fCode, MB_FC_READ_REGS);
	ASSERT_EQ(exCode, MB_EX_SLAVE_FAILURE);
}

TEST_F(ModbusMasterTests, ModbusMaster_isExceptionResponse_False_WrongLength)
{
	modbus->resetFrame(4);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte, byte>(
		frame, 13, MB_FC_READ_REGS + 128, MB_EX_SLAVE_FAILURE, 42);

	byte fCode = 0;
	byte exCode = 0;

	bool result = modbus->isExceptionResponse(fCode, exCode);
	ASSERT_FALSE(result);
	ASSERT_EQ(fCode, 0);
	ASSERT_EQ(exCode, 0);
}

TEST_F(ModbusMasterTests, ModbusMaster_isExceptionResponse_False_ImproperFCode)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte, byte, byte>(
		frame, 13, MB_FC_READ_REGS, MB_EX_SLAVE_FAILURE, 42, 41);

	byte fCode = 0;
	byte exCode = 0;

	bool result = modbus->isExceptionResponse(fCode, exCode);
	ASSERT_FALSE(result);
	ASSERT_EQ(fCode, 0);
	ASSERT_EQ(exCode, 0);
}

TEST_F(ModbusMasterTests, ModbusMaster_isWriteRegResponse_True)
{
	modbus->resetFrame(6);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte, byte, byte>(
		frame, 13, MB_FC_WRITE_REG, 43, 42, 41, 40);

	bool result = modbus->isWriteRegResponse();
	ASSERT_TRUE(result);
}

TEST_F(ModbusMasterTests, ModbusMaster_isWriteRegResponse_False_WrongFCode)
{
	modbus->resetFrame(6);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte, byte, byte>(
		frame, 13, MB_FC_WRITE_REGS, 43, 42, 41, 40);

	bool result = modbus->isWriteRegResponse();
	ASSERT_FALSE(result);
}

TEST_F(ModbusMasterTests, ModbusMaster_isWriteRegResponse_False_BadLength)
{
	modbus->resetFrame(7);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte, byte, byte>(
		frame, 13, MB_FC_WRITE_REG, 43, 42, 41, 40, 39);

	bool result = modbus->isWriteRegResponse();
	ASSERT_FALSE(result);
}

TEST_F(ModbusMasterTests, ModbusMaster_isWriteRegsResponse_True)
{
	modbus->resetFrame(8);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte, byte, byte>(
		frame, 13, MB_FC_WRITE_REGS, 43, 42, 41, 40, 39, 38);

	bool result = modbus->isWriteRegsResponse();
	ASSERT_TRUE(result);
}

TEST_F(ModbusMasterTests, ModbusMaster_isWriteRegsResponse_False_WrongFCode)
{
	modbus->resetFrame(6);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte, byte, byte>(
		frame, 13, MB_FC_READ_REGS, 43, 42, 41, 40, 39, 38);

	bool result = modbus->isWriteRegsResponse();
	ASSERT_FALSE(result);
}

TEST_F(ModbusMasterTests, ModbusMaster_isWriteRegsResponse_False_BadLength)
{
	modbus->resetFrame(7);
	byte *frame = modbus->getFramePtr();
	setArray<byte, byte, byte, byte, byte>(
		frame, 13, MB_FC_WRITE_REGS, 43, 42, 41, 40, 39);

	bool result = modbus->isWriteRegsResponse();
	ASSERT_FALSE(result);
}