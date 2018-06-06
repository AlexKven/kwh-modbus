#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdint>
#include <cstddef>
#include "../noArduino/arduinomacros.h"
#include "../libraries/modbusSlave/modbusserial.h"
#include "../mock/mockserialstream.h"
#include "../noArduino/arduinofunctions.h"

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
	ModbusSerial<MockSerialStream, ArduinoFunctions> slave;
	slave.config(stream, 9600);
}