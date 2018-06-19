#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/modbus.h"
#include "../kwh-modbus/noArduino/modbusmemory.h"
#include "../kwh-modbus/libraries/modbus/modbusserial.h"
#include "../kwh-modbus/interfaces/iserialstream.h"
#include "../kwh-modbus/noArduino/arduinofunctions.h"
#include "test_helpers.h"

#define USE_MOCK Mock<ISerialStream> mockSerial; \
Fake(Method(mockSerial, begin)); \
modbus->config(&mockSerial.get(), 1200, 4);

using namespace fakeit;

class ModbusSerialTests : public ::testing::Test
{
protected:
	ModbusSerial<ISerialStream, ArduinoFunctions, ModbusMemory<Modbus>> *modbus = new ModbusSerial<ISerialStream, ArduinoFunctions, ModbusMemory<Modbus>>();

	inline void setupMock()
	{
		Mock<ISerialStream> mockSerial;
		Fake(Method(mockSerial, begin));
		modbus->config(&mockSerial.get(), 1200, 4);
	}
};

TEST_F(ModbusSerialTests, ModbusSerial_Config)
{
	USE_MOCK

	Verify(Method(mockSerial, begin).Using(1200)).Once();
}

TEST_F(ModbusSerialTests, ModbusSerial_Available)
{
	USE_MOCK
	When(Method(mockSerial, available)).Return(32);

	int result = modbus->available();

	ASSERT_EQ(result, 32);
	Verify(Method(mockSerial, available)).Once();
}

TEST_F(ModbusSerialTests, ModbusSerial_Flush)
{
	USE_MOCK
	Fake(Method(mockSerial, flush));

	modbus->flush();

	Verify(Method(mockSerial, flush)).Once();
}