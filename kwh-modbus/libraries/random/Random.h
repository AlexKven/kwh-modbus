#pragma once
#include <stdint.h>

// Implementation posted by R Simard from
// https://stackoverflow.com/questions/1167253/implementation-of-rand

class Random
{
private:
	uint32_t _z1, _z2, _z3, _z4;
	void validateSeed();

public:
	Random(uint32_t seed1, uint32_t seed2, uint32_t seed3, uint32_t seed4);
	Random(int seedLength, uint8_t *seed);

	double randomDouble();
	uint32_t randomUInt32();
};