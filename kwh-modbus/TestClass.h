#pragma once
class TestClass
{
private:
	int _Value = 5;
public:
	TestClass();
	~TestClass();
	int get_Value();
	void set_Value(int value);
};

