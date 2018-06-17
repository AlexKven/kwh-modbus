#include "stdio.h"
#include "iostream"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdint>
#include <cstddef>
#include "libraries/modbus/modbusmemory.h"
#include "libraries/modbus/modbus.h"
#include "libraries/modbusSlave/modbusserial.h"
#include "mock/mockserialstream.h"
#include "noArduino/arduinofunctions.h"

using namespace std;

int main()
{
	Modbus *mb = new ModbusMemory<ModbusSerial<MockSerialStream, ArduinoFunctions, Modbus>>();
}