#pragma once
#include <stdarg.h>
#include "pch.h"

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