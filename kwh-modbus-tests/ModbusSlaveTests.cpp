#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/Modbus.h"
#include "../kwh-modbus/noArduino/ModbusMemory.h"
#include "../kwh-modbus/libraries/modbus/ModbusSerial.hpp"
#include "../kwh-modbus/interfaces/ISerialStream.h"
#include "../kwh-modbus/noArduino/SystemFunctions.h"
#include "../kwh-modbus/mock/MockSerialStream.h"
#include "../kwh-modbus/libraries/modbusSlave/ModbusSlave.hpp"
#include "test_helpers.h"
#include "WindowsFunctions.h"
#include <queue>

#define USE_FAKE_SYSTEM Mock<ISystemFunctions> fakeSystem
#define USE_FAKE_SERIAL Mock<ISerialStream> fakeSerial

#define USE_MOCK Mock<ModbusMemory> mock = Mock<ModbusMemory>(*modbus)
#define USE_MOCK_SERIAL std::queue<uint8_t> readQueue; \
std::queue<uint8_t> writeQueue; \
MockSerialStream mockSerial = MockSerialStream(&readQueue, &writeQueue);

using namespace fakeit;

class ModbusSlaveTests : public ::testing::Test
{
protected:
	ModbusSlave<ISerialStream, ISystemFunctions, ModbusMemory> *modbus = new ModbusSlave<ISerialStream, ISystemFunctions, ModbusMemory>();

	void setup_FourRegisters(bool missingOne = false)
	{
		int missing = -1;
		if (missingOne)
		{
			missing = rand() % 4;
		}
		if (missing != 0)
			modbus->addHreg(5, 111);
		if (missing != 1)
			modbus->addHreg(6, 703);
		if (missing != 2)
			modbus->addHreg(7, 902);
		if (missing != 3)
			modbus->addHreg(8, 429);
	}
};

TEST_F(ModbusSlaveTests, ModbusSlave)
{
}

TEST_F(ModbusSlaveTests, Modbus_ReceivePDU_WriteRegister_IllegalAddress)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();

	setArray<byte, word, word>(frame,
		MB_FC_WRITE_REG,
		revBytes(5),
		revBytes(703));

	bool success = modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	ASSERT_FALSE(success);
	assertArrayEq<byte, byte>(frame,
		MB_FC_WRITE_REG + 0x80,
		MB_EX_ILLEGAL_ADDRESS);
}

TEST_F(ModbusSlaveTests, Modbus_ReceivePDU_WriteRegister_SlaveFailure)
{
	USE_MOCK;
	When(OverloadedMethod(mock, Hreg, bool(word, word))).Return(true);
	When(OverloadedMethod(mock, Hreg, word(word))).Return(-1);

	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();

	setArray<byte, word, word>(frame,
		MB_FC_WRITE_REG,
		revBytes(5),
		revBytes(703));

	bool success = modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	ASSERT_FALSE(success);
	assertArrayEq<byte, byte>(frame,
		MB_FC_WRITE_REG + 0x80,
		MB_EX_SLAVE_FAILURE);
}

TEST_F(ModbusSlaveTests, Modbus_ReceivePDU_WriteRegister_Success)
{
	modbus->resetFrame(5);
	modbus->addHreg(5, 3);
	byte *frame = modbus->getFramePtr();

	setArray(frame,
		(byte)MB_FC_WRITE_REG,
		revBytes<word>(5),
		revBytes<word>(703));

	bool success = modbus->receivePDU(frame);

	ASSERT_TRUE(success);
	ASSERT_EQ(modbus->_reply, MB_REPLY_ECHO);
	ASSERT_EQ(modbus->Reg(5), 703);
}

TEST_F(ModbusSlaveTests, Modbus_ReceivePDU_ReadRegisters_Success)
{
	modbus->resetFrame(5);
	setup_FourRegisters();
	byte *frame = modbus->getFramePtr();

	setArray(frame,
		(byte)MB_FC_READ_REGS,
		revBytes<word>(5),
		revBytes<word>(4));

	bool success = modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	ASSERT_TRUE(success);
	ASSERT_EQ(frameLength, 9);
	for (int i = 0; i < 4; i++)
	{
		ASSERT_EQ(modbus->getFrameReg(i, 1), modbus->Hreg(i + 5));
	}
}

TEST_F(ModbusSlaveTests, Modbus_ReceivePDU_ReadRegisters_IllegalAddress)
{
	modbus->resetFrame(5);
	setup_FourRegisters(true);
	byte *frame = modbus->getFramePtr();

	setArray(frame,
		(byte)MB_FC_READ_REGS,
		revBytes<word>(5),
		revBytes<word>(4));

	bool success = modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	ASSERT_FALSE(success);
	assertArrayEq<byte, byte>(frame,
		MB_FC_READ_REGS + 0x80,
		MB_EX_ILLEGAL_ADDRESS);
}

TEST_F(ModbusSlaveTests, Modbus_ReceivePDU_ReadRegisters_IllegalValue)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();

	setArray<byte, word, word>(frame,
		MB_FC_READ_REGS,
		revBytes(5),
		revBytes(0));

	bool success = modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	ASSERT_FALSE(success);
	assertArrayEq<byte, byte>(frame,
		MB_FC_READ_REGS + 0x80,
		MB_EX_ILLEGAL_VALUE);
}

TEST_F(ModbusSlaveTests, Modbus_ReceivePDU_ReadRegisters_SlaveFailure)
{
	USE_MOCK;
	modbus->resetFrame(5);
	setup_FourRegisters();
	byte *frame = modbus->getFramePtr();

	setArray(frame,
		(byte)MB_FC_READ_REGS,
		revBytes<word>(5),
		revBytes<word>(4));

	// Do the frame reset for the error in advance, since we'll mock it.
	modbus->resetFrame(2);
	When(Method(mock, resetFrame)).Return(false).Return(true);

	bool success = modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	ASSERT_FALSE(success);
	assertArrayEq<byte, byte>(frame,
		MB_FC_READ_REGS + 0x80,
		MB_EX_SLAVE_FAILURE);
}

TEST_F(ModbusSlaveTests, Modbus_ReceivePDU_WriteRegisters_Success)
{
	modbus->resetFrame(14);
	setup_FourRegisters();
	byte *frame = modbus->getFramePtr();

	word input_start = 5;
	word input_length = 4;
	revBytesAll(input_start, input_length);

	setArray(frame,
		(byte)MB_FC_WRITE_REGS,
		input_start,
		input_length,
		(byte)8,
		revBytes<word>(5),
		revBytes<word>(25),
		revBytes<word>(125),
		revBytes<word>(625));

	bool success = modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	ASSERT_TRUE(success);
	assertArrayEq<byte, word, word>(frame,
		MB_FC_WRITE_REGS,
		input_start,
		input_length);

	ASSERT_EQ(modbus->_reply, MB_REPLY_NORMAL);

	word curVal = 5;
	for (int i = 0; i < 4; i++)
	{
		ASSERT_EQ(curVal, modbus->Hreg(i + 5));
		curVal *= 5;
	}
}

TEST_F(ModbusSlaveTests, Modbus_ReceivePDU_WriteRegisters_IllegalAddress)
{
	modbus->resetFrame(14);
	setup_FourRegisters(true);
	byte *frame = modbus->getFramePtr();

	word input_start = 5;
	word input_length = 4;
	revBytesAll(input_start, input_length);

	setArray(frame,
		(byte)MB_FC_WRITE_REGS,
		input_start,
		input_length,
		(byte)8,
		revBytes<word>(5),
		revBytes<word>(25),
		revBytes<word>(125),
		revBytes<word>(625));

	bool success = modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	ASSERT_FALSE(success);
	assertArrayEq<byte, byte>(frame,
		MB_FC_WRITE_REGS + 0x80,
		MB_EX_ILLEGAL_ADDRESS);
}

TEST_F(ModbusSlaveTests, Modbus_ReceivePDU_WriteRegisters_IllegalValue)
{
	modbus->resetFrame(14);
	setup_FourRegisters();
	byte *frame = modbus->getFramePtr();

	word input_start = 5;
	word input_length = 0;
	revBytesAll(input_start, input_length);

	setArray(frame,
		(byte)MB_FC_WRITE_REGS,
		input_start,
		input_length,
		(byte)8,
		revBytes<word>(5),
		revBytes<word>(25),
		revBytes<word>(125),
		revBytes<word>(625));

	bool success = modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	ASSERT_FALSE(success);
	assertArrayEq<byte, byte>(frame,
		MB_FC_WRITE_REGS + 0x80,
		MB_EX_ILLEGAL_VALUE);
}

TEST_F(ModbusSlaveTests, Modbus_ReceivePDU_WriteRegisters_IllegalValue_BadByteCount)
{
	modbus->resetFrame(14);
	setup_FourRegisters();
	byte *frame = modbus->getFramePtr();

	word input_start = 5;
	word input_length = 4;
	revBytesAll(input_start, input_length);

	setArray(frame,
		(byte)MB_FC_WRITE_REGS,
		input_start,
		input_length,
		(byte)10,
		revBytes<word>(5),
		revBytes<word>(25),
		revBytes<word>(125),
		revBytes<word>(625));

	bool success = modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	ASSERT_FALSE(success);
	assertArrayEq<byte, byte>(frame,
		MB_FC_WRITE_REGS + 0x80,
		MB_EX_ILLEGAL_VALUE);
}

TEST_F(ModbusSlaveTests, Modbus_ReceivePDU_WriteRegisters_SlaveFailure)
{
	USE_MOCK;
	modbus->resetFrame(14);
	setup_FourRegisters();
	byte *frame = modbus->getFramePtr();

	word input_start = 5;
	word input_length = 4;
	revBytesAll(input_start, input_length);

	setArray(frame,
		(byte)MB_FC_WRITE_REGS,
		input_start,
		input_length,
		(byte)8,
		revBytes<word>(5),
		revBytes<word>(25),
		revBytes<word>(125),
		revBytes<word>(625));

	// Do the frame reset for the error in advance, since we'll mock it.
	When(Method(mock, resetFrame)).Return(false).Return(true);

	bool success = modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	ASSERT_FALSE(success);
	assertArrayEq<byte, byte>(frame,
		MB_FC_WRITE_REGS + 0x80,
		MB_EX_SLAVE_FAILURE);
}

TEST_F(ModbusSlaveTests, Modbus_ReceivePDU_IllegalFunction)
{
	modbus->resetFrame(5);
	setup_FourRegisters();
	byte *frame = modbus->getFramePtr();

	word input_start = 5;
	word input_length = 4;
	revBytesAll(input_start, input_length);

	setArray(frame,
		(byte)42,
		input_start,
		input_length);

	bool success = modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	ASSERT_FALSE(success);
	assertArrayEq<byte, byte>(frame,
		42 + 0x80,
		MB_EX_ILLEGAL_FUNCTION);
}