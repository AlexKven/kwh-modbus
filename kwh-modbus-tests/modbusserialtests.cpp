#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/modbus/Modbus.h"
#include "../kwh-modbus/noArduino/ModbusMemory.h"
#include "../kwh-modbus/libraries/modbus/ModbusSerial.hpp"
#include "../kwh-modbus/interfaces/ISerialStream.h"
#include "../kwh-modbus/noArduino/SystemFunctions.h"
#include "../kwh-modbus/mock/MockSerialStream.h"
#include "test_helpers.h"
#include "WindowsFunctions.h"
#include <queue>

#define USE_FAKE Mock<ISerialStream> fakeSerial; \
Mock<ISystemFunctions> fakeSystem;

#define USE_FAKE_SYSTEM Mock<ISystemFunctions> fakeSystem
#define USE_FAKE_SERIAL Mock<ISerialStream> fakeSerial

#define USE_MOCK_SERIAL std::queue<uint8_t> readQueue; \
std::queue<uint8_t> writeQueue; \
MockSerialStream mockSerial = MockSerialStream(&readQueue, &writeQueue);

#define CONFIG_MODBUS_FAKE_ALL Fake(Method(fakeSerial, begin)); \
modbus->config(&fakeSerial.get(), &fakeSystem.get(), 1200, -1)

#define READ_QUEUE_PUSH_FIBONACCI readQueue.push(1); \
readQueue.push(1); \
readQueue.push(2); \
readQueue.push(3); \
readQueue.push(5); \
readQueue.push(8); \
readQueue.push(13); \
readQueue.push(21); \
readQueue.push(34); \
readQueue.push(55); \
readQueue.push(89); \
readQueue.push(144)

using namespace fakeit;

class ModbusSerialTests : public ::testing::Test
{
protected:
	ModbusSerial<ISerialStream, ISystemFunctions, ModbusMemory> *modbus = new ModbusSerial<ISerialStream, ISystemFunctions, ModbusMemory>();
};

TEST_F(ModbusSerialTests, ModbusSerial_Config)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;

	Verify(Method(fakeSerial, begin).Using(1200)).Once();
	ASSERT_EQ(modbus->_t15, 13750);
	ASSERT_EQ(modbus->_t35, 13750 * 3.5);
}

TEST_F(ModbusSerialTests, ModbusSerial_Config_HighBaud)
{
	USE_FAKE;
	Fake(Method(fakeSerial, begin));

	modbus->config(&fakeSerial.get(), &fakeSystem.get(), 20000, -1);

	Verify(Method(fakeSerial, begin).Using(20000)).Once();
	ASSERT_EQ(modbus->_t15, 750);
	ASSERT_EQ(modbus->_t35, 750 * 3.5);
}

TEST_F(ModbusSerialTests, ModbusSerial_Config_TXPin)
{
	USE_FAKE;
	Fake(Method(fakeSerial, begin));
	Fake(Method(fakeSystem, pinMode));
	Fake(Method(fakeSystem, digitalWrite));

	modbus->config(&fakeSerial.get(), &fakeSystem.get(), 1200, 4);

	Verify(Method(fakeSerial, begin).Using(1200)).Once();
	Verify(Method(fakeSystem, pinMode).Using(4, OUTPUT)).Once();
	Verify(Method(fakeSystem, digitalWrite).Using(4, LOW)).Once();
}

TEST_F(ModbusSerialTests, ModbusSerial_Available)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, available)).Return(32);

	int result = modbus->available();

	ASSERT_EQ(result, 32);
	Verify(Method(fakeSerial, available)).Once();
}

TEST_F(ModbusSerialTests, ModbusSerial_Flush)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	Fake(Method(fakeSerial, flush));

	modbus->flush();

	Verify(Method(fakeSerial, flush)).Once();
}

TEST_F(ModbusSerialTests, ModbusSerial_Read)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, read)).Return(5);

	auto result = modbus->read();

	Verify(Method(fakeSerial, read)).Once();
	ASSERT_EQ(result, 5);
}

TEST_F(ModbusSerialTests, ModbusSerial_Read_Success)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, read)).Return(5);

	byte result = 3;
	bool success = modbus->read(result);

	Verify(Method(fakeSerial, read)).Once();
	ASSERT_EQ(result, 5);
	ASSERT_TRUE(success);
}

TEST_F(ModbusSerialTests, ModbusSerial_Read_Failure)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, read)).Return(-1);

	byte result = 3;
	bool success = modbus->read(result);

	Verify(Method(fakeSerial, read)).Once();
	ASSERT_EQ(result, 3);
	ASSERT_FALSE(success);
}

TEST_F(ModbusSerialTests, ModbusSerial_Write_Success)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, write)).Return(1);

	byte write = 5;
	bool success = modbus->write(write);

	Verify(Method(fakeSerial, write).Using(5)).Once();
	ASSERT_TRUE(success);
}

TEST_F(ModbusSerialTests, ModbusSerial_Write_Failure)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, write)).Return(0);

	byte write = 5;
	bool success = modbus->write(write);

	Verify(Method(fakeSerial, write).Using(5)).Once();
	ASSERT_FALSE(success);
}

TEST_F(ModbusSerialTests, ModbusSerial_FrameDelay)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	Fake(Method(fakeSystem, delayMicroseconds));
	unsigned int delay = modbus->_t35;

	modbus->frameDelay();

	Verify(Method(fakeSystem, delayMicroseconds).Using(delay)).Once();
}

TEST_F(ModbusSerialTests, ModbusSerial_ByteDelay)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	Fake(Method(fakeSystem, delayMicroseconds));
	unsigned int delay = modbus->_t15;

	modbus->byteDelay();

	Verify(Method(fakeSystem, delayMicroseconds).Using(delay)).Once();
}

TEST_F(ModbusSerialTests, ModbusSerial_AwaitIncomingSerial_Nothing)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, available)).Return(0);

	int result = modbus->awaitIncomingSerial();

	ASSERT_EQ(result, 0);
}

TEST_F(ModbusSerialTests, ModbusSerial_AwaitIncomingSerial_Something)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, available))
		.Return(1).Return(1)
		.Return(3).Return(3)
		.Return(8).Return(8)
		.Return(21).Return(21)
		.Return(22).Return(22)
		.Return(22);
	Fake(Method(fakeSystem, delayMicroseconds));
	unsigned int delay = modbus->_t15;

	int result = modbus->awaitIncomingSerial();

	ASSERT_EQ(result, 22);
	Verify(Method(fakeSystem, delayMicroseconds).Using(delay)).Exactly(5_Times);
}

TEST_F(ModbusSerialTests, ModbusSerial_readToFrame_All)
{
	USE_FAKE_SYSTEM;
	USE_MOCK_SERIAL;
	READ_QUEUE_PUSH_FIBONACCI;
	modbus->config(&mockSerial, &fakeSystem.get(), 1200, -1);


	modbus->resetFrame(9);
	int length = modbus->readToFrame();

	auto frame = modbus->getFramePtr();
	ASSERT_EQ(length, 9);
	assertArrayEq(frame,
		(byte)1,
		(byte)1,
		(byte)2,
		(byte)3,
		(byte)5,
		(byte)8,
		(byte)13,
		(byte)21,
		(byte)34);
}

TEST_F(ModbusSerialTests, ModbusSerial_readToFrame_Incomplete)
{
	USE_FAKE_SYSTEM;
	USE_MOCK_SERIAL;
	modbus->config(&mockSerial, &fakeSystem.get(), 1200, -1);
	readQueue.push(1);
	readQueue.push(1);
	readQueue.push(2);
	readQueue.push(3);
	readQueue.push(5);
	readQueue.push(8);

	modbus->resetFrame(9);
	int length = modbus->readToFrame();

	auto frame = modbus->getFramePtr();
	ASSERT_EQ(length, 6);
	assertArrayEq(frame,
		(byte)1,
		(byte)1,
		(byte)2,
		(byte)3,
		(byte)5,
		(byte)8);
}

TEST_F(ModbusSerialTests, ModbusSerial_readToFrame_Lower_Length)
{
	USE_FAKE_SYSTEM;
USE_MOCK_SERIAL;
READ_QUEUE_PUSH_FIBONACCI;
modbus->config(&mockSerial, &fakeSystem.get(), 1200, -1);

readQueue.push(1);
readQueue.push(1);
readQueue.push(2);
readQueue.push(3);
readQueue.push(5);
readQueue.push(8);
readQueue.push(13);
readQueue.push(21);
readQueue.push(34);

modbus->resetFrame(9);
int length = modbus->readToFrame(5);

auto frame = modbus->getFramePtr();
ASSERT_EQ(length, 5);
assertArrayEq(frame,
(byte)1,
(byte)1,
(byte)2,
(byte)3,
(byte)5);
}

TEST_F(ModbusSerialTests, ModbusSerial_readToFrame_Offset)
{
	USE_FAKE_SYSTEM;
	USE_MOCK_SERIAL;
	READ_QUEUE_PUSH_FIBONACCI;
	modbus->config(&mockSerial, &fakeSystem.get(), 1200, -1);

	modbus->resetFrame(9);
	int length = modbus->readToFrame(-1, 3);

	auto frame = modbus->getFramePtr();
	ASSERT_EQ(length, 6);
	assertArrayEq(frame + 3,
		(byte)1,
		(byte)1,
		(byte)2,
		(byte)3,
		(byte)5,
		(byte)8);
}

TEST_F(ModbusSerialTests, ModbusSerial_readToFrame_Offset_Lower_Length)
{
	USE_FAKE_SYSTEM;
	USE_MOCK_SERIAL;
	READ_QUEUE_PUSH_FIBONACCI;
	modbus->config(&mockSerial, &fakeSystem.get(), 1200, -1);

	readQueue.push(1);
	readQueue.push(1);
	readQueue.push(2);
	readQueue.push(3);
	readQueue.push(5);
	readQueue.push(8);
	readQueue.push(13);
	readQueue.push(21);
	readQueue.push(34);

	modbus->resetFrame(9);
	int length = modbus->readToFrame(5, 3);

	auto frame = modbus->getFramePtr();
	ASSERT_EQ(length, 5);
	assertArrayEq(frame + 3,
		(byte)1,
		(byte)1,
		(byte)2,
		(byte)3,
		(byte)5);
}

TEST_F(ModbusSerialTests, ModbusSerial_writeFromFrame_All)
{
	USE_FAKE_SYSTEM;
	USE_MOCK_SERIAL;
	modbus->config(&mockSerial, &fakeSystem.get(), 1200, -1);

	modbus->resetFrame(9);
	auto frame = modbus->getFramePtr();
	setArray(frame,
		(byte)2,
		(byte)3,
		(byte)5,
		(byte)7,
		(byte)11,
		(byte)13,
		(byte)17,
		(byte)19,
		(byte)23);

	int length = modbus->writeFromFrame();

	ASSERT_EQ(length, 9);
	ASSERT_EQ(writeQueue.front(), 2);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 3);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 5);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 7);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 11);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 13);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 17);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 19);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 23);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.size(), 0);
}

TEST_F(ModbusSerialTests, ModbusSerial_writeFromFrame_Lower_Length)
{
	USE_FAKE_SYSTEM;
	USE_MOCK_SERIAL;
	modbus->config(&mockSerial, &fakeSystem.get(), 1200, -1);

	modbus->resetFrame(9);
	auto frame = modbus->getFramePtr();
	setArray(frame,
		(byte)2,
		(byte)3,
		(byte)5,
		(byte)7,
		(byte)11,
		(byte)13,
		(byte)17,
		(byte)19,
		(byte)23);

	int length = modbus->writeFromFrame(5);

	ASSERT_EQ(length, 5);
	ASSERT_EQ(writeQueue.front(), 2);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 3);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 5);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 7);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 11);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.size(), 0);
}

TEST_F(ModbusSerialTests, ModbusSerial_writeFromFrame_Offset)
{
	USE_FAKE_SYSTEM;
	USE_MOCK_SERIAL;
	modbus->config(&mockSerial, &fakeSystem.get(), 1200, -1);

	modbus->resetFrame(9);
	auto frame = modbus->getFramePtr();
	setArray(frame,
		(byte)2,
		(byte)3,
		(byte)5,
		(byte)7,
		(byte)11,
		(byte)13,
		(byte)17,
		(byte)19,
		(byte)23);

	int length = modbus->writeFromFrame(-1, 3);

	ASSERT_EQ(length, 6);
	ASSERT_EQ(writeQueue.front(), 7);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 11);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 13);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 17);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 19);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 23);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.size(), 0);
}

TEST_F(ModbusSerialTests, ModbusSerial_writeFromFrame_Offset_Lower_Length)
{
	USE_FAKE_SYSTEM;
	USE_MOCK_SERIAL;
	modbus->config(&mockSerial, &fakeSystem.get(), 1200, -1);

	modbus->resetFrame(9);
	auto frame = modbus->getFramePtr();
	setArray(frame,
		(byte)2,
		(byte)3,
		(byte)5,
		(byte)7,
		(byte)11,
		(byte)13,
		(byte)17,
		(byte)19,
		(byte)23);

	int length = modbus->writeFromFrame(5, 3);

	ASSERT_EQ(length, 5);
	ASSERT_EQ(writeQueue.front(), 7);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 11);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 13);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 17);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.front(), 19);
	writeQueue.pop();
	ASSERT_EQ(writeQueue.size(), 0);
}

TEST_F(ModbusSerialTests, ModbusSerial_WriteTwice_NoBeginTransmission)
{
	USE_FAKE;
	Fake(Method(fakeSerial, begin));
	When(Method(fakeSerial, write)).AlwaysReturn(1);
	Fake(Method(fakeSystem, pinMode));
	Fake(Method(fakeSystem, digitalWrite));
	modbus->config(&fakeSerial.get(), &fakeSystem.get(), 1200, 4);

	modbus->write(5);
	modbus->write(5);

	Verify(Method(fakeSerial, write).Using(5)).Twice();
	Verify(Method(fakeSystem, digitalWrite).Using(4, LOW)).Exactly(3);
	Verify(Method(fakeSystem, digitalWrite).Using(4, HIGH)).Twice();
}

TEST_F(ModbusSerialTests, ModbusSerial_WriteFromFrame_NoBeginTransmission)
{
	USE_FAKE;
	Fake(Method(fakeSerial, begin));
	When(Method(fakeSerial, write)).AlwaysReturn(1);
	Fake(Method(fakeSystem, pinMode));
	Fake(Method(fakeSystem, digitalWrite));
	modbus->config(&fakeSerial.get(), &fakeSystem.get(), 1200, 4);

	modbus->resetFrame(3);
	auto frame = modbus->getFramePtr();
	setArray(frame,
		(byte)3,
		(byte)3,
		(byte)3);
	modbus->writeFromFrame();
	modbus->writeFromFrame();

	Verify(Method(fakeSerial, write).Using(3)).Exactly(6);
	Verify(Method(fakeSystem, digitalWrite).Using(4, LOW)).Exactly(3);
	Verify(Method(fakeSystem, digitalWrite).Using(4, HIGH)).Twice();
}

TEST_F(ModbusSerialTests, ModbusSerial_WriteTwice_BeginTransmission)
{
	USE_FAKE;
	Fake(Method(fakeSerial, begin));
	When(Method(fakeSerial, write)).AlwaysReturn(1);
	Fake(Method(fakeSystem, pinMode));
	Fake(Method(fakeSystem, digitalWrite));
	modbus->config(&fakeSerial.get(), &fakeSystem.get(), 1200, 4);

	bool s1 = modbus->beginTransmission();
	modbus->write(5);
	modbus->write(5);
	bool s2 = modbus->endTransmission();

	ASSERT_TRUE(s1);
	ASSERT_TRUE(s2);
	Verify(Method(fakeSerial, write).Using(5)).Twice();
	Verify(Method(fakeSystem, digitalWrite).Using(4, LOW)).Twice();
	Verify(Method(fakeSystem, digitalWrite).Using(4, HIGH)).Once();
}

TEST_F(ModbusSerialTests, ModbusSerial_WriteFromFrame_BeginTransmission)
{
	USE_FAKE;
	Fake(Method(fakeSerial, begin));
	When(Method(fakeSerial, write)).AlwaysReturn(1);
	Fake(Method(fakeSystem, pinMode));
	Fake(Method(fakeSystem, digitalWrite));
	modbus->config(&fakeSerial.get(), &fakeSystem.get(), 1200, 4);

	modbus->resetFrame(3);
	auto frame = modbus->getFramePtr();
	setArray(frame,
		(byte)3,
		(byte)3,
		(byte)3);
	bool s1 = modbus->beginTransmission();
	modbus->writeFromFrame();
	modbus->writeFromFrame();
	bool s2 = modbus->endTransmission();

	ASSERT_TRUE(s1);
	ASSERT_TRUE(s2);
	Verify(Method(fakeSerial, write).Using(3)).Exactly(6);
	Verify(Method(fakeSystem, digitalWrite).Using(4, LOW)).Twice();
	Verify(Method(fakeSystem, digitalWrite).Using(4, HIGH)).Once();
}

TEST_F(ModbusSerialTests, ModbusSerial_Begin_End_Transmission)
{
	USE_FAKE;
	Fake(Method(fakeSerial, begin));
	Fake(Method(fakeSystem, pinMode));
	Fake(Method(fakeSystem, digitalWrite));
	modbus->config(&fakeSerial.get(), &fakeSystem.get(), 1200, 4);

	bool t1 = modbus->_transmitting;
	bool s1 = modbus->endTransmission();
	bool t2 = modbus->_transmitting;
	bool s2 = modbus->beginTransmission();
	bool t3 = modbus->_transmitting;
	bool s3 = modbus->beginTransmission();
	bool t4 = modbus->_transmitting;
	bool s4 = modbus->endTransmission();
	bool t5 = modbus->_transmitting;
	bool s5 = modbus->endTransmission();
	bool t6 = modbus->_transmitting;
	bool s6 = modbus->beginTransmission();
	bool t7 = modbus->_transmitting;
	bool s7 = modbus->endTransmission();
	bool t8 = modbus->_transmitting;

	ASSERT_FALSE(s1);
	ASSERT_TRUE(s2);
	ASSERT_FALSE(s3);
	ASSERT_TRUE(s4);
	ASSERT_FALSE(s5);
	ASSERT_TRUE(s6);
	ASSERT_TRUE(s7);

	ASSERT_FALSE(t1);
	ASSERT_FALSE(t2);
	ASSERT_TRUE(t3);
	ASSERT_TRUE(t4);
	ASSERT_FALSE(t5);
	ASSERT_FALSE(t6);
	ASSERT_TRUE(t7);
	ASSERT_FALSE(t8);

	Verify(Method(fakeSystem, digitalWrite).Using(4, LOW)).Exactly(3);
	Verify(Method(fakeSystem, digitalWrite).Using(4, HIGH)).Twice();
}

// pending tests:
// * Add ModbusSerial_writeFromFrame_All once supported by MockSerialStream
// * calcCrc() once I have sample CRC data