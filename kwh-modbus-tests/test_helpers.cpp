#include "pch.h"
#include "test_helpers.h"

std::string stringifyCharArray(int length, char * chars)
{
	char *stringChars = new char[length + 1];
	stringChars[length] = '\0';
	for (int i = 0; i < length; i++)
	{
		stringChars[i] = chars[i];
	}
	auto res = std::string((const char*)stringChars);
	delete[] stringChars;
	return res;
}