#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/bitFunctions/bitFunctions.h"
#include "test_helpers.h"

using namespace fakeit;

TEST(BitFunctionTests, bitsToBytes_1)
{
	auto bytes = BitFunctions::bitsToBytes(1);
	ASSERT_EQ(bytes, 1);
}

TEST(BitFunctionTests, bitsToBytes_8)
{
	auto bytes = BitFunctions::bitsToBytes(8);
	ASSERT_EQ(bytes, 1);
}

TEST(BitFunctionTests, bitsToBytes_9)
{
	auto bytes = BitFunctions::bitsToBytes(9);
	ASSERT_EQ(bytes, 2);
}

TEST(BitFunctionTests, bitsToBytes_12)
{
	auto bytes = BitFunctions::bitsToBytes(12);
	ASSERT_EQ(bytes, 2);
}

TEST(BitFunctionTests, bitsToBytes_16)
{
	auto bytes = BitFunctions::bitsToBytes(16);
	ASSERT_EQ(bytes, 2);
}

TEST(BitFunctionTests, bitsToBytes_17)
{
	auto bytes = BitFunctions::bitsToBytes(17);
	ASSERT_EQ(bytes, 3);
}

TEST(BitFunctionTests, bitsToBytes_63)
{
	auto bytes = BitFunctions::bitsToBytes(63);
	ASSERT_EQ(bytes, 8);
}

TEST(BitFunctionTests, bitsToBytes_65)
{
	auto bytes = BitFunctions::bitsToBytes(65);
	ASSERT_EQ(bytes, 9);
}