#include "bitFunctions.h"

inline char BitFunctions::bitsToBytes(unsigned char bits)
{
	unsigned char result = bits / 8;
	if (bits % 8 == 0)
		return result;
	else
		return result + 1;
}
