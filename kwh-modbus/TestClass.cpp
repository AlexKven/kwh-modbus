#include "TestClass.h"



TestClass::TestClass()
{
	int(*fMillis)() = func;
	int i = fMillis();
}

 int func()
{
	 return 5;
}


TestClass::~TestClass()
{
}

int TestClass::get_Value()
{
	return _Value;
}

void TestClass::set_Value(int value)
{
	_Value = value;
}
