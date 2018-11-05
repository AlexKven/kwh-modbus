#pragma once

class BitFunctions
{
public:
	static inline char bitsToBytes(unsigned char bits);

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

			if ((*srcPtr + byteOffsetSrc) & (1 << bitOffsetSrc))
			{
				*(destPtr + byteOffsetDest) |= 1 << bitOffsetDest;
			}
			else
			{
				*(destPtr + byteOffsetDest) &= ~(1 << bitOffsetDest);
			}
		}
	}
};