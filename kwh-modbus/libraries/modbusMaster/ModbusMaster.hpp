#include "../modbus/ModbusSerial.hpp"

template<typename TSerial, typename TSystemFunctions, typename TBase>
class ModbusMaster : public ModbusSerial<TSerial, TSystemFunctions, TBase>
{
public:
	bool task()
	{
		sendPDU();
		word length;
		while (length = awaitIncomingSerial() == 0);

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