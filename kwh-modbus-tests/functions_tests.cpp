#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/arduinoFunctions.cpp"

using namespace fakeit;

TEST(bitReadTest, success) {
	// Arrange
	unsigned char input = 75;
	unsigned char bits[8];
	bits[0] = 1;
	bits[1] = 1;
	bits[2] = 0;
	bits[3] = 1;
	bits[4] = 0;
	bits[5] = 0;
	bits[6] = 1;
	bits[7] = 0;

	// Act, Assert
	for (unsigned char i = 0; i < 8; i++)
	{
		ASSERT_EQ(ArduinoFunctions::bitRead(input, i), bits[i]);
	}
}

TEST(bitSetClearTest, success) {
	// Arrange
	unsigned char input = (unsigned char)(rand() % 256);
	unsigned char expected = 75;
	unsigned char bits[8];
	bits[0] = 1;
	bits[1] = 1;
	bits[2] = 0;
	bits[3] = 1;
	bits[4] = 0;
	bits[5] = 0;
	bits[6] = 1;
	bits[7] = 0;

	// Act
	for (unsigned char i = 0; i < 8; i++)
	{
		if (bits[i] == 1)
			ArduinoFunctions::bitSet(input, i);
		else
			ArduinoFunctions::bitClear(input, i);
	}

	// Assert
	ASSERT_EQ(input, expected);
}
