#include "stdio.h"
#include "iostream"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdint>
#include <cstddef>
#include "mock/arduinoMacros.h"
#include "libraries/modbusMaster/ModbusMaster.h"
#include "libraries/modbusSlave/ModbusSerial.h"
#include "mock/MockSerialStream.h"
#include "mock/arduinoFunctions.h"

using namespace std;

void delayMicrosecond(unsigned long length)
{

}

void pinMode(unsigned char pin, unsigned char mode) {}

long millis()
{
	return 0;
}

int main()
{
	Modbus *mb = new Modbus();
	cout << "Hello World!" << endl;
	int test;
	cin >> test;

	MockSerialStream *stream;

	ModbusMaster<MockSerialStream, ArduinoFunctions> modbus;
	modbus.begin(5, stream);

	ModbusSerial<MockSerialStream, ArduinoFunctions> slave;
}