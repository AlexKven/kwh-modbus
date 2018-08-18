#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/mock/MockSerialStream.h"
#include "test_helpers.h"
#include <queue>
#include "WindowsFunctions.h"

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
		MockSerialStream::randomSeed(time(NULL));
	}

	void TearDown()
	{
		delete readQueue;
		delete writeQueue;
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
	int trueCount01 = 0;
	int trueCount1 = 0;
	int trueCount10 = 0;
	int trueCount25 = 0;
	int trueCount50 = 0;
	int trueCount75 = 0;
	int trueCount90 = 0;
	int trueCount99 = 0;
	int trueCount999 = 0;
	int totalCount = 10000;

	for (int i = 0; i < totalCount; i++)
	{
		if (MockSerialStream::randomBool(.001))
			trueCount01++;
		if (MockSerialStream::randomBool(.01))
			trueCount1++;
		if (MockSerialStream::randomBool(.1))
			trueCount10++;
		if (MockSerialStream::randomBool(.25))
			trueCount25++;
		if (MockSerialStream::randomBool(.5))
			trueCount50++;
		if (MockSerialStream::randomBool(.75))
			trueCount75++;
		if (MockSerialStream::randomBool(.9))
			trueCount90++;
		if (MockSerialStream::randomBool(.99))
			trueCount99++;
		if (MockSerialStream::randomBool(.999))
			trueCount999++;
	}

	ASSERT_NEAR(trueCount01, 10, 8);
	ASSERT_NEAR(trueCount1, 100, 25);
	ASSERT_NEAR(trueCount10, 1000, 50);
	ASSERT_NEAR(trueCount25, 2500, 75);
	ASSERT_NEAR(trueCount50, 5000, 100);
	ASSERT_NEAR(trueCount75, 7500, 75);
	ASSERT_NEAR(trueCount90, 9000, 50);
	ASSERT_NEAR(trueCount99, 9900, 25);
	ASSERT_NEAR(trueCount999, 9990, 8);
}

TEST_F(MockSerialStreamTests, MockSerialStream_RandomlyErroredByte)
{
	MockSerialStream stream;
	stream.setPerBitErrorProb(0.083); // Roughly 50% of bytes should be correct
	int errCount = 0;
	int totalCount = 1000;
	WindowsFunctions win;

	//srand(win.RelativeMicroseconds());
	
	for (int i = 0; i < totalCount; i++)
	{
		if (stream.randomlyErroredByte() != 0)
			errCount++;
	}

	ASSERT_NEAR(errCount, 500, 50);
}