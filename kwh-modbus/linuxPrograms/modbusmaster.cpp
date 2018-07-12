#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdint>
#include <cstddef>
#include "../noArduino/ArduinoMacros.h"
#include "../libraries/modbusMaster/ModbusMaster.h"
#include "../mock/MockSerialStream.h"
#include "../noArduino/SystemFunctions.h"

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
	MockSerialStream *stream;

	ModbusMaster<MockSerialStream, SystemFunctions> modbus;
	modbus.begin(5, stream);
}