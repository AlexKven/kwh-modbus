#include "../modbus/ModbusSerial.hpp"

template<typename TSerial, typename TSystemFunctions, typename TBase>
class ModbusMaster : public ModbusSerial<TSerial, TSystemFunctions, TBase>
{
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
};