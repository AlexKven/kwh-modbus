#pragma once
#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#else
#include "../arduinoMacros/arduinoMacros.h"
#endif
#include "../bitFunctions/BitFunctions.hpp"

template<typename T, int B>
class DenseShiftBuffer
{
private_testable:
	byte *_buffer;
	int _capacity;
	int _current;

public:
	DenseShiftBuffer(int capacity)
	{
		_capacity = capacity;
		_current = capacity - 1;
		_buffer = new byte[BitFunctions::bitsToBytes(_capacity * B)];
	}

	int getCapacity()
	{
		return _capacity;
	}

	void push(T item)
	{
		_current++;
		if (_current >= _capacity)
			_current = 0;
		BitFunctions::copyBits(&item, _buffer, 0, _current * B, B);
	}

	T get(int index)
	{
		int actualIndex = _current - index;
		if (actualIndex < 0)
			actualIndex += _capacity;
		T result = 0;
		BitFunctions::copyBits(_buffer, &result, actualIndex * B, 0, B);
		return result;
	}
};