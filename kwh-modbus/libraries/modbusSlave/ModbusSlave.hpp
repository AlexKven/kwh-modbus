#include "../modbus/ModbusSerial.hpp"

template<typename TSerial, typename TSystemFunctions, typename TBase>
class ModbusSlave : public ModbusSerial<TSerial, TSystemFunctions, TBase>
{
private:
	byte  _slaveId;
	byte _replyFunctionCode;
	bool _exceptionResponse;

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
		_exceptionResponse = !this->receivePDU(frame + 1);
		//No reply to Broadcasts
		if (address == 0xFF) this->_reply = MB_REPLY_OFF;
		return true;
	}

	bool sendPDU()
	{
		beginTransmission();

		//Send slaveId
		write(_slaveId);

		//Resend function code if no exception response
		if (!_exceptionResponse)
			write(_replyFunctionCode);

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

	bool echo()
	{
		byte *frame = this->getFramePtr();
		word length = this->getFrameLength();
		byte i;

		beginTransmission();

		writeFromFrame();

		endTransmission();
		return true;
	}
};