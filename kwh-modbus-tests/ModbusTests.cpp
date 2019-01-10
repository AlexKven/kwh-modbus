#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/Modbus.h"

TEST_TRAITS(ModbusTests, Modbus_RevWord_1,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	word input = 703;
	word result = Modbus::revWord(input);
	ASSERT_EQ(result, 48898);
}

TEST_TRAITS(ModbusTests, Modbus_RevWord_2,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	word input = 48898;
	word result = Modbus::revWord(input);
	ASSERT_EQ(result, 703);
}

TEST_TRAITS(ModbusTests, Modbus_RevWord_3,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	word input = 7967;
	word result = Modbus::revWord(input);
	ASSERT_EQ(result, 7967);
}