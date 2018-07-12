#include "../libraries/modbus/ModbusSerial.hpp"
#include "../mock/MockSerialStream.h"
#include "../noArduino/SystemFunctions.h"
#include "../noArduino/ModbusMemory.h"

using namespace std;
int main()
{
	MockSerialStream *stream;
	SystemFunctions *system;
	ModbusSerial<MockSerialStream, SystemFunctions, ModbusMemory> slave;
	slave.config(stream, system, 9600, -1);
}