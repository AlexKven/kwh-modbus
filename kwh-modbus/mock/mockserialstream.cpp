#include "MockSerialStream.h"
#include <queue>

MockSerialStream::MockSerialStream(std::queue<uint8_t>* _readQueue, std::queue<uint8_t>* _writeQueue)
{
		this->readQueue = _readQueue;
		this->writeQueue = _writeQueue;
}

MockSerialStream::MockSerialStream()
{
	readQueue = new queue<uint8_t>();
	writeQueue = new queue<uint8_t>();
}

MockSerialStream::~MockSerialStream()
{
	delete this->readQueue;
	delete this->writeQueue;
}

void MockSerialStream::begin(long speed)
{

}

bool MockSerialStream::listen()
{

}

void MockSerialStream::end()
{

}

bool MockSerialStream::isListening()
{

}

bool MockSerialStream::stopListening()
{

}

bool MockSerialStream::overflow()
{
	return false;
}

int MockSerialStream::peek()
{
}

size_t MockSerialStream::write(uint8_t bte)
{

}

int MockSerialStream::read()
{

}

int MockSerialStream::available()
{

}

void MockSerialStream::flush()
{

}

MockSerialStream::operator bool()
{
	return true;
}