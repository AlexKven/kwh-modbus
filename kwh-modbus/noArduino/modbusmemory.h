#pragma once
#pragma once
#include "../libraries/modbus/modbus.h"

#include "arduinomacros.h"

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
			if (reg->address == addr) return(reg);
			reg = reg->next;
		} while (reg);
		return nullptr;
	}

protected_testable:
	virtual bool addReg(word address, word value = 0) {
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
		return true;
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
	bool addHreg(word offset, word value = 0) {
		return this->addReg(offset, value);
	}

	virtual bool validRange(word startReg, word numReg)
	{
		for (word i = startReg; i < startReg + numReg; i++)
		{
			if (searchRegister(i) == nullptr)
				return false;
		}
		return true;
	}

	ModbusMemory()
	{
		_regs_head = nullptr;
		_regs_last = nullptr;
	}
};