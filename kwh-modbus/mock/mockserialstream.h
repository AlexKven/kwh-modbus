#pragma once
#include "../interfaces/ISerialStream.h"
#include <queue>

using namespace std;
using std::queue;

class MockSerialStream :
	public ISerialStream
{
public:
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
	MockSerialStream(queue<uint8_t> *_readQueue, queue<uint8_t> *_writeQueue);
	MockSerialStream();

	~MockSerialStream();
private:
	queue<uint8_t> *readQueue;
	queue<uint8_t> *writeQueue;
};