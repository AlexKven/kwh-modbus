/*
    Modbus.cpp - Source for Modbus Base Library
    Copyright (C) 2014 André Sarmento Barbosa
*/
#include "Modbus.h"
#include <stdlib.h>

Modbus::Modbus() {
}

bool Modbus::resetFrame(word byteLength)
{
	_length = byteLength;
	_frame = (byte*)malloc(_length);
	if (_frame)
		return true;
	else
		return false;
}

byte* Modbus::getFramePtr()
{
	return _frame;
}

word Modbus::getFrameLength()
{
	return _length;
}

bool Modbus::resetFrameRegs(word numRegs)
{
	return resetFrame(numRegs * 2);
}

word Modbus::getFrameReg(word address)
{
	word* wordPtr = (word*)getFramePtr(); 
	return wordPtr[address];
}

bool Modbus::setFrameReg(word address, word value)
{
	if (address * 2 >= getFrameLength())
		return false;
	word* wordPtr = (word*)getFramePtr();
	wordPtr[address] = value;
	return true;
}

bool Modbus::Hreg(word offset, word value) {
    return Reg(offset, value);
}

word Modbus::Hreg(word offset) {
    return Reg(offset);
}

void Modbus::receivePDU(byte* frame) {
    byte fcode  = frame[0];
    word field1 = (word)frame[1] << 8 | (word)frame[2];
    word field2 = (word)frame[3] << 8 | (word)frame[4];

    switch (fcode) {

        case MB_FC_WRITE_REG:
            //field1 = reg, field2 = value
            this->writeSingleRegister(field1, field2);
        break;

        case MB_FC_READ_REGS:
            //field1 = startreg, field2 = numregs
            this->readRegisters(field1, field2);
        break;

        case MB_FC_WRITE_REGS:
            //field1 = startreg, field2 = status
            this->writeMultipleRegisters(frame,field1, field2, frame[5]);
        break;

        default:
            this->exceptionResponse(fcode, MB_EX_ILLEGAL_FUNCTION);
    }
}

void Modbus::exceptionResponse(byte fcode, byte excode) {
    //Clean frame buffer
	resetFrame(2);
	byte* frame = getFramePtr();
	frame[0] = fcode + 0x80;
	frame[1] = excode;

    _reply = MB_REPLY_NORMAL;
}

void Modbus::readRegisters(word startreg, word numregs) {
    //Check value (numregs)
    if (numregs < 0x0001 || numregs > 0x007D) {
        this->exceptionResponse(MB_FC_READ_REGS, MB_EX_ILLEGAL_VALUE);
        return;
    }

    //Check address range
    if (!this->validRange(startreg, numregs)) {
        this->exceptionResponse(MB_FC_READ_REGS, MB_EX_ILLEGAL_ADDRESS);
        return;
    }


    //Clean frame buffer
    if (!resetFrameRegs(numregs)) {
        this->exceptionResponse(MB_FC_READ_REGS, MB_EX_SLAVE_FAILURE);
        return;
    }

    word val;
    word i = 0;
	while(numregs--) {
        //retrieve the value from the register bank for the current register
        val = this->Hreg(startreg + i);
        //write the high byte of the register value
		setFrameReg(i, val);
        i++;
	}

    _reply = MB_REPLY_NORMAL;
}

void Modbus::writeSingleRegister(word reg, word value) {
    //No necessary verify illegal value (EX_ILLEGAL_VALUE) - because using word (0x0000 - 0x0FFFF)
    //Check Address and execute (reg exists?)
    if (!this->Hreg(reg, value)) {
        this->exceptionResponse(MB_FC_WRITE_REG, MB_EX_ILLEGAL_ADDRESS);
        return;
    }

    //Check for failure
    if (this->Hreg(reg) != value) {
        this->exceptionResponse(MB_FC_WRITE_REG, MB_EX_SLAVE_FAILURE);
        return;
    }

    _reply = MB_REPLY_ECHO;
}

void Modbus::writeMultipleRegisters(byte* inputFrame,word startreg, word numoutputs, byte bytecount) {
    //Check value
    if (numoutputs < 0x0001 || numoutputs > 0x007B || bytecount != 2 * numoutputs) {
        this->exceptionResponse(MB_FC_WRITE_REGS, MB_EX_ILLEGAL_VALUE);
        return;
    }

	//Check address range
	if (!this->validRange(startreg, numoutputs)) {
		this->exceptionResponse(MB_FC_WRITE_REGS, MB_EX_ILLEGAL_ADDRESS);
		return;
	}

    word val;
    word i = 0;
	while(i < numoutputs) {
        val = (word)inputFrame[6+i*2] << 8 | (word)inputFrame[7+i*2];
        this->Hreg(startreg + i, val);
        i++;
	}

	//Clean frame buffer
	if (!resetFrame(5)) {
		this->exceptionResponse(MB_FC_WRITE_REGS, MB_EX_SLAVE_FAILURE);
		return;
	}
	byte* frame = getFramePtr();

	frame[0] = MB_FC_WRITE_REGS;
	frame[1] = startreg >> 8;
	frame[2] = startreg & 0x00FF;
	frame[3] = numoutputs >> 8;
	frame[4] = numoutputs & 0x00FF;

    _reply = MB_REPLY_NORMAL;
}


