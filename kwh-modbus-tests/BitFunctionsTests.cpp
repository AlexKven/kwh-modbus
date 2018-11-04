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

TEST(BitFunctionTests, setBits_SignedInt_32640)
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

TEST(BitFunctionTests, setBits_ThreeShorts)
{
	word *num = new word[3];
	num[0] = 0;
	num[1] = 0;
	num[2] = 0;
	BitFunctions::setBits(num, 14, 20);

	ASSERT_EQ(num[0], 49152);
	ASSERT_EQ(num[1], 65535);
	ASSERT_EQ(num[2], 3);
	delete[] num;
}

TEST(BitFunctionTests, clearBits_Byte_28)
{
	byte bte = 29;
	BitFunctions::clearBits(&bte, 2, 3);

	ASSERT_EQ(bte, 1);
}

TEST(BitFunctionTests, clearBits_SignedInt_32640)
{
	int num = 32640 + 1 + 32768;
	BitFunctions::clearBits(&num, 7, 8);

	ASSERT_EQ(num, 32769);
}

TEST(BitFunctionTests, clearBits_SignedInt_2sComp)
{
	int num = -2;
	BitFunctions::clearBits(&num, 1, 31);

	ASSERT_EQ(num, 0);
}

TEST(BitFunctionTests, clearBits_TwoBytes_192_7)
{
	byte *bte = new byte[2];
	bte[0] = 193;
	bte[1] = 4;
	BitFunctions::clearBits(bte, 6, 5);

	ASSERT_EQ(bte[0], 1);
	ASSERT_EQ(bte[1], 0);
	delete[] bte;
}

TEST(BitFunctionTests, clearBits_ThreeShorts)
{
	word *num = new word[3];
	num[0] = 49152;
	num[1] = 10416;
	num[2] = 7;
	BitFunctions::clearBits(num, 14, 20);

	ASSERT_EQ(num[0], 0);
	ASSERT_EQ(num[1], 0);
	ASSERT_EQ(num[2], 4);
	delete[] num;
}