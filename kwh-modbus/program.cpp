#include "stdio.h"
#include "iostream"
#include "Libraries/modbus-slave/ModbusSerial/ModbusSerial.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "MockSerialStream.h"
#include "ModbusMaster.h"

using namespace std;

void delayMicrosecond(unsigned long length)
{

}

void pinMode(unsigned char pin, unsigned char mode) {}

int main()
{
	Modbus *mb = new Modbus();
	cout << "Hello World!" << endl;
	int test;
	cin >> test;

	MockSerialStream *stream;

	ModbusMaster<MockSerialStream, pinMode, delayMicrosecond, pinMode> modbus;
	modbus.begin(5, stream);
}