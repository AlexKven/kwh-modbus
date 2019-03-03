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
}