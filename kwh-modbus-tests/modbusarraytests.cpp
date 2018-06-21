#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/modbus.h"
#include "../kwh-modbus/libraries/modbus/modbusarray.h"
#include "test_helpers.h"

#define USE_MOCK Mock<ModbusArray> mock = Mock<ModbusArray>(*modbus);

using namespace fakeit;

class ModbusArrayTests : public ::testing::Test
{
protected:
	word *registerArray;
	ModbusArray *modbus = new ModbusArray();

public:
	void SetUp()
	{
		registerArray = new word[10];
		modbus->init(registerArray, 5, 10, 12);
	}

	void TearDown()
	{
		delete modbus;
		delete[] registerArray;
	}
};


TEST_F(ModbusArrayTests, ModbusArray_Hreg)
{
	modbus->Hreg(5, 1);
	modbus->Hreg(10, 50);
	word val1 = modbus->Hreg(5);
	word val2 = modbus->Hreg(10);
	bool bool1 = modbus->Hreg(20, 10);
	bool bool2 = modbus->Hreg(10, 24);
	word val3 = modbus->Hreg(10);

	ASSERT_EQ(val1, 1);
	ASSERT_EQ(val2, 50);
	ASSERT_EQ(val3, 24);
	ASSERT_EQ(bool1, false);
	ASSERT_EQ(bool2, true);
}

TEST_F(ModbusArrayTests, ModbusArray_Frame_Byte)
{
	bool success1 = modbus->resetFrame(10);
	byte *ptr = modbus->getFramePtr();
	word len = modbus->getFrameLength();

	bool success2 = modbus->setFrameReg(3, 703);
	bool success3 = modbus->setFrameReg(5, 703);
	word regValue;

	parseArray(ptr + 6, regValue);

	ASSERT_EQ(len, 10);
	ASSERT_EQ(success1, true);
	ASSERT_EQ(success2, true);
	ASSERT_EQ(success3, false);
	ASSERT_EQ(regValue, 703);
}

TEST_F(ModbusArrayTests, ModbusArray_Frame_Byte_BeyondMaxFrame)
{
	bool success1 = modbus->resetFrame(20);
	byte *ptr = modbus->getFramePtr();
	word len = modbus->getFrameLength();

	bool success2 = modbus->setFrameReg(3, 703);

	ASSERT_EQ(len, 0);
	ASSERT_EQ(success1, false);
	ASSERT_EQ(success2, false);
}

TEST_F(ModbusArrayTests, ModbusArray_FrameRegister)
{
	bool success1 = modbus->resetFrameRegs(5);
	byte *ptr = modbus->getFramePtr();
	word len = modbus->getFrameLength();

	bool success2 = modbus->setFrameReg(3, 703);
	bool success3 = modbus->setFrameReg(5, 703);

	word reg = modbus->getFrameReg(3);

	ASSERT_EQ(len, 10);
	ASSERT_EQ(success1, true);
	ASSERT_EQ(success2, true);
	ASSERT_EQ(success3, false);
	ASSERT_EQ(reg, 703);
}

TEST_F(ModbusArrayTests, ModbusMemory_FrameRegister_BeyondMaxFrame)
{
	bool success1 = modbus->resetFrameRegs(10);
	byte *ptr = modbus->getFramePtr();
	word len = modbus->getFrameLength();

	bool success2 = modbus->setFrameReg(3, 703);
	bool success3 = modbus->setFrameReg(5, 703);

	ASSERT_EQ(len, 0);
	ASSERT_EQ(success1, false);
	ASSERT_EQ(success2, false);
	ASSERT_EQ(success3, false);
}

TEST_F(ModbusArrayTests, ModbusMemory_ValidRange_True)
{
	bool result = modbus->validRange(5, 4);

	ASSERT_TRUE(result);
}

TEST_F(ModbusArrayTests, ModbusMemory_ValidRange_False)
{
	bool result = modbus->validRange(5, 40);

	ASSERT_FALSE(result);
}

TEST_F(ModbusArrayTests, Modbus_ReceivePDU_WriteRegister_IllegalAddress)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();

	setArray<byte, word, word>(frame,
		MB_FC_WRITE_REG,
		revBytes(5),
		revBytes(703));

	modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	assertArrayEq<byte, byte>(frame,
		MB_FC_WRITE_REG + 0x80,
		MB_EX_ILLEGAL_ADDRESS);
}

TEST_F(ModbusArrayTests, Modbus_ReceivePDU_WriteRegister_SlaveFailure)
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

	modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	assertArrayEq<byte, byte>(frame,
		MB_FC_WRITE_REG + 0x80,
		MB_EX_SLAVE_FAILURE);
}

TEST_F(ModbusArrayTests, Modbus_ReceivePDU_WriteRegister_Success)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();

	setArray(frame,
		(byte)MB_FC_WRITE_REG,
		revBytes<word>(5),
		revBytes<word>(703));

	modbus->receivePDU(frame);

	ASSERT_EQ(modbus->_reply, MB_REPLY_ECHO);
	ASSERT_EQ(modbus->Reg(5), 703);
}

TEST_F(ModbusArrayTests, Modbus_ReceivePDU_ReadRegisters_Success)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();

	setArray(frame,
		(byte)MB_FC_READ_REGS,
		revBytes<word>(5),
		revBytes<word>(4));

	modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	ASSERT_EQ(frameLength, 8);
	for (int i = 0; i < 4; i++)
	{
		ASSERT_EQ(modbus->getFrameReg(i), modbus->Hreg(i + 5));
	}
}

TEST_F(ModbusArrayTests, Modbus_ReceivePDU_ReadRegisters_IllegalAddress)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();

	setArray(frame,
		(byte)MB_FC_READ_REGS,
		revBytes<word>(3),
		revBytes<word>(4));

	modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	assertArrayEq<byte, byte>(frame,
		MB_FC_READ_REGS + 0x80,
		MB_EX_ILLEGAL_ADDRESS);
}

TEST_F(ModbusArrayTests, Modbus_ReceivePDU_ReadRegisters_IllegalValue)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();

	setArray<byte, word, word>(frame,
		MB_FC_READ_REGS,
		revBytes(5),
		revBytes(0));

	modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	assertArrayEq<byte, byte>(frame,
		MB_FC_READ_REGS + 0x80,
		MB_EX_ILLEGAL_VALUE);
}

TEST_F(ModbusArrayTests, Modbus_ReceivePDU_ReadRegisters_SlaveFailure)
{
	USE_MOCK;
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();

	setArray(frame,
		(byte)MB_FC_READ_REGS,
		revBytes<word>(5),
		revBytes<word>(4));

	// Do the frame reset for the error in advance, since we'll mock it.
	modbus->resetFrame(2);
	When(Method(mock, resetFrame)).Return(false).Return(true);

	modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	assertArrayEq<byte, byte>(frame,
		MB_FC_READ_REGS + 0x80,
		MB_EX_SLAVE_FAILURE);
}

TEST_F(ModbusArrayTests, Modbus_ReceivePDU_WriteRegisters_Success)
{
	modbus->resetFrame(14);
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

	modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

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

TEST_F(ModbusArrayTests, Modbus_ReceivePDU_WriteRegisters_IllegalAddress)
{
	modbus->resetFrame(14);
	byte *frame = modbus->getFramePtr();

	word input_start = 3;
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

	modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	assertArrayEq<byte, byte>(frame,
		MB_FC_WRITE_REGS + 0x80,
		MB_EX_ILLEGAL_ADDRESS);
}

TEST_F(ModbusArrayTests, Modbus_ReceivePDU_WriteRegisters_IllegalValue)
{
	modbus->resetFrame(14);
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

	modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	assertArrayEq<byte, byte>(frame,
		MB_FC_WRITE_REGS + 0x80,
		MB_EX_ILLEGAL_VALUE);
}

TEST_F(ModbusArrayTests, Modbus_ReceivePDU_WriteRegisters_IllegalValue_BadByteCount)
{
	modbus->resetFrame(14);
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

	modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	assertArrayEq<byte, byte>(frame,
		MB_FC_WRITE_REGS + 0x80,
		MB_EX_ILLEGAL_VALUE);
}

TEST_F(ModbusArrayTests, Modbus_ReceivePDU_WriteRegisters_SlaveFailure)
{
	USE_MOCK;
	modbus->resetFrame(14);
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

	modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	assertArrayEq<byte, byte>(frame,
		MB_FC_WRITE_REGS + 0x80,
		MB_EX_SLAVE_FAILURE);
}

TEST_F(ModbusArrayTests, Modbus_ReceivePDU_IllegalFunction)
{
	modbus->resetFrame(5);
	byte *frame = modbus->getFramePtr();

	word input_start = 5;
	word input_length = 4;
	revBytesAll(input_start, input_length);

	setArray(frame,
		(byte)42,
		input_start,
		input_length);

	modbus->receivePDU(frame);

	word frameLength = modbus->getFrameLength();
	frame = modbus->getFramePtr();

	assertArrayEq<byte, byte>(frame,
		42 + 0x80,
		MB_EX_ILLEGAL_FUNCTION);
}