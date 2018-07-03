#include "stdio.h"
#include "iostream"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdint>
#include <cstddef>
#include "libraries/modbus/modbus.h"
#include "libraries/modbus/modbusserial.h"
#include "libraries/modbus/modbusarray.h"
#include "interfaces/isystemfunctions.h"
#include "mock/mockserialstream.h"
#define modbus_t ModbusSerial<MockSerialStream, ISystemFunctions, ModbusArray>

using namespace std;

int main()
{
	modbus_t *mb = new modbus_t();
	
}