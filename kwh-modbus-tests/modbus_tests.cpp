#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbusSlave/modbus.cpp"
#include "../kwh-modbus/libraries/modbusSlave/modbusmemory.h"
#include "../kwh-modbus/mock/mockpublicmodbus.h"

using namespace fakeit;

TEST(Modbus, ModbusMemory_Hreg)
{
	ModbusMemory<Modbus> *modbus = new ModbusMemory<Modbus>();
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

TEST(Modbus, ModbusMemory_Frame_Byte)
{
	ModbusMemory<MockPublicModbus<Modbus>> *modbus = new ModbusMemory<MockPublicModbus<Modbus>>();
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

TEST(Modbus, ModbusMemory_Frame_Register)
{
	ModbusMemory<MockPublicModbus<Modbus>> *modbus = new ModbusMemory<MockPublicModbus<Modbus>>();
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

TEST(Modbus, Modbus_ReceivePDU_WriteRegister_IllegalAddress)
{
	ModbusMemory<MockPublicModbus<Modbus>> *modbus = new ModbusMemory<MockPublicModbus<Modbus>>();
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

TEST(Modbus, Modbus_ReceivePDU_WriteRegister_SlaveFailure)
{
	ModbusMemory<MockPublicModbus<Modbus>> *modbus = new ModbusMemory<MockPublicModbus<Modbus>>();
	Mock<ModbusMemory<MockPublicModbus<Modbus>>> mock(*modbus);

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

TEST(Modbus, Modbus_ReceivePDU_WriteRegister_Success)
{
	ModbusMemory<MockPublicModbus<Modbus>> *modbus = new ModbusMemory<MockPublicModbus<Modbus>>();
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