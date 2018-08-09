#ifndef NO_ARDUINO
template<typename S>
class ArduinoSerial<S>
{
public:
	ArduinoSerial() {}

	void begin(long speed)
	{
		S.begin(speed);
	}

	bool listen()
	{
		return S.listen();
	}

	void end()
	{
		S.end();
	}

	bool isListening()
	{
		return S.isListening();
	}

	bool stopListening();
	bool overflow();
	int peek();

	size_T write(uint8_t bte);
	int read();
	int available();
	void flish();
	operator bool();
};
#endif