#pragma once
#include "../interfaces/ISerialStream.h"
#include "../../kwh-modbus/noArduino/TestHelpers.h"
#include <queue>
#include <random>

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

	~MockSerialStream();

	int numErrors = 0;
private_testable:
	queue<uint8_t> *readQueue;
	queue<uint8_t> *writeQueue;
	long baud = -1;
	bool _isListening = true;
	bool externalQueues = false;
	double  _perBitErrorProb = 0;

	uint8_t randomlyErroredByte();

	static bool randomBool(double trueProbability);
};