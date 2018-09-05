#pragma once
#include "../interfaces/ISerialStream.h"
#include "../interfaces/ISystemFunctions.h"
#include "../../kwh-modbus/noArduino/TestHelpers.h"
#include <queue>
#include <vector>
#include "../libraries/random/Random.h"

using namespace std;
using std::queue;

class MockSerialStream :
	public ISerialStream
{
public:
	void begin(long _baud);
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
	MockSerialStream(queue<uint8_t> *_readQueue, queue<uint8_t> *_writeQueue);
	MockSerialStream(queue<uint8_t> *_queue, bool writeOnly);
	MockSerialStream();

	void setPerBitErrorProb(double probability);
	double getPerBitErrorProb();

	void setSystem(ISystemFunctions *system);
	ISystemFunctions* getSystem();

	~MockSerialStream();

	bool randomBool(double trueProbability);
	void randomSeed(unsigned int seed1, unsigned int seed2, unsigned int seed3, unsigned int seed4);
	void randomSeed(int seedLength, uint8_t *seed);

	void setReadDelays(unsigned int meanMicros, unsigned int stdDevMicros);
	void getReadDelays(unsigned int &meanMicrosOut, unsigned int &stdDevMicrosOut);

private_testable:
	queue<uint8_t> *_readQueue;
	queue<uint8_t> *_writeQueue;
	deque<unsigned int> *_delayQueue;
	long _baud = -1;
	bool _isListening = true;
	bool _externalQueues = false;
	double  _perBitErrorProb = 0;
	Random _random;
	unsigned int _meanReadDelay = 0;
	unsigned int _stdDevReadDelay = 0;
	unsigned long _lastReadTime = 0;
	ISystemFunctions *_system = nullptr;

	uint8_t randomlyErroredByte();

	static vector<unsigned int > *_randSeeds;
	static int _curSeed;

	void calculateDelays();
};