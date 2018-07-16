#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/mock/MockSerialStream.h"
#include "test_helpers.h"
#include <queue>

#define USE_MOCK Mock<ModbusMemory> mock = Mock<ModbusMemory>(*modbus);

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