#include "stdio.h"
#include "iostream"
#include "ModbusSerial.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "MockSerialStream.h"
#include "ModbusMaster.h"

#include <cstdint>
#include <cstddef>
#include "functions.h"
#include "crc16.h"
#include "word.h"
#include "Modbus.h"

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

	ModbusMaster<MockSerialStream, millis, pinMode, delayMicrosecond, pinMode> modbus;
	modbus.begin(5, stream);

	ModbusSerial<MockSerialStream, pinMode, delayMicrosecond, pinMode> slave;
}