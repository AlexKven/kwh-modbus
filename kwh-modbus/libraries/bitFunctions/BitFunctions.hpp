#pragma once

#ifdef NO_ARDUINO
#else
#include "../arduinoMacros/arduinoMacros.h"
#endif
#include <climits>

class BitFunctions
{
public:
	static inline char bitsToBytes(unsigned char bits)
	{
		unsigned char result = bits / CHAR_BIT;
		if (bits % CHAR_BIT == 0)
			return result;
		else
			return result + 1;
	}

	template<class T, class N>
	static inline void setBits(T *ptr, N bit, N count = 1)
	{
		for (N i = 0; i < count; i++)
		{
			N byteOffset = (bit + i) / (CHAR_BIT * sizeof(T));
			N bitOffset = (bit + i) % (CHAR_BIT * sizeof(T));
			*(ptr + byteOffset) |= 1 << bitOffset;
		}
	}

	template<class T, class N>
	static inline void clearBits(T *ptr, N bit, N count = 1)
	{
		for (N i = 0; i < count; i++)
		{
			N byteOffset = (bit + i) / (CHAR_BIT * sizeof(T));
			N bitOffset = (bit + i) % (CHAR_BIT * sizeof(T));
			*(ptr + byteOffset) &= ~(1 << bitOffset);
		}
	}

	template<class T, class U, class N>
	static inline void copyBits(T *srcPtr, U *destPtr, N srcBit, N destBit, N count = 1)
	{
		for (N i = 0; i < count; i++)
		{
			N byteOffsetSrc = (srcBit + i) / (CHAR_BIT * sizeof(T));
			N bitOffsetSrc = (srcBit + i) % (CHAR_BIT * sizeof(T));
			N byteOffsetDest = (destBit + i) / (CHAR_BIT * sizeof(U));
			N bitOffsetDest = (destBit + i) % (CHAR_BIT * sizeof(U));

			if (*(srcPtr + byteOffsetSrc) & (1 << bitOffsetSrc))
			{
				*(destPtr + byteOffsetDest) |= 1 << bitOffsetDest;
			}
			else
			{
				*(destPtr + byteOffsetDest) &= ~(1 << bitOffsetDest);
			}
		}
	}

	template<class T, class N>
	static inline N bitsToStructs(unsigned char bits)
	{
		unsigned char result = bits / (CHAR_BIT * sizeof(T));
		if (bits % (CHAR_BIT * sizeof(T)) == 0)
			return result;
		else
			return result + 1;
	}
};