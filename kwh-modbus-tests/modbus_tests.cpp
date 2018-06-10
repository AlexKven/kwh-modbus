#include "pch.h"

#include "../kwh-modbus/libraries/modbusSlave/modbus.cpp"
#include "../kwh-modbus/libraries/modbusSlave/modbusmemory.h"
#include "../kwh-modbus/mock/mockpublicmodbus.h"

TEST(Modbus, ModbusMemory)
{
	ModbusMemory<MockPublicModbus<Modbus>> *modbus = new ModbusMemory<MockPublicModbus<Modbus>>();
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