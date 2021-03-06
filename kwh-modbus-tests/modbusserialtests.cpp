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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_Config,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;

	Verify(Method(fakeSerial, begin).Using(1200)).Once();
	ASSERT_EQ(modbus->_t15, 13750);
	ASSERT_EQ(modbus->_t35, 13750 * 3.5);
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_Config_HighBaud,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	USE_FAKE;
	Fake(Method(fakeSerial, begin));

	modbus->config(&fakeSerial.get(), &fakeSystem.get(), 20000, -1);

	Verify(Method(fakeSerial, begin).Using(20000)).Once();
	ASSERT_EQ(modbus->_t15, 750);
	ASSERT_EQ(modbus->_t35, 750 * 3.5);
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_Config_TXPin,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_Available,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, available)).Return(32);

	int result = modbus->available();

	ASSERT_EQ(result, 32);
	Verify(Method(fakeSerial, available)).Once();
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_Flush,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	Fake(Method(fakeSerial, flush));

	modbus->flush();

	Verify(Method(fakeSerial, flush)).Once();
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_Read,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, read)).Return(5);

	auto result = modbus->read();

	Verify(Method(fakeSerial, read)).Once();
	ASSERT_EQ(result, 5);
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_Read_Success,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_Read_Failure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_ReadWord_Success,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, read)).Return(2).Return(191);

	word result = 3;
	bool success = modbus->readWord(result);

	Verify(Method(fakeSerial, read)).Twice();
	ASSERT_EQ(result, 703);
	ASSERT_TRUE(success);
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_ReadWord_Failure_First,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, read)).Return(-1);

	word result = 3;
	bool success = modbus->readWord(result);

	Verify(Method(fakeSerial, read)).Once();
	ASSERT_EQ(result, 3);
	ASSERT_FALSE(success);
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_ReadWord_Failure_Second,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, read)).Return(2).Return(-1);

	word result = 3;
	bool success = modbus->readWord(result);

	Verify(Method(fakeSerial, read)).Twice();
	ASSERT_EQ(result, 3);
	ASSERT_FALSE(success);
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_Write_Success,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, write)).Return(1);

	byte write = 5;
	bool success = modbus->write(write);

	Verify(Method(fakeSerial, write).Using(5)).Once();
	ASSERT_TRUE(success);
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_WriteWord_Success,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, write)).Return(1).Return(1);

	word write = 703;
	bool success = modbus->writeWord(write);

	Verify(Method(fakeSerial, write).Using(191)).Once();
	Verify(Method(fakeSerial, write).Using(2)).Once();
	ASSERT_TRUE(success);
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_WriteWord_Failure_First,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, write)).Return(0).Return(1);

	word write = 703;
	bool success = modbus->writeWord(write);

	Verify(Method(fakeSerial, write).Using(191)).Never();
	Verify(Method(fakeSerial, write).Using(2)).Once();
	ASSERT_FALSE(success);
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_WriteWord_Failure_Second,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, write)).Return(1).Return(0);

	word write = 703;
	bool success = modbus->writeWord(write);

	Verify(Method(fakeSerial, write).Using(191)).Once();
	Verify(Method(fakeSerial, write).Using(2)).Once();
	ASSERT_FALSE(success);
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_Write_Failure,
	Type, Unit, Threading, Single, Determinism, Static, Case, Failure)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, write)).Return(0);

	byte write = 5;
	bool success = modbus->write(write);

	Verify(Method(fakeSerial, write).Using(5)).Once();
	ASSERT_FALSE(success);
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_FrameDelay,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	Fake(Method(fakeSystem, delayMicroseconds));
	unsigned int delay = modbus->_t35;

	modbus->frameDelay();

	Verify(Method(fakeSystem, delayMicroseconds).Using(delay)).Once();
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_ByteDelay,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	Fake(Method(fakeSystem, delayMicroseconds));
	unsigned int delay = modbus->_t15;

	modbus->byteDelay();

	Verify(Method(fakeSystem, delayMicroseconds).Using(delay)).Once();
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_AwaitIncomingSerial_Nothing,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_FAKE;
	CONFIG_MODBUS_FAKE_ALL;
	When(Method(fakeSerial, available)).Return(0);

	int result = modbus->awaitIncomingSerial();

	ASSERT_EQ(result, 0);
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_AwaitIncomingSerial_Something,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_readToFrame_All,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_readToFrame_Incomplete,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_readToFrame_Lower_Length,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_readToFrame_Offset,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_readToFrame_Offset_Lower_Length,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_writeFromFrame_All,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_writeFromFrame_Lower_Length,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_writeFromFrame_Offset,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_writeFromFrame_Offset_Lower_Length,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_WriteTwice_NoBeginTransmission,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	USE_FAKE;
	Fake(Method(fakeSerial, begin));
	Fake(Method(fakeSerial, flush));
	When(Method(fakeSerial, write)).AlwaysReturn(1);
	Fake(Method(fakeSystem, pinMode));
	Fake(Method(fakeSystem, digitalWrite));
	Fake(Method(fakeSystem, delayMicroseconds));
	modbus->config(&fakeSerial.get(), &fakeSystem.get(), 1200, 4);

	modbus->write(5);
	modbus->write(5);

	Verify(Method(fakeSerial, write).Using(5)).Twice();
	Verify(Method(fakeSystem, digitalWrite).Using(4, LOW)).Exactly(3);
	Verify(Method(fakeSystem, digitalWrite).Using(4, HIGH)).Twice();
}

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_WriteFromFrame_NoBeginTransmission,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	USE_FAKE;
	Fake(Method(fakeSerial, begin));
	Fake(Method(fakeSerial, flush));
	When(Method(fakeSerial, write)).AlwaysReturn(1);
	Fake(Method(fakeSystem, pinMode));
	Fake(Method(fakeSystem, digitalWrite));
	Fake(Method(fakeSystem, delayMicroseconds));
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_WriteTwice_BeginTransmission,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_FAKE;
	Fake(Method(fakeSerial, begin));
	Fake(Method(fakeSerial, flush));
	When(Method(fakeSerial, write)).AlwaysReturn(1);
	Fake(Method(fakeSystem, pinMode));
	Fake(Method(fakeSystem, digitalWrite));
	Fake(Method(fakeSystem, delayMicroseconds));
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_WriteFromFrame_BeginTransmission,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_FAKE;
	Fake(Method(fakeSerial, begin));
	Fake(Method(fakeSerial, flush));
	When(Method(fakeSerial, write)).AlwaysReturn(1);
	Fake(Method(fakeSystem, pinMode));
	Fake(Method(fakeSystem, digitalWrite));
	Fake(Method(fakeSystem, delayMicroseconds));
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

TEST_F_TRAITS(ModbusSerialTests, ModbusSerial_Begin_End_Transmission,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	USE_FAKE;
	Fake(Method(fakeSerial, begin));
	Fake(Method(fakeSerial, flush));
	Fake(Method(fakeSystem, pinMode));
	Fake(Method(fakeSystem, digitalWrite));
	Fake(Method(fakeSystem, delayMicroseconds));
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
// * write(word val)