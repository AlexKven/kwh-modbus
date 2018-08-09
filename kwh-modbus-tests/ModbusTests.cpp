#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/Modbus.h"

TEST(ModbusTests, Modbus_RevWord_1)
{
	word input = 703;
	word result = Modbus::revWord(input);
	ASSERT_EQ(result, 48898);
}

TEST(ModbusTests, Modbus_RevWord_2)
{
	word input = 48898;
	word result = Modbus::revWord(input);
	ASSERT_EQ(result, 703);
}

TEST(ModbusTests, Modbus_RevWord_3)
{
	word input = 7967;
	word result = Modbus::revWord(input);
	ASSERT_EQ(result, 7967);
}