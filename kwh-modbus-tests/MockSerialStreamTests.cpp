#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/mock/MockSerialStream.h"
#include "test_helpers.h"
#include <queue>
#include "WindowsFunctions.h"
#include "WindowsSystemFunctions.h"
#include <math.h>

using namespace fakeit;

class MockSerialStreamTests : public ::testing::Test
{
public:
	std::queue<uint8_t> *readQueue;
	std::queue<uint8_t> *writeQueue;

	void SetUp()
	{
		readQueue = new std::queue<uint8_t>();
		writeQueue = new std::queue<uint8_t>();
		WindowsFunctions win;
	}

	void TearDown()
	{
		delete readQueue;
		delete writeQueue;
	}

	void seedRandom(MockSerialStream &stream)
	{
		WindowsFunctions win;
		uint8_t seed[16];
		bool success = win.Windows_CryptGenRandom(16, seed);
		if (success)
			stream.randomSeed(16, seed);
		else
			stream.randomSeed(time(NULL), time(NULL), time(NULL), time(NULL));
	}
};

TEST_F(MockSerialStreamTests, MockSerialStream_Uninitalized)
{
	MockSerialStream stream;

	ASSERT_FALSE(stream.isListening());
	ASSERT_EQ(stream.available(), 0);
	ASSERT_EQ(stream.read(), -1);
	ASSERT_EQ(stream.write(42), 0);
	ASSERT_EQ(stream.listen(), false);
}

TEST_F(MockSerialStreamTests, MockSerialStream_End)
{
	MockSerialStream stream;
	stream.begin(1200);
	stream.end();

	ASSERT_FALSE(stream.isListening());
	ASSERT_EQ(stream.available(), 0);
	ASSERT_EQ(stream.read(), -1);
	ASSERT_EQ(stream.write(42), 0);
	ASSERT_EQ(stream.listen(), false);
}

TEST_F(MockSerialStreamTests, MockSerialStream_StartStopListening)
{
	MockSerialStream stream;
	stream.begin(1200);

	ASSERT_TRUE(stream.isListening());
	ASSERT_FALSE(stream.listen());
	ASSERT_TRUE(stream.stopListening());
	ASSERT_FALSE(stream.isListening());
	ASSERT_FALSE(stream.stopListening());
	ASSERT_TRUE(stream.listen());
	ASSERT_TRUE(stream.isListening());
	ASSERT_FALSE(stream.listen());
}

TEST_F(MockSerialStreamTests, MockSerialStream_Initalized)
{
	MockSerialStream stream;
	stream.begin(1200);

	ASSERT_TRUE(stream.isListening());
	ASSERT_EQ(stream.available(), 0);
	ASSERT_EQ(stream.read(), -1);
	ASSERT_EQ(stream.write(42), 1);
	ASSERT_EQ(stream.listen(), false);
}

TEST_F(MockSerialStreamTests, MockSerialStream_Read)
{
	MockSerialStream stream = MockSerialStream(readQueue, writeQueue);
	stream.begin(1200);

	readQueue->push(1);
	readQueue->push(1);
	readQueue->push(2);
	readQueue->push(3);
	readQueue->push(5);
	readQueue->push(8);

	ASSERT_EQ(stream.available(), 6);
	ASSERT_EQ(stream.peek(), 1);
	ASSERT_EQ(stream.read(), 1);
	ASSERT_EQ(stream.read(), 1);
	ASSERT_EQ(stream.read(), 2);
	ASSERT_EQ(stream.peek(), 3);
	ASSERT_EQ(stream.read(), 3);
	ASSERT_EQ(stream.read(), 5);
	ASSERT_EQ(stream.read(), 8);
	ASSERT_EQ(stream.available(), 0);
}

TEST_F(MockSerialStreamTests, MockSerialStream_Write)
{
	MockSerialStream stream = MockSerialStream(readQueue, writeQueue);
	stream.begin(1200);

	ASSERT_EQ(stream.write(1), 1);
	ASSERT_EQ(stream.write(2), 1);
	ASSERT_EQ(stream.write(4), 1);
	ASSERT_EQ(stream.stopListening(), true);
	ASSERT_EQ(stream.write(8), 0);
	ASSERT_EQ(stream.listen(), true);
	ASSERT_EQ(stream.write(16), 1);

	ASSERT_EQ(writeQueue->size(), 4);
	ASSERT_EQ(writeQueue->front(), 1);
	writeQueue->pop();
	ASSERT_EQ(writeQueue->front(), 2);
	writeQueue->pop();
	ASSERT_EQ(writeQueue->front(), 4);
	writeQueue->pop();
	ASSERT_EQ(writeQueue->front(), 16);
}

TEST_F(MockSerialStreamTests, MockSerialStream_ReadWrite_TwoStreams)
{
	MockSerialStream stream1 = MockSerialStream(readQueue, writeQueue);
	MockSerialStream stream2 = MockSerialStream(writeQueue, readQueue);
	stream1.begin(1200);
	stream2.begin(1200);

	stream1.write(2);
	stream1.write(3);
	stream1.write(5);
	stream1.write(7);
	stream1.write(11);
	stream1.write(13);
	stream1.write(17);
	stream1.write(19);
	stream1.stopListening();
	stream1.write(23);

	stream2.write(1);
	stream2.write(1);
	stream2.write(2);
	stream2.write(3);
	stream2.write(5);
	stream2.write(8);
	stream2.write(13);
	stream2.stopListening();
	stream2.write(21);

	ASSERT_EQ(stream1.available(), 7);
	ASSERT_EQ(stream2.available(), 8);

	ASSERT_EQ(stream1.read(), 1);
	ASSERT_EQ(stream1.read(), 1);
	ASSERT_EQ(stream1.read(), 2);
	ASSERT_EQ(stream1.read(), 3);
	ASSERT_EQ(stream1.read(), 5);
	ASSERT_EQ(stream1.read(), 8);
	ASSERT_EQ(stream1.read(), 13);

	ASSERT_EQ(stream2.read(), 2);
	ASSERT_EQ(stream2.read(), 3);
	ASSERT_EQ(stream2.read(), 5);
	ASSERT_EQ(stream2.read(), 7);
	ASSERT_EQ(stream2.read(), 11);
	ASSERT_EQ(stream2.read(), 13);
	ASSERT_EQ(stream2.read(), 17);
	ASSERT_EQ(stream2.read(), 19);

	ASSERT_EQ(stream1.available(), 0);
	ASSERT_EQ(stream2.available(), 0);
}

TEST_F(MockSerialStreamTests, MockSerialStream_RandomBool)
{
	MockSerialStream stream;
	seedRandom(stream);

	int trueCount01 = 0;
	int trueCount1 = 0;
	int trueCount10 = 0;
	int trueCount25 = 0;
	int trueCount50 = 0;
	int trueCount75 = 0;
	int trueCount90 = 0;
	int trueCount99 = 0;
	int trueCount999 = 0;
	int totalCount = 100000;

	for (int i = 0; i < totalCount; i++)
	{
		if (stream.randomBool(.001))
			trueCount01++;
		if (stream.randomBool(.01))
			trueCount1++;
		if (stream.randomBool(.1))
			trueCount10++;
		if (stream.randomBool(.25))
			trueCount25++;
		if (stream.randomBool(.5))
			trueCount50++;
		if (stream.randomBool(.75))
			trueCount75++;
		if (stream.randomBool(.9))
			trueCount90++;
		if (stream.randomBool(.99))
			trueCount99++;
		if (stream.randomBool(.999))
			trueCount999++;
	}

	ASSERT_NEAR(trueCount01, 100, 75);
	ASSERT_NEAR(trueCount1, 1000, 125);
	ASSERT_NEAR(trueCount10, 10000, 175);
	ASSERT_NEAR(trueCount25, 25000, 350);
	ASSERT_NEAR(trueCount50, 50000, 500);
	ASSERT_NEAR(trueCount75, 75000, 350);
	ASSERT_NEAR(trueCount90, 90000, 175);
	ASSERT_NEAR(trueCount99, 99000, 125);
	ASSERT_NEAR(trueCount999, 99900, 75);
}

TEST_F(MockSerialStreamTests, MockSerialStream_RandomlyErroredByte)
{
	MockSerialStream stream;
	seedRandom(stream);
	stream.setPerBitErrorProb(0.083); // Roughly 50% of bytes should be correct
	int errCount = 0;
	int totalCount = 1000;
	WindowsFunctions win;

	for (int i = 0; i < totalCount; i++)
	{
		if (stream.randomlyErroredByte() != 0)
			errCount++;
	}

	ASSERT_NEAR(errCount, 500, 50);
}

TEST_F(MockSerialStreamTests, MockSerialStream_DelayDistribution)
{
	MockSerialStream stream = MockSerialStream(readQueue, writeQueue);
	WindowsSystemFunctions wsf();
	seedRandom(stream);
	stream.setReadDelays(100, 40);
	int totalCount = 20000;

	for (int i = 0; i < totalCount; i++)
	{
		readQueue->push(0);
	}
	stream.calculateDelays();

	double mean = 0;
	double stdDev = 0;

	for (int i = 0; i < totalCount; i++)
	{
		unsigned int val = stream._delayQueue->front();
		mean += (double)val / (double)totalCount;
		stdDev += (double)(val * val) / (double)totalCount;
		stream._delayQueue->pop_front();
	}

	stdDev -= mean * mean;
	stdDev = sqrt(stdDev);

	ASSERT_NEAR(mean, 100, 5);
	ASSERT_NEAR(stdDev, 40, 5);
}

TEST_F(MockSerialStreamTests, MockSerialStream_DelayQueue)
{
	MockSerialStream stream = MockSerialStream(readQueue, writeQueue);
	stream.begin(1200);
	WindowsSystemFunctions wsf;
	stream.setSystem(&wsf);
	seedRandom(stream);
	int totalCount = 6;

	for (int i = 0; i < totalCount; i++)
	{
		readQueue->push(i);
	}

	stream._delayQueue->push_back(25000);
	stream._delayQueue->push_back(30000);
	stream._delayQueue->push_back(20000);
	stream._delayQueue->push_back(50000);
	stream._delayQueue->push_back(100000);

	ASSERT_EQ(stream.available(), 0)	<< "Assert 0";
	ASSERT_EQ(stream.read(), -1)		<< "Assert 1";
	wsf.delay(30);
	ASSERT_EQ(stream.available(), 1)	<< "Assert 2";
	ASSERT_EQ(stream.read(), 0)			<< "Assert 3";
	ASSERT_EQ(stream.available(), 0)	<< "Assert 4";
	ASSERT_EQ(stream.read(), -1)		<< "Assert 5";
	wsf.delay(15);
	ASSERT_EQ(stream.available(), 0)	<< "Assert 6";
	ASSERT_EQ(stream.read(), -1)		<< "Assert 7";
	wsf.delay(15);
	ASSERT_EQ(stream.available(), 1)	<< "Assert 8";
	ASSERT_EQ(stream.read(), 1)			<< "Assert 9";
	ASSERT_EQ(stream.available(), 0)	<< "Assert 10";
	ASSERT_EQ(stream.read(), -1)		<< "Assert 11";
	wsf.delay(15);
	ASSERT_EQ(stream.available(), 0)	<< "Assert 12";
	ASSERT_EQ(stream.read(), -1)		<< "Assert 13";
	wsf.delay(15);
	ASSERT_EQ(stream.available(), 1)	<< "Assert 14";
	ASSERT_EQ(stream.read(), 2)			<< "Assert 15";
	ASSERT_EQ(stream.available(), 0)	<< "Assert 16";
	ASSERT_EQ(stream.read(), -1)		<< "Assert 17";
	wsf.delay(25);
	ASSERT_EQ(stream.available(), 0)	<< "Assert 18";
	ASSERT_EQ(stream.read(), -1)		<< "Assert 19";
	wsf.delay(25);
	ASSERT_EQ(stream.available(), 1)	<< "Assert 20";
	ASSERT_EQ(stream.read(), 3)			<< "Assert 21";
	ASSERT_EQ(stream.available(), 0)	<< "Assert 22";
	ASSERT_EQ(stream.read(), -1)		<< "Assert 23";
	wsf.delay(25);
	ASSERT_EQ(stream.available(), 0)	<< "Assert 24";
	ASSERT_EQ(stream.read(), -1)		<< "Assert 25";
	wsf.delay(25);
	ASSERT_EQ(stream.available(), 0)	<< "Assert 26";
	ASSERT_EQ(stream.read(), -1)		<< "Assert 27";
	wsf.delay(25);
	ASSERT_EQ(stream.available(), 0)	<< "Assert 28";
	ASSERT_EQ(stream.read(), -1)		<< "Assert 29";
	wsf.delay(25);
	ASSERT_EQ(stream.available(), 2)	<< "Assert 30";
	ASSERT_EQ(stream.read(), 4)			<< "Assert 31";
	ASSERT_EQ(stream.available(), 1)	<< "Assert 32";
	ASSERT_EQ(stream.read(), 5)			<< "Assert 33";
	//wsf.delay(25);
	//ASSERT_EQ(stream.available(), 0)	<< "Assert 34";
	//ASSERT_EQ(stream.read(), -1)		<< "Assert 35";
	//wsf.delay(25);
	//ASSERT_EQ(stream.available(), 0)	<< "Assert 36";
	//ASSERT_EQ(stream.read(), -1)		<< "Assert 37";
	//wsf.delay(25);
	//ASSERT_EQ(stream.available(), 0)	<< "Assert 38";
	//ASSERT_EQ(stream.read(), -1)		<< "Assert 39";
	//wsf.delay(25);
	//ASSERT_EQ(stream.available(), 1)	<< "Assert 40";
	//ASSERT_EQ(stream.read(), 5)			<< "Assert 41";
	//ASSERT_EQ(stream.available(), 0)	<< "Assert 42";
	//ASSERT_EQ(stream.read(), -1)		<< "Assert 43";
}