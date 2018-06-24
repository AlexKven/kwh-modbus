#include "../libraries/modbus/modbusserial.h"
#include "../mock/mockserialstream.h"
#include "../noArduino/systemfunctions.h"
#include "../noArduino/modbusmemory.h"

using namespace std;
int main()
{
	MockSerialStream *stream;
	SystemFunctions *system;
	ModbusSerial<MockSerialStream, SystemFunctions, ModbusMemory> slave;
	slave.config(stream, system, 9600, -1);
}