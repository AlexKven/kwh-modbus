#pragma once
#pragma once
#include "../libraries/modbus/modbus.h"

#include "arduinomacros.h"

typedef struct TRegister {
	word address;
	word value;
	struct TRegister* next;
} TRegister;

class ModbusMemory : public Modbus {
private:
	TRegister * _regs_head;
	TRegister *_regs_last;

	TRegister* searchRegister(word addr);

protected_testable:
	virtual bool addReg(word address, word value = 0);

	bool Reg(word address, word value);

	word Reg(word address);

public:
	bool addHreg(word offset, word value = 0);

	virtual bool validRange(word startReg, word numReg);

	ModbusMemory();
};