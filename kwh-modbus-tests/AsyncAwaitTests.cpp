#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/asyncAwait/AsyncAwait.hpp"
#include "test_helpers.h"

using namespace fakeit;

class AsyncAwaitTests : public ::testing::Test
{
public:
	void SetUp()
	{
	}

	void TearDown()
	{
	}

	DEFINE_TASK(noParams_Five, int, VARS());
	ASYNC_FUNC(noParams_Five)
	{
		START_ASYNC;
		YIELD_ASYNC;
		YIELD_ASYNC;
		YIELD_ASYNC;
		RESULT_ASYNC(5);
		END_ASYNC;
	}

	DEFINE_TASK(noParams_Void, void, VARS());
	ASYNC_FUNC(noParams_Void)
	{
		START_ASYNC;
		YIELD_ASYNC;
		YIELD_ASYNC;
		YIELD_ASYNC;
		RETURN_ASYNC;
		END_ASYNC;
	}

	DEFINE_TASK(multiply, int, VARS(int, int), int, int);
	ASYNC_FUNC(multiply, int x, int y)
	{
		ASYNC_VAR(0, i);
		ASYNC_VAR_INIT(1, acc, 0);
		START_ASYNC;
		for (i = 0; i < x; i++)
		{
			acc += y;
			YIELD_ASYNC;
		}
		RESULT_ASYNC(acc);
		END_ASYNC;
	}

	DEFINE_TASK(power, int, VARS(int, int, multiply_Task), int, int);
	ASYNC_FUNC(power, int x, int y)
	{
		ASYNC_VAR(0, i);
		ASYNC_VAR_INIT(1, acc, 1);
		ASYNC_VAR(2, multiplyTask);
		START_ASYNC;
		for (i = 0; i < x; i++)
		{
			multiplyTask = CREATE_TASK(multiply, acc, y);
			acc += y;
			YIELD_ASYNC;
		}
		RESULT_ASYNC(acc);
		END_ASYNC;
	}

	int asyncTest(int __resume_line__)
	{

	}
};

TEST_F(AsyncAwaitTests, NoParams_FiveYields)
{
	auto task = CREATE_TASK(noParams_Five);
	ASSERT_FALSE(task.completed());
	ASSERT_FALSE(task());
	ASSERT_FALSE(task.completed());
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());
	ASSERT_TRUE(task());
	ASSERT_TRUE(task.completed());
	ASSERT_EQ(task.result(), 5);
}

TEST_F(AsyncAwaitTests, NoParams_FiveYields_Sync)
{
	auto task = CREATE_TASK(noParams_Five);
	ASSERT_FALSE(task.completed());
	ASSERT_EQ(task.runSynchronously(), 5);
}

TEST_F(AsyncAwaitTests, NoParams_FiveYieldsVoid)
{
	auto task = CREATE_TASK(noParams_Void);
	ASSERT_FALSE(task.completed());
	ASSERT_FALSE(task());
	ASSERT_FALSE(task.completed());
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());
	ASSERT_TRUE(task());
	ASSERT_TRUE(task.completed());
	task.result();
}

TEST_F(AsyncAwaitTests, NoParams_FiveYieldsVoid_Sync)
{
	auto task = CREATE_TASK(noParams_Void);
	ASSERT_FALSE(task.completed());
	task.runSynchronously();
}

TEST_F(AsyncAwaitTests, Multiply)
{
	auto task = CREATE_TASK(multiply, 4, 7);
	ASSERT_FALSE(task.completed());
	ASSERT_FALSE(task());
	ASSERT_FALSE(task.completed());
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());
	ASSERT_TRUE(task());
	ASSERT_TRUE(task.completed());
	ASSERT_EQ(task.result(), 28);
}

TEST_F(AsyncAwaitTests, Multiply_Sync)
{
	auto task = CREATE_TASK(multiply, 4, 7);
	ASSERT_FALSE(task.completed());
	ASSERT_EQ(task.runSynchronously(), 28);
}

TEST_F(AsyncAwaitTests, MultiplyZero)
{
	auto task = CREATE_TASK(multiply, 0, 7);
	ASSERT_FALSE(task.completed());
	ASSERT_TRUE(task());
	ASSERT_TRUE(task.completed());
	ASSERT_EQ(task.result(), 0);
}

TEST_F(AsyncAwaitTests, MultiplyZero_Sync)
{
	auto task = CREATE_TASK(multiply, 0, 7);
	ASSERT_FALSE(task.completed());
	ASSERT_EQ(task.runSynchronously(), 0);
}