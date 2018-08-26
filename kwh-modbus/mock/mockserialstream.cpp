#include "MockSerialStream.h"
#include <queue>
#include "random"

vector<unsigned int > *MockSerialStream::_randSeeds = nullptr;
int MockSerialStream::_curSeed = 0;

MockSerialStream::MockSerialStream(std::queue<uint8_t>* _readQueue, std::queue<uint8_t>* _writeQueue)
{
	_externalQueues = true;
	this->_readQueue = _readQueue;
	this->_writeQueue = _writeQueue;
}

MockSerialStream::MockSerialStream(queue<uint8_t>* _queue, bool writeOnly)
{
	_externalQueues = true;
	if (writeOnly)
	{
		this->_readQueue = _queue;
		this->_writeQueue = nullptr;
	}
	else
	{
		this->_readQueue = nullptr;
		this->_writeQueue = _queue;
	}
	_delayQueue = new queue<unsigned int>();
}

MockSerialStream::MockSerialStream()
{
	_readQueue = new queue<uint8_t>();
	_writeQueue = new queue<uint8_t>();
	_delayQueue = new queue<unsigned int>();
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
	if (!_externalQueues)
	{
		delete this->_readQueue;
		delete this->_writeQueue;
	}
	delete this->_delayQueue;
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

void MockSerialStream::calculateDelays()
{
	if (_readQueue->size() - _delayQueue->size() <= 0)
		return;
	std::normal_distribution<unsigned int> dist(_meanReadDelay, _stdDevReadDelay);
	std::linear_congruential_engine<unsigned int, 16807UL, 0UL, 2147483647UL>
		generator(_random.randomUInt32());
	for (int i = _delayQueue->size(); i < _readQueue->size(); i++)
	{
		_delayQueue->push(dist(generator));
	}
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
	double compare = _random.randomDouble();
	return (trueProbability > compare);
}

void MockSerialStream::randomSeed(unsigned int seed1, unsigned int seed2, unsigned int seed3, unsigned int seed4)
{
	_random.seed(seed1, seed2, seed3, seed4);
}

void MockSerialStream::randomSeed(int seedLength, uint8_t * seed)
{
	_random.seed(seedLength, seed);
}

void MockSerialStream::setReadDelays(unsigned int meanMicros, unsigned int stdDevMicros)
{
}

void MockSerialStream::getReadDelays(unsigned int & meanMicrosOut, unsigned int & stdDevMicrosOut)
{
}

void MockSerialStream::begin(long _baud)
{
	_baud = _baud;
}

bool MockSerialStream::listen()
{
	if (_isListening || _baud == -1)
		return false;
	_isListening = true;
	return true;
}

void MockSerialStream::end()
{
	_baud = -1;
}

bool MockSerialStream::isListening()
{
	if (_baud == -1)
		return false;
	return _isListening;
}

bool MockSerialStream::stopListening()
{
	if (!_isListening || _baud == -1)
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
		return _readQueue->front();
	
}

size_t MockSerialStream::write(uint8_t bte)
{
	if (isListening())
	{
		_writeQueue->push(bte);
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
		if (_meanReadDelay > 0 || _stdDevReadDelay > 0)
		{
			calculateDelays();
			if (_lastReadTime == 0)
			{
				_lastReadTime = system
			}
			auto delay = _delayQueue->front();

		}
		uint8_t error = randomlyErroredByte();
		auto res = _readQueue->front();
		res = res ^ error;
		_readQueue->pop();
		return res;
	}
}

int MockSerialStream::available()
{
	if (_baud > 0)
		return _readQueue->size();
	return 0;
}

void MockSerialStream::flush() {}

MockSerialStream::operator bool()
{
	return true;
}