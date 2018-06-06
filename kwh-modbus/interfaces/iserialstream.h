#pragma once
#include <cstdint>
#include <cstddef>
class ISerialStream
{
public:
	virtual void begin(long speed) = 0;
	virtual bool listen() = 0;
	virtual void end() = 0;
	virtual bool isListening() = 0;
	virtual bool stopListening() = 0;
	virtual bool overflow() = 0;
	virtual int peek() = 0;

	virtual size_t write(uint8_t bte) = 0;
	virtual int read() = 0;
	virtual int available() = 0;
	virtual void flush() = 0;
	virtual operator bool() = 0;
};