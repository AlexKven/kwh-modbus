#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/Modbus.h"
#include "../kwh-modbus/noArduino/ModbusMemory.h"
#include "../kwh-modbus/libraries/modbus/ModbusSerial.hpp"
#include "../kwh-modbus/interfaces/ISerialStream.h"
#include "../kwh-modbus/noArduino/SystemFunctions.h"
#include "test_helpers.h"
#include "WindowsFunctions.h"

#define USE_MOCK Mock<ISerialStream> mockSerial; \
Mock<ISystemFunctions> mockSystem;

#define CONFIG_MODBUS Fake(Method(mockSerial, begin)); \
modbus->config(&mockSerial.get(), &mockSystem.get(), 1200, -1);

using namespace fakeit;

class ModbusSerialTests : public ::testing::Test
{
protected:
	ModbusSerial<ISerialStream, ISystemFunctions, ModbusMemory> *modbus = new ModbusSerial<ISerialStream, ISystemFunctions, ModbusMemory>();
};

TEST_F(ModbusSerialTests, ModbusSerial_Config)
{
	USE_MOCK;
	CONFIG_MODBUS;

	Verify(Method(mockSerial, begin).Using(1200)).Once();
	ASSERT_EQ(modbus->_t15, 13750);
	ASSERT_EQ(modbus->_t35, 13750 * 3.5);
}

TEST_F(ModbusSerialTests, ModbusSerial_Config_HighBaud)
{
	USE_MOCK;
	Fake(Method(mockSerial, begin));

	modbus->config(&mockSerial.get(), &mockSystem.get(), 20000, -1);

	Verify(Method(mockSerial, begin).Using(20000)).Once();
	ASSERT_EQ(modbus->_t15, 750);
	ASSERT_EQ(modbus->_t35, 750 * 3.5);
}

TEST_F(ModbusSerialTests, ModbusSerial_Config_TXPin)
{
	USE_MOCK;
	Fake(Method(mockSerial, begin));
	Fake(Method(mockSystem, pinMode));
	Fake(Method(mockSystem, digitalWrite));

	modbus->config(&mockSerial.get(), &mockSystem.get(), 1200, 4);

	Verify(Method(mockSerial, begin).Using(1200)).Once();
	Verify(Method(mockSystem, pinMode).Using(4, OUTPUT)).Once();
	Verify(Method(mockSystem, digitalWrite).Using(4, LOW)).Once();
}

TEST_F(ModbusSerialTests, ModbusSerial_Available)
{
	USE_MOCK;
	CONFIG_MODBUS;
	When(Method(mockSerial, available)).Return(32);

	int result = modbus->available();

	ASSERT_EQ(result, 32);
	Verify(Method(mockSerial, available)).Once();
}

TEST_F(ModbusSerialTests, ModbusSerial_Flush)
{
	USE_MOCK;
	CONFIG_MODBUS;
	Fake(Method(mockSerial, flush));

	modbus->flush();

	Verify(Method(mockSerial, flush)).Once();
}

TEST_F(ModbusSerialTests, ModbusSerial_FrameDelay)
{
	USE_MOCK;
	CONFIG_MODBUS;
	Fake(Method(mockSystem, delayMicroseconds));
	unsigned int delay = modbus->_t35;

	modbus->frameDelay();

	Verify(Method(mockSystem, delayMicroseconds).Using(delay)).Once();
}

TEST_F(ModbusSerialTests, ModbusSerial_ByteTimeout)
{
	USE_MOCK;
	CONFIG_MODBUS;
	Fake(Method(mockSystem, delayMicroseconds));
	unsigned int delay = modbus->_t15;

	modbus->byteTimeout();

	Verify(Method(mockSystem, delayMicroseconds).Using(delay)).Once();
}