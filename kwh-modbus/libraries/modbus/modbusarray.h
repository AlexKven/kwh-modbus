#pragma once
#include "Modbus.h"

#ifdef NO_ARDUINO
#include "../../noArduino/ArduinoMacros.h"
#else
#include "Arduino.h"
#endif

class ModbusArray : public Modbus {
private:
	word *_registerArray;
	word _registerStart;
	word _registerCount;
	word _frameLength = 0;

	bool getRegIndex(word address, word &index);

protected_testable:
	bool Reg(word address, word value);

	word Reg(word address);

	bool resetFrame(word byteLength);

	word getFrameLength();

public:
	void init(word *registerArray, word registerStart, word registerCount, word maxFrameLength);

	bool validRange(word startReg, word numReg);
};