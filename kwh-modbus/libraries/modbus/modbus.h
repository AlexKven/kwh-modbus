/*
    Modbus.h - Header for Modbus Base Library
    Copyright (C) 2014 André Sarmento Barbosa
*/

#ifndef MODBUS_H
#define MODBUS_H

#include "ModbusHelpers.h"

#ifdef NO_ARDUINO
#include "../../noArduino/ArduinoMacros.h"
#else
#include "Arduino.h"
#endif
//
//#define MAX_REGS     32
//#define MAX_FRAME   128
//#define USE_HOLDING_REGISTERS_ONLY

//Function Codes
enum {
    MB_FC_READ_COILS       = 0x01, // Read Coils (Output) Status 0xxxx
    MB_FC_READ_INPUT_STAT  = 0x02, // Read Input Status (Discrete Inputs) 1xxxx
    MB_FC_READ_REGS        = 0x03, // Read Holding Registers 4xxxx
    MB_FC_READ_INPUT_REGS  = 0x04, // Read Input Registers 3xxxx
    MB_FC_WRITE_COIL       = 0x05, // Write Single Coil (Output) 0xxxx
    MB_FC_WRITE_REG        = 0x06, // Preset Single Register 4xxxx
    MB_FC_WRITE_COILS      = 0x0F, // Write Multiple Coils (Outputs) 0xxxx
    MB_FC_WRITE_REGS       = 0x10, // Write block of contiguous registers 4xxxx
};

//Exception Codes
enum {
    MB_EX_ILLEGAL_FUNCTION = 0x01, // Function Code not Supported
    MB_EX_ILLEGAL_ADDRESS  = 0x02, // Output Address not exists
    MB_EX_ILLEGAL_VALUE    = 0x03, // Output Value not in Range
    MB_EX_SLAVE_FAILURE    = 0x04, // Slave Deive Fails to process request
};

//Reply Types
enum {
    MB_REPLY_OFF    = 0x01,
    MB_REPLY_ECHO   = 0x02,
    MB_REPLY_NORMAL = 0x03,
};

class Modbus {
    private:
		byte *_frame;
		word  _length;

	protected_testable:
        byte  _reply;

		virtual bool Reg(word address, word value) = 0;
		virtual word Reg(word address) = 0;

		virtual bool resetFrame(word byteLength);
		virtual byte* getFramePtr();
		virtual word getFrameLength();

		bool resetFrameRegs(word numRegs, byte begin);
		word getFrameReg(word address, byte begin);
		virtual bool setFrameReg(word address, word value, byte begin);
    public:
        Modbus();

        bool Hreg(word offset, word value);
        word Hreg(word offset);

		virtual bool validRange(word startReg, word numReg) = 0;

		static word revWord(word input);
};

#endif //MODBUS_H