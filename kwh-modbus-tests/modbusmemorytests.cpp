#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/Modbus.h"
#include "../kwh-modbus/noArduino/ModbusMemory.h"
#include "test_helpers.h"
#include "PointerTracker.h"

#define USE_MOCK Mock<ModbusMemory> mock = Mock<ModbusMemory>(*modbus)

using namespace fakeit;

class ModbusMemoryTests : public ::testing::Test
{
protected:
	ModbusMemory *modbus = new ModbusMemory();

	PointerTracker tracker;

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
public:
	void SetUp()
	{
		tracker.addPointer(modbus);
	}

	void TearDown()
	{
	}
};


TEST_F_TRAITS(ModbusMemoryTests, ModbusMemory_Hreg,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(ModbusMemoryTests, ModbusMemory_Frame_Byte,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	modbus->resetFrame(13);
	byte *ptr = modbus->getFramePtr();
	word len = modbus->getFrameLength();

	bool success1 = modbus->setFrameReg(3, 703, 3);
	bool success2 = modbus->setFrameReg(5, 703, 3);
	word regValue;

	parseArray(ptr + 9, regValue);

	ASSERT_EQ(len, 13);
	ASSERT_EQ(success1, true);
	ASSERT_EQ(success2, false);
	ASSERT_EQ(regValue, 703);
}

TEST_F_TRAITS(ModbusMemoryTests, ModbusMemory_FrameRegister,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	modbus->resetFrameRegs(5, 1);
	byte *ptr = modbus->getFramePtr();
	word len = modbus->getFrameLength();

	bool success1 = modbus->setFrameReg(3, 703, 1);
	bool success2 = modbus->setFrameReg(5, 703, 1);

	word reg = modbus->getFrameReg(3, 1);

	ASSERT_EQ(len, 11);
	ASSERT_EQ(success1, true);
	ASSERT_EQ(success2, false);
	ASSERT_EQ(reg, 703);
}

TEST_F_TRAITS(ModbusMemoryTests, ModbusMemory_ValidRange_True,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	setup_FourRegisters();

	bool result = modbus->validRange(5, 4);

	ASSERT_TRUE(result);
}

TEST_F_TRAITS(ModbusMemoryTests, ModbusMemory_ValidRange_False,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	setup_FourRegisters(true);

	bool result = modbus->validRange(5, 4);

	ASSERT_FALSE(result);
}