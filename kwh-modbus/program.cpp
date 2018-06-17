#include "stdio.h"
#include "iostream"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdint>
#include <cstddef>
#include "noArduino/modbusmemory.h"
#include "libraries/modbus/modbus.h"
#include "libraries/modbusSlave/modbusserial.h"
#include "libraries/modbus/modbusarray.h"
#include "mock/mockserialstream.h"
#include "noArduino/arduinofunctions.h"

using namespace std;

int main()
{
	Modbus *mb = new ModbusArray<ModbusSerial<MockSerialStream, ArduinoFunctions, Modbus>>();
}