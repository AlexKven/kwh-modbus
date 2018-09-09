#include "../../noArduino/TestHelpers.h"
#ifdef NO_ARDUINO
#include "../../noArduino/ArduinoMacros.h"
#endif

template<class M, class S>
class Slave
{
private_testable:
	const byte _majorVersion = 1;
	const byte _minorVersion = 0;
	word _deviceNameLength;
	word _deviceCount;

	M _modbus;
	S _system;
};