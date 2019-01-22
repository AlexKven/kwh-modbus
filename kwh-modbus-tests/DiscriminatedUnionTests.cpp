#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/discriminatedUnion/DiscriminatedUnion.hpp"
#include "test_helpers.h"
#include "../kwh-modbus/noArduino/ArduinoMacros.h"

using namespace fakeit;

TEST_TRAITS(DiscriminatedUnionTests, sizeof_byte,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto size = sizeof(DiscriminatedUnion<byte>);
	ASSERT_EQ(size, sizeof(byte));
}

TEST_TRAITS(DiscriminatedUnionTests, sizeof_byte_short,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto size = sizeof(DiscriminatedUnion<byte, short>);
	ASSERT_EQ(size, sizeof(short));
}

TEST_TRAITS(DiscriminatedUnionTests, sizeof_short_byte,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto size = sizeof(DiscriminatedUnion<short, byte>);
	ASSERT_EQ(size, sizeof(short));
}

TEST_TRAITS(DiscriminatedUnionTests, sizeof_byte_int_short,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto size = sizeof(DiscriminatedUnion<byte, int, short>);
	ASSERT_EQ(size, sizeof(int));
}

TEST_TRAITS(DiscriminatedUnionTests, sizeof_byte_int_short_long,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto size = sizeof(DiscriminatedUnion<byte, int, short, long>);
	ASSERT_EQ(size, sizeof(long));
}

TEST_TRAITS(DiscriminatedUnionTests, union_asFirstType,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	DiscriminatedUnion<byte, int, double> disc;
	disc.As<byte>() = 240;

	ASSERT_EQ(disc.As<byte>(), 240);
}

TEST_TRAITS(DiscriminatedUnionTests, union_asSecondType,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	DiscriminatedUnion<byte, int, double> disc;
	disc.As<int>() = 64012;

	ASSERT_EQ(disc.As<int>(), 64012);
}

TEST_TRAITS(DiscriminatedUnionTests, union_asThirdType,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	DiscriminatedUnion<byte, int, double> disc;
	disc.As<double>() = 3.14159265358979;

	ASSERT_EQ(disc.As<double>(), 3.14159265358979);
}

TEST_TRAITS(DiscriminatedUnionTests, union_asTypeConverter,
	Type, Stress, Threading, Single, Determinism, Static, Case, Typical)
{
	DiscriminatedUnion<byte, uint16_t> disc;
	disc.As<uint16_t>() = 0x1234;

	ASSERT_EQ(disc.As<byte>(), 0x34);
}