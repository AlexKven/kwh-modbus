#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbusSlave/modbus.cpp"
#include "../kwh-modbus/libraries/modbusSlave/modbusmemory.h"
#include "../kwh-modbus/mock/mockpublicmodbus.h"

#define USE_MOCK Mock<MockPublicModbus<ModbusMemory<Modbus>>> mock = Mock<MockPublicModbus<ModbusMemory<Modbus>>>(*modbus);

using namespace fakeit;

class ModbusMemoryTests : public ::testing::Test
{
protected:
	MockPublicModbus<ModbusMemory<Modbus>> *modbus = new MockPublicModbus<ModbusMemory<Modbus>>();

	void setup_FourRegisters(bool missingOne = false)
	{
		int missing = -1;
		if (missingOne)
		{
			missing = rand() % 4;
		}
		if (missing != 0)
			modbus->_addReg(5, 111);
		if (missing != 1)
			modbus->_addReg(6, 703);
		if (missing != 2)
			modbus->_addReg(7, 902);
		if (missing != 3)
			modbus->_addReg(8, 429);
	}
public:
	void SetUp()
	{
	}

	void TearDown()
	{
		delete modbus;
	}
};

TEST_F(ModbusMemoryTests, ModbusMemory_Hreg)
{
	modbus->addHreg(5, 1);
	modbus->addHreg(20, 50);
	word val1 = modbus->Hreg(5);
	word val2 = modbus->Hreg(20);
	bool bool1 = modbus->Hreg(21, 10);
	bool bool2 = modbus->Hreg(20, 24);
	word val3 = modbus->Hreg(20);

	ASSERT_EQ(val1, 1);
	ASSERT_EQ(val2, 50);
	ASSERT_EQ(val3, 24);
	ASSERT_EQ(bool1, false);
	ASSERT_EQ(bool2, true);
}

TEST_F(ModbusMemoryTests, ModbusMemory_Frame_Byte)
{
	modbus->_resetFrame(10);
	byte *ptr = modbus->_getFramePtr();
	word len = modbus->_getFrameLength();

	bool success1 = modbus->_setFrameReg(3, 703);
	bool success2 = modbus->_setFrameReg(5, 703);

	byte regLow = ptr[6];
	byte regHigh = ptr[7];

	ASSERT_EQ(len, 10);
	ASSERT_EQ(success1, true);
	ASSERT_EQ(success2, false);
	ASSERT_EQ(regLow, 703 % 256);
	ASSERT_EQ(regHigh, 703 / 256);
}

TEST_F(ModbusMemoryTests, ModbusMemory_Frame_Register)
{
	modbus->_resetFrameRegs(5);
	byte *ptr = modbus->_getFramePtr();
	word len = modbus->_getFrameLength();

	bool success1 = modbus->_setFrameReg(3, 703);
	bool success2 = modbus->_setFrameReg(5, 703);

	word reg = modbus->_getFrameReg(3);

	ASSERT_EQ(len, 10);
	ASSERT_EQ(success1, true);
	ASSERT_EQ(success2, false);
	ASSERT_EQ(reg, 703);
}

TEST_F(ModbusMemoryTests, Modbus_ReceivePDU_WriteRegister_IllegalAddress)
{
	modbus->_resetFrame(5);
	byte *frame = modbus->_getFramePtr();
	frame[0] = ::MB_FC_WRITE_REG;
	word input1 = 5;
	word input2 = 703;
	frame[1] = input1 / 256;
	frame[2] = input1 % 256;
	frame[3] = input2 / 256;
	frame[4] = input2 % 256;

	modbus->_receivePDU(frame);

	word frameLength = modbus->_getFrameLength();
	frame = modbus->_getFramePtr();

	ASSERT_EQ(frame[0], MB_FC_WRITE_REG + 0x80);
	ASSERT_EQ(frame[1], MB_EX_ILLEGAL_ADDRESS);
}

TEST_F(ModbusMemoryTests, Modbus_ReceivePDU_WriteRegister_SlaveFailure)
{
	USE_MOCK;
	When(OverloadedMethod(mock, Hreg, bool(word, word))).Return(true);
	When(OverloadedMethod(mock, Hreg, word(word))).Return(-1);

	modbus->_resetFrame(5);
	byte *frame = modbus->_getFramePtr();
	frame[0] = ::MB_FC_WRITE_REG;
	word input1 = 5;
	word input2 = 703;
	frame[1] = input1 / 256;
	frame[2] = input1 % 256;
	frame[3] = input2 / 256;
	frame[4] = input2 % 256;

	modbus->_receivePDU(frame);

	word frameLength = modbus->_getFrameLength();
	frame = modbus->_getFramePtr();

	ASSERT_EQ(frame[0], MB_FC_WRITE_REG + 0x80);
	ASSERT_EQ(frame[1], MB_EX_SLAVE_FAILURE);
}

TEST_F(ModbusMemoryTests, Modbus_ReceivePDU_WriteRegister_Success)
{
	modbus->_resetFrame(5);
	modbus->_addReg(5, 3);
	byte *frame = modbus->_getFramePtr();
	frame[0] = ::MB_FC_WRITE_REG;
	word input1 = 5;
	word input2 = 703;
	frame[1] = input1 / 256;
	frame[2] = input1 % 256;
	frame[3] = input2 / 256;
	frame[4] = input2 % 256;

	modbus->_receivePDU(frame);

	ASSERT_EQ(modbus->_getReply(), MB_REPLY_ECHO);
	ASSERT_EQ(modbus->_Reg(5), 703);
}

TEST_F(ModbusMemoryTests, Modbus_ReceivePDU_ReadRegisters_Success)
{
	modbus->_resetFrame(5);
	setup_FourRegisters();
	byte *frame = modbus->_getFramePtr();
	frame[0] = ::MB_FC_READ_REGS;
	word input1 = 5;
	word input2 = 4;
	frame[1] = input1 / 256;
	frame[2] = input1 % 256;
	frame[3] = input2 / 256;
	frame[4] = input2 % 256;

	modbus->_receivePDU(frame);

	word frameLength = modbus->_getFrameLength();
	frame = modbus->_getFramePtr();

	ASSERT_EQ(frameLength, 8);
	for (int i = 0; i < 4; i++)
	{
		ASSERT_EQ(modbus->_getFrameReg(i), modbus->Hreg(i + 5));
	}
}

TEST_F(ModbusMemoryTests, Modbus_ReceivePDU_ReadRegisters_IllegalAddress)
{
	modbus->_resetFrame(5);
	setup_FourRegisters(true);
	byte *frame = modbus->_getFramePtr();
	frame[0] = ::MB_FC_READ_REGS;
	word input1 = 5;
	word input2 = 4;
	frame[1] = input1 / 256;
	frame[2] = input1 % 256;
	frame[3] = input2 / 256;
	frame[4] = input2 % 256;

	modbus->_receivePDU(frame);

	word frameLength = modbus->_getFrameLength();
	frame = modbus->_getFramePtr();

	ASSERT_EQ(frame[0], MB_FC_READ_REGS + 0x80);
	ASSERT_EQ(frame[1], MB_EX_ILLEGAL_ADDRESS);
}

TEST_F(ModbusMemoryTests, Modbus_ReceivePDU_ReadRegisters_IllegalValue)
{
	modbus->_resetFrame(5);
	byte *frame = modbus->_getFramePtr();
	frame[0] = ::MB_FC_READ_REGS;
	word input1 = 5;
	word input2 = 0;
	frame[1] = input1 / 256;
	frame[2] = input1 % 256;
	frame[3] = input2 / 256;
	frame[4] = input2 % 256;

	modbus->_receivePDU(frame);

	word frameLength = modbus->_getFrameLength();
	frame = modbus->_getFramePtr();

	ASSERT_EQ(frame[0], MB_FC_READ_REGS + 0x80);
	ASSERT_EQ(frame[1], MB_EX_ILLEGAL_VALUE);
}

TEST_F(ModbusMemoryTests, Modbus_ReceivePDU_ReadRegisters_SlaveFailure)
{
	USE_MOCK;
	modbus->_resetFrame(5);
	setup_FourRegisters();
	byte *frame = modbus->_getFramePtr();
	frame[0] = ::MB_FC_READ_REGS;
	word input1 = 5;
	word input2 = 4;
	frame[1] = input1 / 256;
	frame[2] = input1 % 256;
	frame[3] = input2 / 256;
	frame[4] = input2 % 256;

	// Do the frame reset for the error in advance, since we'll mock it.
	modbus->_resetFrame(2);
	When(Method(mock, _resetFrame)).Return(false).Return(true);

	modbus->_receivePDU(frame);

	word frameLength = modbus->_getFrameLength();
	frame = modbus->_getFramePtr();

	ASSERT_EQ(frame[0], MB_FC_READ_REGS + 0x80);
	ASSERT_EQ(frame[1], MB_EX_SLAVE_FAILURE);
}