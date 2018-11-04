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
};