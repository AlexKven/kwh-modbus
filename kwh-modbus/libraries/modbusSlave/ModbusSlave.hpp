#include "../modbus/ModbusSerial.hpp"

template<typename TSerial, typename TSystemFunctions, typename TBase>
class ModbusSlave : public ModbusSerial<TSerial, TSystemFunctions, TBase>
{
private:
	byte  _slaveId;
	byte _replyFunctionCode;


	void exceptionResponse(byte fcode, byte excode) {
		//Clean frame buffer
		resetFrame(2);
		byte* frame = getFramePtr();
		frame[0] = fcode + 0x80;
		frame[1] = excode;

		_reply = MB_REPLY_NORMAL;
	}

	bool readRegisters(word startreg, word numregs) {
		//Check value (numregs)
		if (numregs < 0x0001 || numregs > 0x007D) {
			this->exceptionResponse(MB_FC_READ_REGS, MB_EX_ILLEGAL_VALUE);
			return false;
		}

		//Check address range
		if (!this->validRange(startreg, numregs)) {
			this->exceptionResponse(MB_FC_READ_REGS, MB_EX_ILLEGAL_ADDRESS);
			return false;
		}


		//Clean frame buffer
		if (!resetFrameRegs(numregs, 1)) {
			this->exceptionResponse(MB_FC_READ_REGS, MB_EX_SLAVE_FAILURE);
			return false;
		}

		getFramePtr()[0] = MB_FC_READ_REGS;
		word val;
		word i = 0;
		while (numregs--) {
			//retrieve the value from the register bank for the current register
			val = this->Hreg(startreg + i);
			//write the high byte of the register value
			setFrameReg(i, val, 1);
			i++;
		}

		_reply = MB_REPLY_NORMAL;
		return true;
	}

	bool writeSingleRegister(word reg, word value) {
		//No necessary verify illegal value (EX_ILLEGAL_VALUE) - because using word (0x0000 - 0x0FFFF)
		//Check Address and execute (reg exists?)
		if (!this->Hreg(reg, value)) {
			this->exceptionResponse(MB_FC_WRITE_REG, MB_EX_ILLEGAL_ADDRESS);
			return false;
		}

		//Check for failure
		if (this->Hreg(reg) != value) {
			this->exceptionResponse(MB_FC_WRITE_REG, MB_EX_SLAVE_FAILURE);
			return false;
		}

		_reply = MB_REPLY_ECHO;
		return true;
	}

	bool writeMultipleRegisters(byte* inputFrame, word startreg, word numoutputs, byte bytecount, byte begin = 6) {
		//Check value
		if (numoutputs < 0x0001 || numoutputs > 0x007B || bytecount != 2 * numoutputs) {
			this->exceptionResponse(MB_FC_WRITE_REGS, MB_EX_ILLEGAL_VALUE);
			return false;
		}

		//Check address range
		if (!this->validRange(startreg, numoutputs)) {
			this->exceptionResponse(MB_FC_WRITE_REGS, MB_EX_ILLEGAL_ADDRESS);
			return false;
		}

		word val;
		word i = 0;
		while (i < numoutputs) {
			val = (word)inputFrame[begin + i * 2] << 8 | (word)inputFrame[begin + 1 + i * 2];
			this->Hreg(startreg + i, val);
			i++;
		}

		//Clean frame buffer
		if (!resetFrame(5)) {
			this->exceptionResponse(MB_FC_WRITE_REGS, MB_EX_SLAVE_FAILURE);
			return false;
		}
		byte* frame = getFramePtr();

		frame[0] = MB_FC_WRITE_REGS;
		frame[1] = startreg >> 8;
		frame[2] = startreg & 0x00FF;
		frame[3] = numoutputs >> 8;
		frame[4] = numoutputs & 0x00FF;

		_reply = MB_REPLY_NORMAL;
		return true;
	}

public:
	bool setSlaveId(byte slaveId)
	{
		_slaveId = slaveId;
		return true;
	}

	byte getSlaveId()
	{
		return _slaveId;
	}

	bool task()
	{
		word length = awaitIncomingSerial();
		if (length == 0)
			return false;

		if (!this->resetFrame(length))
			return false;
		readToFrame();
		
		if (this->readInputFrame())
		{
			if (this->_reply == MB_REPLY_NORMAL)
				this->sendPDU();
			else
				if (this->_reply == MB_REPLY_ECHO)
					this->echo();
		}

		this->resetFrame(0);
		return true;
	}

	bool readInputFrame()
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();
		// first byte of frame = address
		byte address = frame[0];
		//Last two bytes = crc
		unsigned int crc = ((frame[length - 2] << 8) | frame[length - 1]);

		//Slave Check
		if (address != 0xFF && address != this->getSlaveId()) {
			return false;
		}

		//CRC Check
		if (crc != this->calcCrc(frame[0], frame + 1, length - 3)) {
			return false;
		}

		_replyFunctionCode = frame[1];
		//PDU starts after first byte
		//framesize PDU = framesize - address(1) - crc(2)
		this->receivePDU(frame + 1);
		//No reply to Broadcasts
		if (address == 0xFF) this->_reply = MB_REPLY_OFF;
		return true;
	}

	bool receivePDU(byte* frame) {
		byte fcode = frame[0];
		word field1 = (word)frame[1] << 8 | (word)frame[2];
		word field2 = (word)frame[3] << 8 | (word)frame[4];

		switch (fcode) {

		case MB_FC_WRITE_REG:
			//field1 = reg, field2 = value
			return this->writeSingleRegister(field1, field2);
			break;

		case MB_FC_READ_REGS:
			//field1 = startreg, field2 = numregs
			return this->readRegisters(field1, field2);
			break;

		case MB_FC_WRITE_REGS:
			//field1 = startreg, field2 = status
			return this->writeMultipleRegisters(frame, field1, field2, frame[5]);
			break;

		default:
			this->exceptionResponse(fcode, MB_EX_ILLEGAL_FUNCTION);
			return false;
		}
	}

	bool sendPDU()
	{
		beginTransmission();

		//Send slaveId
		write(_slaveId);

		////Resend function code if no exception response
		//if (!_exceptionResponse)
		//	write(_replyFunctionCode);

		//Send PDU
		writeFromFrame();

		//Send CRC
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();
		word crc = calcCrc(_slaveId, frame, length);
		writeWord(crc);

		endTransmission();
		return true;
	}

	bool echo(bool fCodeOnly = true)
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();
		byte i;

		beginTransmission();

		if (fCodeOnly)
		{
			writeFromFrame(2);
			word crc = calcCrc(_slaveId, frame + 1, 1);
			writeWord(crc);
		}
		else
			writeFromFrame();

		endTransmission();
		return true;
	}
};