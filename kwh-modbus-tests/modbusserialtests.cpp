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

#define USE_FAKE_SYSTEM Mock<ISystemFunctions> fakeSystem;
#define USE_FAKE_SERIAL Mock<ISerialStream> fakeSerial;

#define USE_MOCK_SERIAL std::queue<uint8_t> readQueue; \
std::queue<uint8_t> writeQueue; \
MockSerialStream mockSerial = MockSerialStream(&readQueue, &writeQueue);

#define CONFIG_MODBUS_FAKE_ALL Fake(Method(fakeSerial, begin)); \
modbus->config(&fakeSerial.get(), &fakeSystem.get(), 1200, -1);

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
readQueue.push(144);

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
	USE_FAKE_SYSTEM
		USE_MOCK_SERIAL
		READ_QUEUE_PUSH_FIBONACCI
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
	USE_FAKE_SYSTEM
	USE_MOCK_SERIAL
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
	USE_FAKE_SYSTEM
	USE_MOCK_SERIAL
	READ_QUEUE_PUSH_FIBONACCI
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
	USE_FAKE_SYSTEM
	USE_MOCK_SERIAL
	READ_QUEUE_PUSH_FIBONACCI
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
	USE_FAKE_SYSTEM
	USE_MOCK_SERIAL
	READ_QUEUE_PUSH_FIBONACCI
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