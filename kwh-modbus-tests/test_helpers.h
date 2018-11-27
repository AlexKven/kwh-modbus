#pragma once
#include <stdarg.h>
#include "pch.h"
#include <string>

#define MockNewMethod(NAME, ...) class MockableMethod_ ## NAME { \
public: virtual void method(__VA_ARGS__) {}}; Mock<MockableMethod_ ## NAME> NAME; \
Fake(Method(NAME, method))

template<class T>
T maxOfUnsigned()
{
	return (T)-1;
}

template<class A>
void setArray(void* arr, A head)
{
	A *headArr = (A*)arr;
	headArr[0] = head;
}

template<class A, class ...B>
void setArray(void* arr, A head, B... args)
{
	A *headArr = (A*)arr;
	headArr[0] = head;
	setArray(headArr + 1, args...);
}

template<class A>
void parseArray(void* arr, A &head)
{
	A *headArr = (A*)arr;
	head = headArr[0];
}

template<class A, class ...B>
void parseArray(void* arr, A &head, B &... args)
{
	A *headArr = (A*)arr;
	head = headArr[0];
	parseArray(headArr + 1, args...);
}

template<class A>
A revBytes(A input)
{
	A result = input;
	char *rptr = (char*)&result;
	char *iptr = (char*)&input;
	int size = sizeof(A);
	for (int i = 0; i < size; i++)
	{
		rptr[size - 1 - i] = iptr[i];
	}
	return result;
}

template<class A>
void revBytesAll(A &head)
{
	head = revBytes(head);
}

template<class A, class ...B>
void revBytesAll(A &head, B &... args)
{
	head = revBytes(head);
	revBytesAll(args...);
}

template<class A>
void assertArrayEq(void* arr, A head)
{
	A *headArr = (A*)arr;
	ASSERT_EQ(headArr[0], head);
}

template<class A, class ...B>
void assertArrayEq(void* arr, A head, B... args)
{
	A *headArr = (A*)arr;
	ASSERT_EQ(headArr[0], head);
	assertArrayEq(headArr + 1, args...);
}

std::string stringifyCharArray(int length, char* chars);