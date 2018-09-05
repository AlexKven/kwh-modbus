#include "MockSerialStream.h"
#include <queue>
#include "random"
#include <math.h>

vector<unsigned int > *MockSerialStream::_randSeeds = nullptr;
int MockSerialStream::_curSeed = 0;

MockSerialStream::MockSerialStream(std::queue<uint8_t>* readQueue, std::queue<uint8_t>* writeQueue)
{
	_externalQueues = true;
	this->_readQueue = readQueue;
	this->_writeQueue = writeQueue;
	_delayQueue = new deque<unsigned int>();
}

MockSerialStream::MockSerialStream(queue<uint8_t>* queue, bool writeOnly)
{
	_externalQueues = true;
	if (writeOnly)
	{
		this->_readQueue = queue;
		this->_writeQueue = nullptr;
	}
	else
	{
		this->_readQueue = nullptr;
		this->_writeQueue = queue;
	}
	_delayQueue = new std::deque<unsigned int>();
}

MockSerialStream::MockSerialStream()
{
	_readQueue = new queue<uint8_t>();
	_writeQueue = new queue<uint8_t>();
	_delayQueue = new deque<unsigned int>();
}

void MockSerialStream::setPerBitErrorProb(double probability)
{
	_perBitErrorProb = probability;
}

double MockSerialStream::getPerBitErrorProb()
{
	return _perBitErrorProb;
}

void MockSerialStream::setSystem(ISystemFunctions *system)
{
	_system = system;
}

ISystemFunctions * MockSerialStream::getSystem()
{
	return _system;
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
	if (_stdDevReadDelay == 0)
	{
		for (int i = _delayQueue->size(); i < _readQueue->size(); i++)
			_delayQueue->push_back(_meanReadDelay);
		return;
	}
	std::normal_distribution<double> dist(0, 
		(double)_stdDevReadDelay / (double)_meanReadDelay);
	std::linear_congruential_engine<unsigned int, 16807UL, 0UL, 2147483647UL>
		generator(_random.randomUInt32());
	for (int i = _delayQueue->size(); i < _readQueue->size(); i++)
	{
		double multiplier = (1.0 + dist(generator));
		if (multiplier < 0)
			multiplier = 0;
		_delayQueue->push_back(_meanReadDelay * multiplier);
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
	_meanReadDelay = meanMicros;
	_stdDevReadDelay = stdDevMicros;
}

void MockSerialStream::getReadDelays(unsigned int &meanMicrosOut, unsigned int &stdDevMicrosOut)
{
	meanMicrosOut = _meanReadDelay;
	stdDevMicrosOut =_stdDevReadDelay;
}

void MockSerialStream::begin(long baud)
{
	_baud = baud;
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
		if (_delayQueue->size() > 0)
		{
			auto _curReadTime = _system->micros();
			long timediff = _excessTime + _curReadTime - _lastReadTime;
			_lastReadTime = _curReadTime;
			_excessTime = timediff;
			_delayQueue->pop_front();
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
	{
		if ((_meanReadDelay > 0 &&
			_stdDevReadDelay > 0) ||
			_delayQueue->size() > 0)
		{
			calculateDelays();
			auto _curTime = _system->micros();
			if (_lastReadTime == 0)
			{
				_lastReadTime = _curTime;
			}
			long timePassed = _excessTime + _curTime - _lastReadTime;
			int numAvail = 0;
			for (numAvail = 0; 
				timePassed >= 0 && _delayQueue->size() > numAvail;
				numAvail++)
			{
				timePassed -= _delayQueue->at(numAvail);
			}
			if (timePassed < 0 && numAvail > 0)
				numAvail--;
			return min(numAvail, (int)_readQueue->size());
		}
		return _readQueue->size();
	}
	return 0;
}

void MockSerialStream::flush() {}

MockSerialStream::operator bool()
{
	return true;
}