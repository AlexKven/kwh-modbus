#include "stdio.h"
#include "iostream"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdint>
#include <cstddef>
#include "noArduino/ModbusMemory.h"
#include "libraries/modbus/Modbus.h"
#include "libraries/modbus/ModbusSerial.h"
#include "libraries/modbus/ModbusArray.h"
#include "interfaces/ISystemFunctions.h"
#include "mock/MockSerialStream.h"
#define modbus_t ModbusSerial<MockSerialStream, ISystemFunctions, ModbusArray>

using namespace std;

int main()
{
	modbus_t *mb = new modbus_t();
	
}