#pragma once
#include "../modbus/ModbusSerial.hpp"

template<typename TSerial, typename TSystemFunctions, typename TBase>
class ModbusMaster : public ModbusSerial<TSerial, TSystemFunctions, TBase>
{
private_testable:
	byte _recipientId = 0;

public:
	// Tested with sendPDU
	virtual void send()
	{
		sendPDU();
	}

	virtual bool receive()
	{
		word length = awaitIncomingSerial();
		if (length == 0)
			return false;

		this->resetFrame(length);
		readToFrame();
		return true;
	}

	// Tested
	virtual void sendPDU()
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();

		word crc = calcCrc(frame[0], frame + 1, length - 1);

		beginTransmission();
		writeFromFrame();
		writeWord(crc);
		endTransmission();
	}

	// Tested
	virtual bool setRequest_ReadRegisters(byte recipientId, word regStart, word regCount)
	{
		if (!this->resetFrame(6))
			return false;
		byte *frame = this->getFramePtr();

		_recipientId = recipientId;
		frame[0] = recipientId;
		frame[1] = MB_FC_READ_REGS;
		frame[2] = regStart >> 8;
		frame[3] = regStart;
		frame[4] = regCount >> 8;
		frame[5] = regCount;
		return true;
	}

	// Tested
	virtual bool setRequest_WriteRegister(byte recipientId, word regIndex, word regValue)
	{
		if (!this->resetFrame(6))
			return false;
		byte *frame = this->getFramePtr();

		_recipientId = recipientId;
		frame[0] = recipientId;
		frame[1] = MB_FC_WRITE_REG;
		frame[2] = regIndex >> 8;
		frame[3] = regIndex;
		frame[4] = regValue >> 8;
		frame[5] = regValue;
		return true;
	}

	// Tested
	virtual bool setRequest_WriteRegisters(byte recipientId, word regStart, word regCount, word *regValues)
	{
		if (!this->resetFrameRegs(regCount, 7))
			return false;
		byte *frame = this->getFramePtr();

		_recipientId = recipientId;
		frame[0] = recipientId;
		frame[1] = MB_FC_WRITE_REGS;
		frame[2] = regStart >> 8;
		frame[3] = regStart;
		frame[4] = regCount >> 8;
		frame[5] = regCount;
		frame[6] = regCount * 2;
		for (int i = 0; i < regCount; i++)
		{
			this->setFrameReg(i, revWord(regValues[i]), 7);
		}
		return true;
	}

	// Tested
	virtual bool verifyResponseIntegrity()
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();

		unsigned int crc = ((frame[length - 2] << 8) | frame[length - 1]);

		//Slave Check
		if (frame[0] != _recipientId)
			return false;

		//CRC Check
		if (crc != this->calcCrc(frame[0], frame + 1, length - 3))
			return false;
		return true;
	}

	// Tested
	virtual bool isReadRegsResponse(word &countOut, word* &regsOut)
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();
		if (length < 4 || length % 2 != 0)
			return false;
		if (frame[1] != MB_FC_READ_REGS)
			return false;
		countOut = (length - 4) / 2;
		regsOut = (word*)(frame + 2);
		return true;
	}

	// Tested
	virtual bool isExceptionResponse(byte &fcode, byte &excode)
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();
		if (length != 5)
			return false;
		if (frame[1] >= 128)
		{
			fcode = frame[1] - 128;
			excode = frame[2];
			return true;
		}
		else
			return false;
		return true;
	}

	// Tested
	virtual bool isWriteRegResponse()
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();
		if (length != 4 && length != 6)
		{
			return false;
		}
		if (frame[1] != MB_FC_WRITE_REG)
		{
			return false;
		}
		return true;
	}

	// Tested
	virtual bool isWriteRegsResponse()
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();
		if (length < 4 || length % 2 != 0)
		{
			return false;
		}
		if (frame[1] != MB_FC_WRITE_REGS)
		{
			return false;
		}
		return true;
	}

	virtual bool isAnyWriteResponse()
	{
		return isWriteRegResponse() || isWriteRegsResponse();
	}

	// Not tested, trivial
	virtual byte getRecipientId()
	{
		return _recipientId;
	}
};