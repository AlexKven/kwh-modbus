#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/modbus.h"
#include "../kwh-modbus/libraries/modbus/modbusarray.h"
#include "../kwh-modbus/mock/mockpublicmodbus.h"
#include "test_helpers.h"

#define USE_MOCK Mock<MockPublicModbus<ModbusArray<Modbus>>> mock = Mock<MockPublicModbus<ModbusArray<Modbus>>>(*modbus);

class ModbusArrayTests : public ::testing::Test
{
protected:
	word *registerArray;
	MockPublicModbus<ModbusArray<Modbus>> *modbus = new MockPublicModbus<ModbusArray<Modbus>>();

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
	bool success1 = modbus->_resetFrame(10);
	byte *ptr = modbus->_getFramePtr();
	word len = modbus->_getFrameLength();

	bool success2 = modbus->_setFrameReg(3, 703);
	bool success3 = modbus->_setFrameReg(5, 703);
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
	bool success1 = modbus->_resetFrame(20);
	byte *ptr = modbus->_getFramePtr();
	word len = modbus->_getFrameLength();

	bool success2 = modbus->_setFrameReg(3, 703);

	ASSERT_EQ(len, 0);
	ASSERT_EQ(success1, false);
	ASSERT_EQ(success2, false);
}

TEST_F(ModbusArrayTests, ModbusArray_Frame_Register)
{
	bool success1 = modbus->_resetFrameRegs(5);
	byte *ptr = modbus->_getFramePtr();
	word len = modbus->_getFrameLength();

	bool success2 = modbus->_setFrameReg(3, 703);
	bool success3 = modbus->_setFrameReg(5, 703);

	word reg = modbus->_getFrameReg(3);

	ASSERT_EQ(len, 10);
	ASSERT_EQ(success1, true);
	ASSERT_EQ(success2, true);
	ASSERT_EQ(success3, false);
	ASSERT_EQ(reg, 703);
}

TEST_F(ModbusArrayTests, ModbusMemory_Frame_Register_BeyondMaxFrame)
{
	bool success1 = modbus->_resetFrameRegs(10);
	byte *ptr = modbus->_getFramePtr();
	word len = modbus->_getFrameLength();

	bool success2 = modbus->_setFrameReg(3, 703);
	bool success3 = modbus->_setFrameReg(5, 703);

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