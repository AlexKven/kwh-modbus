#include "stdio.h"
#include "iostream"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdint>
#include <cstddef>
#include "noArduino/modbusmemory.h"
#include "libraries/modbus/modbus.h"
#include "libraries/modbus/modbusserial.h"
#include "libraries/modbus/modbusarray.h"
#include "interfaces/isystemfunctions.h"
#include "mock/mockserialstream.h"
#define modbus_t ModbusArray<ModbusSerial<MockSerialStream, ISystemFunctions, Modbus>>

using namespace std;

int main()
{
	modbus_t *mb = new modbus_t();
	
}