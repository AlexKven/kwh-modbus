#pragma once

#include "modbus.h"

#ifdef NO_ARDUINO
#include "../../noArduino/arduinofunctions.h"
#include "../../noArduino/arduinomacros.h"
#else
#include "Arduino.h"
#endif

typedef struct TRegister {
	word address;
	word value;
	struct TRegister* next;
} TRegister;

template<typename TBase>
class ModbusMemory : public TBase {
private:
	TRegister * _regs_head;
	TRegister *_regs_last;

	TRegister* searchRegister(word addr) {
		TRegister *reg = _regs_head;
		//if there is no register configured, bail
		if (reg == 0) return(0);
		//scan through the linked list until the end of the list or the register is found.
		//return the pointer.
		do {
			if (reg->address == address) return(reg);
			reg = reg->next;
		} while (reg);
		return nullptr;
	}

protected:
	void addReg(word address, word value = 0) {
		TRegister *newreg;

		newreg = (TRegister *)malloc(sizeof(TRegister));
		newreg->address = address;
		newreg->value = value;
		newreg->next = 0;

		if (_regs_head == 0) {
			_regs_head = newreg;
			_regs_last = _regs_head;
		}
		else {
			//Assign the last register's next pointer to newreg.
			_regs_last->next = newreg;
			//then make temp the last register in the list.
			_regs_last = newreg;
		}
	}

	bool Reg(word address, word value) {
		TRegister *reg;
		//search for the register address
		reg = this->searchRegister(address);
		//if found then assign the register value to the new value.
		if (reg) {
			reg->value = value;
			return true;
		}
		else
			return false;
	}

	word Reg(word address) {
		TRegister *reg;
		reg = this->searchRegister(address);
		if (reg)
			return(reg->value);
		else
			return(0);
	}

public:
	ModbusMemory::ModbusMemory()
	{
		_regs_head = 0;
		_regs_last = 0;
	}
};