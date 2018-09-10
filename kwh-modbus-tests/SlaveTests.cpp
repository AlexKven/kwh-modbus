#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/modbus/ModbusArray.h"
#include "../kwh-modbus/libraries/slave/Slave.hpp"
#include "../kwh-modbus/libraries/modbusSlave/ModbusSlave.hpp"
#include "../kwh-modbus/mock/MockSerialStream.h"
#include "WindowsSystemFunctions.h"
#include "test_helpers.h"

#define USE_MOCK Mock<ModbusArray> mock = Mock<ModbusArray>(*modbus)

using namespace fakeit;

typedef ModbusSlave<MockSerialStream, WindowsSystemFunctions, ModbusArray> T_MODBUS;
typedef Slave<ModbusSlave<MockSerialStream, WindowsSystemFunctions, ModbusArray>, WindowsSystemFunctions> T_SLAVE;

class SlaveTests : public ::testing::Test
{
protected:
	word *registerArray;
	MockSerialStream *serial = new MockSerialStream();
	WindowsSystemFunctions *system = new WindowsSystemFunctions();
	T_MODBUS *modbus = new T_MODBUS();
	T_SLAVE *slave = new T_SLAVE();

public:
	void SetUp()
	{
		registerArray = new word[12];
		modbus->init(registerArray, 5, 10, 14);
	}

	void TearDown()
	{
		delete modbus;
		delete[] registerArray;
	}
};