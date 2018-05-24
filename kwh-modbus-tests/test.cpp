#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/TestClass.h"

using namespace fakeit;

class Test1
{
public:
	virtual int Sum(int x, int y) = 0;
};

class Test2 : public Test1
{
public:
	virtual int Sum(int x, int y)
	{
		TestClass tc;
		return y + x;
	}
};

TEST(TestCaseName, TestName) {
	Mock<Test1> mock = Mock<Test1>();
	When(Method(mock, Sum)).Return(5);
	Test1 &ts = mock.get();
	EXPECT_EQ(ts.Sum(1, 1), 5);
}

