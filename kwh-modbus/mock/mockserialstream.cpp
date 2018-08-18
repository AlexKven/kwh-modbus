#include "MockSerialStream.h"
#include <queue>

vector<unsigned int > *MockSerialStream::_randSeeds = nullptr;
int MockSerialStream::_curSeed = 0;

MockSerialStream::MockSerialStream(std::queue<uint8_t>* _readQueue, std::queue<uint8_t>* _writeQueue)
{
	externalQueues = true;
	this->readQueue = _readQueue;
	this->writeQueue = _writeQueue;
}

MockSerialStream::MockSerialStream(queue<uint8_t>* _queue, bool writeOnly)
{
	externalQueues = true;
	if (writeOnly)
	{
		this->readQueue = _queue;
		this->writeQueue = nullptr;
	}
	else
	{
		this->readQueue = nullptr;
		this->writeQueue = _queue;
	}
}

MockSerialStream::MockSerialStream()
{
	readQueue = new queue<uint8_t>();
	writeQueue = new queue<uint8_t>();
}

void MockSerialStream::setPerBitErrorProb(double probability)
{
	_perBitErrorProb = probability;
}

double MockSerialStream::getPerBitErrorProb()
{
	return _perBitErrorProb;
}

MockSerialStream::~MockSerialStream()
{
	if (!externalQueues)
	{
		delete this->readQueue;
		delete this->writeQueue;
	}
	if (random != nullptr)
	{
		delete random;
	}
}

uint8_t MockSerialStream::randomlyErroredByte()
{
	return randomBool(_perBitErrorProb) * 0x01 |
		randomBool(_perBitErrorProb) * 0x02 |
		randomBool(_perBitErrorProb) * 0x04 |
		randomBool(_perBitErrorProb) * 0x08 |
		randomBool(_perBitErrorProb) * 0x10 |
		randomBool(_perBitErrorProb) * 0x20 |
		randomBool(_perBitErrorProb) * 0x40 |
		randomBool(_perBitErrorProb) * 0x80;
}

bool MockSerialStream::randomBool(double trueProbability)
{
	if (trueProbability <= 0)
		return false;
	if (trueProbability >= 1)
		return true;
	if (trueProbability < .4)
	{
		if (randomBool(0.5))
			return randomBool(trueProbability * 2);
		else
			return false;
	}
	int compare = 32768 * trueProbability;
	int randValue = random();
	//srand(randValue);
	return (compare > randValue);
}

void MockSerialStream::randomSeed(unsigned int seed)
{
	if (_randSeeds == nullptr)
		_randSeeds = new vector<unsigned int >();
	_randSeeds->push_back(seed);
}

unsigned int MockSerialStream::random()
{
	auto next = _randSeeds->at(_curSeed);
	next = next * 1103515245 + 12345;
	return (unsigned int)(next / 65536) % 32768;
	_randSeeds->at(_curSeed) = next;
	_curSeed++;
	if (_curSeed > _randSeeds->size())
		_curSeed = 0;
	return (unsigned int)(next / 65536) % 32768;
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
		uint8_t error = randomlyErroredByte();
		auto res = readQueue->front();
		res = res ^ error;
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