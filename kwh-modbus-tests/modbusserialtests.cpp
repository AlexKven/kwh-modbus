#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/modbus.h"
#include "../kwh-modbus/noArduino/modbusmemory.h"
#include "../kwh-modbus/libraries/modbus/modbusserial.h"
#include "../kwh-modbus/mock/mockpublicmodbus.h"
#include "../kwh-modbus/interfaces/iserialstream.h"
#include "../kwh-modbus/noArduino/arduinofunctions.h"
#include "test_helpers.h"

using namespace fakeit;

class ModbusSerialTests : public ::testing::Test
{
protected:
	ModbusSerial<ISerialStream, ArduinoFunctions, ModbusMemory<Modbus>> *modbus = new ModbusSerial<ISerialStream, ArduinoFunctions, ModbusMemory<Modbus>>();
};

TEST_F(ModbusSerialTests, ModbusSerial_Config)
{
	Mock<ISerialStream> mockSerial;
	modbus->config(&mockSerial.get(), 1200, 4);

	
}