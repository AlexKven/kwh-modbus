#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/timeManager/TimeManager.h"
#include "test_helpers.h"

using namespace fakeit;

class TimeManagerTests : public ::testing::Test
{
protected:
	TimeManager *timeManager;

public:
	void SetUp()
	{
		timeManager = new TimeManager();
	}

	void TearDown()
	{
		delete timeManager;
	}
};

TEST_F(TimeManagerTests, getClock)
{
	timeManager->_initialClock = 537408000;
	timeManager->_clockSet = 20000000000;
	timeManager->_curTime = 22000500000;
	auto clock = timeManager->getClock();

	ASSERT_EQ(clock, 537410000);
}

TEST_F(TimeManagerTests, setClock)
{
	timeManager->_curTime = 4000000000;
	timeManager->setClock(537408000);

	ASSERT_EQ(timeManager->_initialClock, 537408000);
	ASSERT_EQ(timeManager->_clockSet, 4000000000);
}

TEST_F(TimeManagerTests, getTimeCodeForTime_simpleCaseWithSeconds)
{
	auto result = timeManager->getTimeCodeForTime(TimeScale::sec1, 703);

	ASSERT_EQ(result, 703);
}

TEST_F(TimeManagerTests, getTimeCodeForTime_simpleCaseWithMinutes)
{
	auto result = timeManager->getTimeCodeForTime(TimeScale::min1, 360);

	ASSERT_EQ(result, 6);
}

TEST_F(TimeManagerTests, getTimeCodeForTime_lastSecondOfAMinute)
{
	auto result = timeManager->getTimeCodeForTime(TimeScale::min1, 359);

	ASSERT_EQ(result, 5);
}

TEST_F(TimeManagerTests, getTimeCodeForTime_currentTimeWithHours)
{
	timeManager->setClock(8000);
	auto result = timeManager->getTimeCodeForTime(TimeScale::hr1);

	ASSERT_EQ(result, 2);
}

TEST_F(TimeManagerTests, getTimeCodeForTime_simpleCaseWithMilliseconds)
{
	auto result = timeManager->getTimeCodeForTime(TimeScale::ms250, 1000);

	ASSERT_EQ(result, 4000);
}

TEST_F(TimeManagerTests, getTimeCodeForTime_overflowWithMilliseconds_belowEdge)
{
	auto clock = maxOfUnsigned<uint32_t>();
	clock >>= 2; // 2 ^ 30 - 1;
	auto result = timeManager->getTimeCodeForTime(TimeScale::ms250, clock);

	ASSERT_EQ(result, maxOfUnsigned<uint32_t>() - 3);
}

TEST_F(TimeManagerTests, getTimeCodeForTime_overflowWithMilliseconds_atEdge)
{
	auto clock = maxOfUnsigned<uint32_t>();
	clock >>= 2;
	clock += 1; // 2 ^ 30;
	auto result = timeManager->getTimeCodeForTime(TimeScale::ms250, clock);

	ASSERT_EQ(result, 0);
}

TEST_F(TimeManagerTests, getTimeCodeForTime_overflowWithMilliseconds_aboveEdge)
{
	auto clock = maxOfUnsigned<uint32_t>();
	clock >>= 2;
	clock += 20; // 2 ^ 30 + 19;
	auto result = timeManager->getTimeCodeForTime(TimeScale::ms250, clock);

	ASSERT_EQ(result, 76);
}

TEST_F(TimeManagerTests, getTimeCodeForTime_overflowWithMilliseconds_atSecondEdge)
{
	auto clock = maxOfUnsigned<uint32_t>();
	clock >>= 1;
	clock += 1; // 2 ^ 31;
	auto result = timeManager->getTimeCodeForTime(TimeScale::ms250, clock);

	ASSERT_EQ(result, 0);
}