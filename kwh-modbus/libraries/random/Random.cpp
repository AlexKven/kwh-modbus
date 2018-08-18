#include "Random.h"
#pragma once

void Random::validateSeed()
{
	if (_z1 <= 1)
		_z1++;
	if (_z2 <= 7)
		_z2 += 7;
	if (_z3 <= 15)
		_z3 += 15;
	if (_z4 <= 127)
		_z4 += 127;
}

Random::Random(uint32_t seed1, uint32_t seed2, uint32_t seed3, uint32_t seed4)
{
	_z1 = seed1;
	_z2 = seed2;
	_z3 = seed3;
	_z4 = seed4;
	validateSeed();
}

Random::Random(int seedLength, uint8_t *seed)
{
	for (int i = 0; i < seedLength; i++)
	{
		_z1 = _z2 = _z3 = _z4 = 0;
		if (i < 4)
		{
			uint32_t shift = seed[i] << (8 * i);
			_z1 += shift;
			_z2 += shift;
			_z3 += shift;
			_z4 += shift;
		}
		else if (i < 16)
		{
			if (i == 4)
				_z2 = 0;
			else if (i == 8)
				_z3 = 0;
			else if (i == 12)
				_z4 = 0;
			
			uint32_t shift = seed[i] << (i % 4) * 8;
			uint8_t which = i / 4;
			switch (which)
			{
			case 1:
				_z2 += shift;
				break;
			case 2:
				_z3 += shift;
				break;
			case 3:
				_z4 += shift;
				break;
			}
		}
		validateSeed();
	}
}

double Random::randomDouble()
{
	uint32_t b;
	b = ((_z1 << 6) ^ _z1) >> 13;
	_z1 = ((_z1 & 4294967294U) << 18) ^ b;
	b = ((_z2 << 2) ^ _z2) >> 27;
	_z2 = ((_z2 & 4294967288U) << 2) ^ b;
	b = ((_z3 << 13) ^ _z3) >> 21;
	_z3 = ((_z3 & 4294967280U) << 7) ^ b;
	b = ((_z4 << 3) ^ _z4) >> 12;
	_z4 = ((_z4 & 4294967168U) << 13) ^ b;
	return (_z1 ^ _z2 ^ _z3 ^ _z4) * 2.3283064365386963e-10;
}

uint32_t Random::randomUInt32()
{
	uint32_t b;
	b = ((_z1 << 6) ^ _z1) >> 13;
	_z1 = ((_z1 & 4294967294U) << 18) ^ b;
	b = ((_z2 << 2) ^ _z2) >> 27;
	_z2 = ((_z2 & 4294967288U) << 2) ^ b;
	b = ((_z3 << 13) ^ _z3) >> 21;
	_z3 = ((_z3 & 4294967280U) << 7) ^ b;
	b = ((_z4 << 3) ^ _z4) >> 12;
	_z4 = ((_z4 & 4294967168U) << 13) ^ b;
	return (_z1 ^ _z2 ^ _z3 ^ _z4);
}
