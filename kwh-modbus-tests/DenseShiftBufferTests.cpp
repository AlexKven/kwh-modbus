#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/denseShiftBuffer/DenseShiftBuffer.hpp"
#include "test_helpers.h"
#include "../kwh-modbus/noArduino/ArduinoMacros.h"

using namespace fakeit;

TEST_TRAITS(DenseShiftBufferTests, bytesAs8Bits_Create,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	DenseShiftBuffer<byte, 8> buffer(10);

	ASSERT_EQ(buffer.getCapacity(), 10);
	ASSERT_EQ(buffer._current, 9);
	ASSERT_EQ(buffer.getNumStored(), 0);
}

TEST_TRAITS(DenseShiftBufferTests, bytesAs8Bits_PartlyFill,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	DenseShiftBuffer<byte, 8> buffer(10);

	buffer.push(2);
	buffer.push(3);
	buffer.push(5);
	buffer.push(7);
	buffer.push(11);
	buffer.push(13);

	ASSERT_EQ(buffer._current, 5);
	assertArrayEq<byte, byte, byte, byte, byte, byte>(
		buffer._buffer, 2, 3, 5, 7, 11, 13);
	ASSERT_EQ(buffer.get(0), 13);
	ASSERT_EQ(buffer.get(1), 11);
	ASSERT_EQ(buffer.get(2), 7);
	ASSERT_EQ(buffer.get(5), 2);
	ASSERT_EQ(buffer.getNumStored(), 6);
}

TEST_TRAITS(DenseShiftBufferTests, bytesAs8Bits_ExactlyFill,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	DenseShiftBuffer<byte, 8> buffer(10);

	buffer.push(2);
	buffer.push(3);
	buffer.push(5);
	buffer.push(7);
	buffer.push(11);
	buffer.push(13);
	buffer.push(17);
	buffer.push(19);
	buffer.push(23);
	buffer.push(29);

	ASSERT_EQ(buffer._current, 9);
	assertArrayEq<byte, byte, byte, byte, byte, byte, byte, byte, byte, byte>(
		buffer._buffer, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29);
	ASSERT_EQ(buffer.get(0), 29);
	ASSERT_EQ(buffer.get(1), 23);
	ASSERT_EQ(buffer.get(2), 19);
	ASSERT_EQ(buffer.get(8), 3);
	ASSERT_EQ(buffer.get(9), 2);
	ASSERT_EQ(buffer.getNumStored(), 10);
}

TEST_TRAITS(DenseShiftBufferTests, bytesAs8Bits_OverFill,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	DenseShiftBuffer<byte, 8> buffer(10);

	buffer.push(2);
	buffer.push(3);
	buffer.push(5);
	buffer.push(7);
	buffer.push(11);
	buffer.push(13);
	buffer.push(17);
	buffer.push(19);
	buffer.push(23);
	buffer.push(29);
	buffer.push(31);
	buffer.push(37);
	buffer.push(41);
	buffer.push(43);

	ASSERT_EQ(buffer._current, 3);
	assertArrayEq<byte, byte, byte, byte, byte, byte, byte, byte, byte, byte>(
		buffer._buffer, 31, 37, 41, 43, 11, 13, 17, 19, 23, 29);
	ASSERT_EQ(buffer.get(0), 43);
	ASSERT_EQ(buffer.get(1), 41);
	ASSERT_EQ(buffer.get(2), 37);
	ASSERT_EQ(buffer.get(8), 13);
	ASSERT_EQ(buffer.get(9), 11);
	ASSERT_EQ(buffer.getNumStored(), 10);
}

TEST_TRAITS(DenseShiftBufferTests, shortAs12Bits_Create,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	DenseShiftBuffer<uint16_t, 12> buffer(5);

	ASSERT_EQ(buffer.getCapacity(), 5);
	ASSERT_EQ(buffer._current, 4);
	ASSERT_EQ(buffer.getNumStored(), 0);
}

TEST_TRAITS(DenseShiftBufferTests, shortAs12Bits_PartlyFill,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	DenseShiftBuffer<uint16_t, 12> buffer(5);
	buffer._buffer[3] = 0; // Allows us to verify last index

	buffer.push(1);
	buffer.push(10);
	buffer.push(100);

	ASSERT_EQ(buffer._current, 2);
	assertArrayEq<byte, byte, byte, byte>(
		buffer._buffer, 0x01, 0xA0, 0x00, 0x64);
	ASSERT_EQ(buffer.get(0), 100);
	ASSERT_EQ(buffer.get(1), 10);
	ASSERT_EQ(buffer.get(2), 1);
	ASSERT_EQ(buffer.getNumStored(), 3);
}

TEST_TRAITS(DenseShiftBufferTests, shortAs12Bits_ExactlyFill,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	DenseShiftBuffer<uint16_t, 12> buffer(5);
	buffer._buffer[7] = 0; // Allows us to verify last index

	buffer.push(1);
	buffer.push(10);
	buffer.push(100);
	buffer.push(1000);
	buffer.push(10000);

	ASSERT_EQ(buffer._current, 4);
	assertArrayEq<byte, byte, byte, byte, byte, byte, byte, byte>(
		buffer._buffer, 0x01, 0xA0, 0x00, 0x64, 0x80, 0x3E, 0x10, 0x07);
	ASSERT_EQ(buffer.get(0), 1808); // 10000 doesn't fit in 12 bits
	ASSERT_EQ(buffer.get(1), 1000);
	ASSERT_EQ(buffer.get(2), 100);
	ASSERT_EQ(buffer.get(3), 10);
	ASSERT_EQ(buffer.get(4), 1);
	ASSERT_EQ(buffer.getNumStored(), 5);
}

TEST_TRAITS(DenseShiftBufferTests, shortAs12Bits_OverFill,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	DenseShiftBuffer<uint16_t, 12> buffer(5);
	buffer._buffer[7] = 0; // Allows us to verify last index

	buffer.push(1);
	buffer.push(10);
	buffer.push(100);
	buffer.push(1000);
	buffer.push(10000);
	buffer.push(3855);
	buffer.push(1092);

	ASSERT_EQ(buffer._current, 1);
	assertArrayEq<byte, byte, byte, byte, byte, byte, byte, byte>(
		buffer._buffer, 0x0F, 0x4F, 0x44, 0x64, 0x80, 0x3E, 0x10, 0x07);
	ASSERT_EQ(buffer.get(0), 1092); // 10000 doesn't fit in 12 bits
	ASSERT_EQ(buffer.get(1), 3855);
	ASSERT_EQ(buffer.get(2), 1808);
	ASSERT_EQ(buffer.get(3), 1000);
	ASSERT_EQ(buffer.get(4), 100);
	ASSERT_EQ(buffer.getNumStored(), 5);
}