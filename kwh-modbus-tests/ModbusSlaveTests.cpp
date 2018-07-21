#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/Modbus.h"
#include "../kwh-modbus/noArduino/ModbusMemory.h"
#include "../kwh-modbus/libraries/modbus/ModbusSerial.hpp"
#include "../kwh-modbus/interfaces/ISerialStream.h"
#include "../kwh-modbus/noArduino/SystemFunctions.h"
#include "../kwh-modbus/mock/MockSerialStream.h"
#include "../kwh-modbus/libraries/modbusSlave/ModbusSlave.hpp"
#include "test_helpers.h"
#include "WindowsFunctions.h"
#include <queue>

#define USE_FAKE_SYSTEM Mock<ISystemFunctions> fakeSystem
#define USE_FAKE_SERIAL Mock<ISerialStream> fakeSerial

#define USE_MOCK_SERIAL std::queue<uint8_t> readQueue; \
std::queue<uint8_t> writeQueue; \
MockSerialStream mockSerial = MockSerialStream(&readQueue, &writeQueue);

using namespace fakeit;

class ModbusSlaveTests : public ::testing::Test
{
protected:
	ModbusSlave<ISerialStream, ISystemFunctions, ModbusMemory> *modbus = new ModbusSlave<ISerialStream, ISystemFunctions, ModbusMemory>();
};

TEST_F(ModbusSlaveTests, ModbusSlave)
{
}