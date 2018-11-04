#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/bitFunctions/bitFunctions.h"
#include "test_helpers.h"
#include "../kwh-modbus/noArduino/ArduinoMacros.h"

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

TEST(BitFunctionTests, setBits_Byte_28)
{
	byte bte = 0;
	BitFunctions::setBits(&bte, 2, 3);

	ASSERT_EQ(bte, 28);
}

TEST(BitFunctionTests, setBits_SignedInt_28)
{
	int num = 0;
	BitFunctions::setBits(&num, 7, 8);

	ASSERT_EQ(num, 32640);
}

TEST(BitFunctionTests, setBits_SignedInt_2sComp)
{
	int num = 0;
	BitFunctions::setBits(&num, 1, 31);

	ASSERT_EQ(num, -2);
}

TEST(BitFunctionTests, setBits_TwoBytes_192_7)
{
	byte *bte = new byte[2];
	bte[0] = 0;
	bte[1] = 0;
	BitFunctions::setBits(bte, 6, 5);

	ASSERT_EQ(bte[0], 192);
	ASSERT_EQ(bte[1], 7);
	delete[] bte;
}