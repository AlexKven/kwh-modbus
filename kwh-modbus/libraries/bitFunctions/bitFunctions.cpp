#include "bitFunctions.h"

inline char BitFunctions::bitsToBytes(unsigned char bits)
{
	unsigned char result = bits / CHAR_BIT;
	if (bits % CHAR_BIT == 0)
		return result;
	else
		return result + 1;
}
