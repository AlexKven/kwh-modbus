#include "../modbus/ModbusSerial.hpp"

template<typename TSerial, typename TSystemFunctions, typename TBase>
class ModbusMaster : public ModbusSerial<TSerial, TSystemFunctions, TBase>
{
private_testable:
	byte _recipientId = 0;

public:
	void send()
	{
		sendPDU();
	}

	bool receive()
	{
		word length = awaitIncomingSerial();
		if (length == 0)
			return false;

		this->resetFrame(length);
		readToFrame();
		return true;
	}

	void sendPDU()
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();

		word crc = calcCrc(frame[0], frame + 1, length - 1);

		beginTransmission();
		writeFromFrame();
		writeWord(crc);
		endTransmission();
	}

	bool setRequest_ReadRegisters(byte recipientId, word regStart, word regCount)
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

	bool setRequest_WriteRegister(byte recipientId, word regIndex, word regValue)
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

	bool verifyResponseIntegrity()
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();

		unsigned int crc = ((frame[length - 2] << 8) | frame[length - 1]);

		//Slave Check
		if (frame[0] != _recipientId)
			return false;

		//CRC Check
		if (crc != this->calcCrc(frame[0], frame + 2, length - 4))
			return false;
	}

	bool isReadRegsResponse(word &countOut, word* &regsOut)
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();
		if (length < 4 || length % 2 != 0)
			return false;
		if (frame[1] != MB_FC_READ_REGS)
			return false;
		countOut = (length - 4) / 2;
		regsOut = (word*)(frame + 2);
	}

	bool isExceptionResponse(byte &fcode, byte &excode)
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
	}

	bool isWriteRegResponse()
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();
		if (length < 4 || length % 2 != 0)
			return false;
		if (frame[1] != MB_FC_WRITE_REG)
			return false;
	}
};