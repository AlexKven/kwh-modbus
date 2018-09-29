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

	DEFINE_TASK(asyncFunc, long, VARS(byte, word));
	ASYNC_FUNC(asyncFunc)
	{
		ASYNC_VAR(0, bte);
		ASYNC_VAR_INIT(1, wrd, 25);
		START_ASYNC;
		bte = 0;
		wrd++;
		YIELD_ASYNC;
		bte = 2;
		wrd++;
		YIELD_ASYNC;
		bte++;
		wrd++;
		YIELD_ASYNC;
		bte = 5;
		wrd++;
		RESULT_ASYNC(45);
		END_ASYNC;
	}

	int asyncTest(int __resume_line__)
	{

	}
};

TEST_F(AsyncAwaitTests, NoParams_FiveYields)
{
	CREATE_TASK(noParams_Five, task);
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());
	ASSERT_FALSE(task());
	ASSERT_TRUE(task());
	ASSERT_EQ(task.result(), 5);
}

TEST_F(AsyncAwaitTests, NoParams_FiveYields_Sync)
{
	CREATE_TASK(noParams_Five, task);
	ASSERT_EQ(task.runSynchronously(), 5);
}