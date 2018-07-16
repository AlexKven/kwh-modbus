#include "MockSerialStream.h"
#include <queue>

MockSerialStream::MockSerialStream(std::queue<uint8_t>* _readQueue, std::queue<uint8_t>* _writeQueue)
{
	externalQueues = true;
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
	if (!externalQueues)
	{
		delete this->readQueue;
		delete this->writeQueue;
	}
}

void MockSerialStream::begin(long _baud)
{
	baud = _baud;
}

bool MockSerialStream::listen()
{
	if (_isListening || baud == -1)
		return false;
	_isListening = true;
	return true;
}

void MockSerialStream::end()
{
	baud = -1;
}

bool MockSerialStream::isListening()
{
	if (baud == -1)
		return false;
	return _isListening;
}

bool MockSerialStream::stopListening()
{
	if (!_isListening || baud == -1)
		return false;
	_isListening = false;
	return true;
}

bool MockSerialStream::overflow()
{
	return false;
}

int MockSerialStream::peek()
{
	if (!(available() > 0))
		return -1;
	else
		return readQueue->front();
}

size_t MockSerialStream::write(uint8_t bte)
{
	if (isListening())
	{
		writeQueue->push(bte);
		return 1;
	}
	return 0;
}

int MockSerialStream::read()
{
	if (!(available() > 0))
		return -1;
	else
	{
		auto res = readQueue->front();
		readQueue->pop();
		return res;
	}
}

int MockSerialStream::available()
{
	if (baud > 0)
		return readQueue->size();
	return 0;
}

void MockSerialStream::flush() {}

MockSerialStream::operator bool()
{
	return true;
}