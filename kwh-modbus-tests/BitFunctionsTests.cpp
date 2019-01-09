#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/bitFunctions/BitFunctions.hpp"
#include "test_helpers.h"
#include "../kwh-modbus/noArduino/ArduinoMacros.h"

using namespace fakeit;

TEST_TRAITS(BitFunctionTests, bitsToBytes_1,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto bytes = BitFunctions::bitsToBytes(1);
	ASSERT_EQ(bytes, 1);
}

TEST_TRAITS(BitFunctionTests, bitsToBytes_8,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto bytes = BitFunctions::bitsToBytes(8);
	ASSERT_EQ(bytes, 1);
}

TEST_TRAITS(BitFunctionTests, bitsToBytes_9,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto bytes = BitFunctions::bitsToBytes(9);
	ASSERT_EQ(bytes, 2);
}

TEST_TRAITS(BitFunctionTests, bitsToBytes_12,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto bytes = BitFunctions::bitsToBytes(12);
	ASSERT_EQ(bytes, 2);
}

TEST_TRAITS(BitFunctionTests, bitsToBytes_16,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto bytes = BitFunctions::bitsToBytes(16);
	ASSERT_EQ(bytes, 2);
}

TEST_TRAITS(BitFunctionTests, bitsToBytes_17,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto bytes = BitFunctions::bitsToBytes(17);
	ASSERT_EQ(bytes, 3);
}

TEST_TRAITS(BitFunctionTests, bitsToBytes_63,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto bytes = BitFunctions::bitsToBytes(63);
	ASSERT_EQ(bytes, 8);
}

TEST_TRAITS(BitFunctionTests, bitsToBytes_65,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto bytes = BitFunctions::bitsToBytes(65);
	ASSERT_EQ(bytes, 9);
}

TEST_TRAITS(BitFunctionTests, bitsToStructs_uint16_t_1,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = BitFunctions::bitsToStructs<uint16_t, byte>(1);
	ASSERT_EQ(result, 1);
}

TEST_TRAITS(BitFunctionTests, bitsToStructs_uint16_t_16,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = BitFunctions::bitsToStructs<uint16_t, byte>(16);
	ASSERT_EQ(result, 1);
}

TEST_TRAITS(BitFunctionTests, bitsToStructs_int_33,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto result = BitFunctions::bitsToStructs<uint16_t, byte>(33);
	ASSERT_EQ(result, 3);
}

TEST_TRAITS(BitFunctionTests, bitsToStructs_uint64_t_96,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = BitFunctions::bitsToStructs<uint64_t, byte>(96);
	ASSERT_EQ(result, 2);
}

TEST_TRAITS(BitFunctionTests, bitsToStructs_int_64,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = BitFunctions::bitsToStructs<int, byte>(64);
	ASSERT_EQ(result, 2);
}

TEST_TRAITS(BitFunctionTests, bitsToStructs_int_65,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto result = BitFunctions::bitsToStructs<int, byte>(65);
	ASSERT_EQ(result, 3);
}

TEST_TRAITS(BitFunctionTests, bitsToStructs_int_95,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = BitFunctions::bitsToStructs<int, byte>(95);
	ASSERT_EQ(result, 3);
}

TEST_TRAITS(BitFunctionTests, bitsToStructs_int_96,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = BitFunctions::bitsToStructs<int, byte>(96);
	ASSERT_EQ(result, 3);
}

TEST_TRAITS(BitFunctionTests, bitsToStructs_int_97,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto result = BitFunctions::bitsToStructs<int, byte>(97);
	ASSERT_EQ(result, 4);
}

TEST_TRAITS(BitFunctionTests, bitsToStructs_string_0,
	Type, Unit, Threading, Single, Determinism, Static, Case, Rare)
{
	auto result = BitFunctions::bitsToStructs<std::string, byte>(0);
	ASSERT_EQ(result, 0);
}

TEST_TRAITS(BitFunctionTests, setBits_Byte_28,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	byte bte = 0;
	BitFunctions::setBits(&bte, 2, 3);

	ASSERT_EQ(bte, 28);
}

TEST_TRAITS(BitFunctionTests, setBits_SignedInt_32640,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	int num = 0;
	BitFunctions::setBits(&num, 7, 8);

	ASSERT_EQ(num, 32640);
}

TEST_TRAITS(BitFunctionTests, setBits_SignedInt_2sComp,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	int num = 0;
	BitFunctions::setBits(&num, 1, 31);

	ASSERT_EQ(num, -2);
}

TEST_TRAITS(BitFunctionTests, setBits_TwoBytes_192_7,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	byte *bte = new byte[2];
	bte[0] = 0;
	bte[1] = 0;
	BitFunctions::setBits(bte, 6, 5);

	ASSERT_EQ(bte[0], 192);
	ASSERT_EQ(bte[1], 7);
	delete[] bte;
}

TEST_TRAITS(BitFunctionTests, setBits_Threeuint16_ts,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_TRAITS(BitFunctionTests, clearBits_Byte_28,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	byte bte = 29;
	BitFunctions::clearBits(&bte, 2, 3);

	ASSERT_EQ(bte, 1);
}

TEST_TRAITS(BitFunctionTests, clearBits_SignedInt_32640,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	int num = 32640 + 1 + 32768;
	BitFunctions::clearBits(&num, 7, 8);

	ASSERT_EQ(num, 32769);
}

TEST_TRAITS(BitFunctionTests, clearBits_SignedInt_2sComp,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	int num = -2;
	BitFunctions::clearBits(&num, 1, 31);

	ASSERT_EQ(num, 0);
}

TEST_TRAITS(BitFunctionTests, clearBits_TwoBytes_192_7,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	byte *bte = new byte[2];
	bte[0] = 193;
	bte[1] = 4;
	BitFunctions::clearBits(bte, 6, 5);

	ASSERT_EQ(bte[0], 1);
	ASSERT_EQ(bte[1], 0);
	delete[] bte;
}

TEST_TRAITS(BitFunctionTests, clearBits_Threeuint16_ts,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
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

TEST_TRAITS(BitFunctionTests, copyBits_Bytes_36,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	byte dest = 89;
	byte src = 166;
	BitFunctions::copyBits(&src, &dest, 2, 2, 4);

	ASSERT_EQ(dest, 101);
}

TEST_TRAITS(BitFunctionTests, copyBits_uint16_tToBytes_Middle14,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	byte *dest = new byte[2];
	dest[0] = 1;
	dest[1] = 128;
	uint16_t src = 5000;
	BitFunctions::copyBits(&src, dest, 1, 1, 14);

	ASSERT_EQ(dest[0], 137);
	ASSERT_EQ(dest[1], 147);
}

TEST_TRAITS(BitFunctionTests, copyBits_uint16_tToBytes_EndToEnd,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	byte *dest = new byte[2];
	dest[0] = 1;
	dest[1] = 128;
	uint16_t src = 5000;
	BitFunctions::copyBits(&src, dest, 0, 0, 16);

	ASSERT_EQ(dest[0], 136);
	ASSERT_EQ(dest[1], 19);
}

TEST_TRAITS(BitFunctionTests, copyBits_IntToByte_2sCompliment,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	byte *dest = new byte[4];
	int src = -4;
	BitFunctions::copyBits(&src, dest, 0, 0, 32);

	ASSERT_EQ(dest[0], 252);
	ASSERT_EQ(dest[1], 255);
	ASSERT_EQ(dest[2], 255);
	ASSERT_EQ(dest[3], 255);
}

TEST_TRAITS(BitFunctionTests, copyBits_byteTouint16_t_LeftShift,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	uint16_t dest = 1;
	byte src = 20;
	BitFunctions::copyBits(&src, &dest, 0, 7, 8);

	ASSERT_EQ(dest, 2561);
}

TEST_TRAITS(BitFunctionTests, copyBits_byteTouint16_t_RightShift,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	byte dest = 1;
	uint16_t src = 2592;
	BitFunctions::copyBits(&src, &dest, 7, 0, 8);

	ASSERT_EQ(dest, 20);
}