#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/Modbus.h"
#include "../kwh-modbus/libraries/modbus/ModbusArray.h"
#include "test_helpers.h"
#include "PointerTracker.h"

#define USE_MOCK Mock<ModbusArray> mock = Mock<ModbusArray>(*modbus)

using namespace fakeit;

class ModbusArrayTests : public ::testing::Test
{
protected:
	word *registerArray;
	ModbusArray *modbus = new ModbusArray();

	PointerTracker tracker;

public:
	void SetUp()
	{
		registerArray = new word[12];
		modbus->init(registerArray, 5, 10, 14);
		tracker.addArray(registerArray);
		tracker.addPointer(modbus);
	}

	void TearDown()
	{
	}
};


TEST_F_TRAITS(ModbusArrayTests, ModbusArray_Hreg,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(ModbusArrayTests, ModbusArray_Frame_Byte,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	bool success1 = modbus->resetFrame(10);
	byte *ptr = modbus->getFramePtr();
	word len = modbus->getFrameLength();

	bool success2 = modbus->setFrameReg(3, 703, 0);
	bool success3 = modbus->setFrameReg(5, 703, 0);
	word regValue;

	parseArray(ptr + 6, regValue);

	ASSERT_EQ(len, 10);
	ASSERT_EQ(success1, true);
	ASSERT_EQ(success2, true);
	ASSERT_EQ(success3, false);
	ASSERT_EQ(regValue, 703);
}

TEST_F_TRAITS(ModbusArrayTests, ModbusArray_Frame_Byte_BeyondMaxFrame,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	bool success1 = modbus->resetFrame(20);
	byte *ptr = modbus->getFramePtr();
	word len = modbus->getFrameLength();

	bool success2 = modbus->setFrameReg(3, 703, 0);

	ASSERT_EQ(len, 0);
	ASSERT_EQ(success1, false);
	ASSERT_EQ(success2, false);
}

TEST_F_TRAITS(ModbusArrayTests, ModbusArray_FrameRegister,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	bool success1 = modbus->resetFrameRegs(5, 2);
	byte *ptr = modbus->getFramePtr();
	word len = modbus->getFrameLength();

	bool success2 = modbus->setFrameReg(3, 703, 2);
	bool success3 = modbus->setFrameReg(5, 703, 2);

	word reg = modbus->getFrameReg(3, 2);

	ASSERT_EQ(len, 12);
	ASSERT_EQ(success1, true);
	ASSERT_EQ(success2, true);
	ASSERT_EQ(success3, false);
	ASSERT_EQ(reg, 703);
}

TEST_F_TRAITS(ModbusArrayTests, ModbusMemory_FrameRegister_BeyondMaxFrame,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	bool success1 = modbus->resetFrameRegs(10, 4);
	byte *ptr = modbus->getFramePtr();
	word len = modbus->getFrameLength();

	bool success2 = modbus->setFrameReg(3, 703, 4);
	bool success3 = modbus->setFrameReg(5, 703, 4);

	ASSERT_EQ(len, 0);
	ASSERT_EQ(success1, false);
	ASSERT_EQ(success2, false);
	ASSERT_EQ(success3, false);
}

TEST_F_TRAITS(ModbusArrayTests, ModbusMemory_ValidRange_True,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	bool result = modbus->validRange(5, 4);

	ASSERT_TRUE(result);
}

TEST_F_TRAITS(ModbusArrayTests, ModbusMemory_ValidRange_False,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	bool result = modbus->validRange(5, 40);

	ASSERT_FALSE(result);
}