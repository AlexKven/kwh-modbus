#pragma once
#include <stdint.h>

// Implementation posted by R Simard from
// https://stackoverflow.com/questions/1167253/implementation-of-rand

class Random
{
private:
	// default seed values. Providing your own
	// seed highly recommended;
	uint32_t _z1 = 12345,
		_z2 = 12345,
		_z3 = 12345,
		_z4 = 12345;
	void validateSeed();

public:
	Random();

	double randomDouble();
	uint32_t randomUInt32();

	void seed(uint32_t seed1, uint32_t seed2, uint32_t seed3, uint32_t seed4);
	void seed(int seedLength, uint8_t *seed);
};