#pragma once
#include "../interfaces/iserialstream.h"
class MockSerialStream :
	public ISerialStream
{
public:
	MockSerialStream();
	~MockSerialStream();
	void begin(long speed);
	bool listen();
	void end();
	bool isListening();
	bool stopListening();
	bool overflow();
	int peek();
	size_t write(uint8_t bte);
	int read();
	int available();
	void flush();
	operator bool();
};