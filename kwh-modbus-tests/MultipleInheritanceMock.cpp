#include "pch.h"
#include "fakeit.hpp"

using namespace fakeit;

class I1
{
public:
	virtual int one() = 0;
};

class I2
{
public:
	virtual int two() = 0;
};

class Both : public I1, public I2
{
public:
	virtual int one()
	{
		return 0;
	}
	virtual int two()
	{
		return 0;
	}
	virtual int three()
	{
		return one() + two();
	}
};

class More : public Both
{
public:
	virtual int one()
	{
		return 10;
	}
	virtual int two()
	{
		return 10;
	}
};

TEST(both_mock, three)
{
		More both;
		Mock<I1> mock1((I1&)both);
		Mock<I2> mock2((I2&)both);

		When(Method(mock1, one)).Return(1);
		When(Method(mock2, two)).Return(2);

		ASSERT_EQ(both.three(), 3);
	}