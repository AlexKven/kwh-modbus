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

TEST_F(TimeManagerTests, getTimeCodeForClock_simpleCaseWithSeconds)
{
	auto result = timeManager->getTimeCodeForClock(TimeScale::sec1, 703);

	ASSERT_EQ(result, 703);
}

TEST_F(TimeManagerTests, getTimeCodeForClock_simpleCaseWithMinutes)
{
	auto result = timeManager->getTimeCodeForClock(TimeScale::min1, 360);

	ASSERT_EQ(result, 6);
}

TEST_F(TimeManagerTests, getTimeCodeForClock_lastSecondOfAMinute)
{
	auto result = timeManager->getTimeCodeForClock(TimeScale::min1, 359);

	ASSERT_EQ(result, 5);
}

TEST_F(TimeManagerTests, getTimeCodeForClock_currentTimeWithHours)
{
	timeManager->setClock(8000);
	auto result = timeManager->getTimeCodeForClock(TimeScale::hr1);

	ASSERT_EQ(result, 2);
}

TEST_F(TimeManagerTests, getTimeCodeForClock_simpleCaseWithMilliseconds)
{
	auto result = timeManager->getTimeCodeForClock(TimeScale::ms250, 1000);

	ASSERT_EQ(result, 4000);
}

TEST_F(TimeManagerTests, getTimeCodeForClock_overflowWithMilliseconds_belowEdge)
{
	auto clock = maxOfUnsigned<uint32_t>();
	clock >>= 2; // 2 ^ 30 - 1;
	auto result = timeManager->getTimeCodeForClock(TimeScale::ms250, clock);

	ASSERT_EQ(result, maxOfUnsigned<uint32_t>() - 3);
}

TEST_F(TimeManagerTests, getTimeCodeForClock_overflowWithMilliseconds_atEdge)
{
	auto clock = maxOfUnsigned<uint32_t>();
	clock >>= 2;
	clock += 1; // 2 ^ 30;
	auto result = timeManager->getTimeCodeForClock(TimeScale::ms250, clock);

	ASSERT_EQ(result, 0);
}

TEST_F(TimeManagerTests, getTimeCodeForClock_overflowWithMilliseconds_aboveEdge)
{
	auto clock = maxOfUnsigned<uint32_t>();
	clock >>= 2;
	clock += 20; // 2 ^ 30 + 19;
	auto result = timeManager->getTimeCodeForClock(TimeScale::ms250, clock);

	ASSERT_EQ(result, 76);
}

TEST_F(TimeManagerTests, getTimeCodeForClock_overflowWithMilliseconds_atSecondEdge)
{
	auto clock = maxOfUnsigned<uint32_t>();
	clock >>= 1;
	clock += 1; // 2 ^ 31;
	auto result = timeManager->getTimeCodeForClock(TimeScale::ms250, clock);

	ASSERT_EQ(result, 0);
}

TEST_F(TimeManagerTests, getClockForTimeCode_simpleCaseWithSeconds)
{
	auto result = timeManager->getClockForTimeCode(TimeScale::sec1, 703);

	ASSERT_EQ(result, 703);
}

TEST_F(TimeManagerTests, getClockForTimeCode_simpleCaseWithMilliseconds)
{
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, 24);

	ASSERT_EQ(result, 6);
}

TEST_F(TimeManagerTests, getClockForTimeCode_fractionalCaseWithMilliseconds)
{
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, 30);

	ASSERT_EQ(result, 7);
}

TEST_F(TimeManagerTests, getClockForTimeCode_simpleCaseWithMinutes)
{
	auto result = timeManager->getClockForTimeCode(TimeScale::min1, 111);

	ASSERT_EQ(result, 6660);
}

TEST_F(TimeManagerTests, getClockForTimeCode_simpleCaseWithDays)
{
	auto result = timeManager->getClockForTimeCode(TimeScale::hr24, 3);

	ASSERT_EQ(result, 259200);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case0)
{
	auto timeCode = maxOfUnsigned<uint32_t>();
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, 1);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case1)
{
	auto timeCode = maxOfUnsigned<uint32_t>();
	auto edge = 0x40000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge - 10);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case2)
{
	auto timeCode = maxOfUnsigned<uint32_t>();
	auto edge = 0x40000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge + 10);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case3)
{
	auto timeCode = maxOfUnsigned<uint32_t>() - 703;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, 1);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case4)
{
	auto timeCode = maxOfUnsigned<uint32_t>() - 703;
	auto edge = 0x40000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge - 10);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case5)
{
	auto timeCode = maxOfUnsigned<uint32_t>() - 703;
	auto edge = 0x40000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge + 10);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case6)
{
	auto timeCode = 703;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, 1);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case7)
{
	auto timeCode = 703;
	auto edge = 0x40000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge - 10);
	auto expected = 0x40000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case8)
{
	auto timeCode = 703;
	auto edge = 0x40000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge + 10);
	auto expected = 0x40000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case6_WithCurrentClock)
{
	auto timeCode = 703;
	timeManager->setClock(1);
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case7_WithCurrentClock)
{
	auto timeCode = 703;
	auto edge = 0x40000000;
	timeManager->setClock(edge - 10);
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode);
	auto expected = 0x40000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case9)
{
	auto timeCode = 703;
	auto edge = 0x80000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge - 10);
	auto expected = 0x80000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case10)
{
	auto timeCode = 703;
	auto edge = 0x80000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge + 10);
	auto expected = 0x80000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case11)
{
	auto timeCode = 0x80000000 + 703;
	auto edge = 0x80000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge - 10);
	auto expected = 0x40000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case12)
{
	auto timeCode = 0x80000000 + 703;
	auto edge = 0x80000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge + 10);
	auto expected = 0x40000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F(TimeManagerTests, timeCodeRoundTrip_Days)
{
	auto clock = 200000;
	auto scale = TimeScale::hr24;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock - (clock % (TimeManager::getPeriodFromTimeScale(scale) / 1000)));
}

TEST_F(TimeManagerTests, timeCodeRoundTrip_Hours)
{
	auto clock = 200000;
	auto scale = TimeScale::hr1;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock - (clock % (TimeManager::getPeriodFromTimeScale(scale) / 1000)));
}

TEST_F(TimeManagerTests, timeCodeRoundTrip_HalfHour)
{
	auto clock = 200000;
	auto scale = TimeScale::min30;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock - (clock % (TimeManager::getPeriodFromTimeScale(scale) / 1000)));
}

TEST_F(TimeManagerTests, timeCodeRoundTrip_TenMinutes)
{
	auto clock = 200000;
	auto scale = TimeScale::min10;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock - (clock % (TimeManager::getPeriodFromTimeScale(scale) / 1000)));
}

TEST_F(TimeManagerTests, timeCodeRoundTrip_Minutes)
{
	auto clock = 200000;
	auto scale = TimeScale::min1;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock - (clock % (TimeManager::getPeriodFromTimeScale(scale) / 1000)));
}

TEST_F(TimeManagerTests, timeCodeRoundTrip_QuarterMinute)
{
	auto clock = 200000;
	auto scale = TimeScale::sec15;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock - (clock % (TimeManager::getPeriodFromTimeScale(scale) / 1000)));
}

TEST_F(TimeManagerTests, timeCodeRoundTrip_Seconds)
{
	auto clock = 200000;
	auto scale = TimeScale::sec1;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock);
}

TEST_F(TimeManagerTests, timeCodeRoundTrip_QuarterSecond)
{
	auto clock = 200000;
	auto scale = TimeScale::ms250;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock);
}

TEST_F(TimeManagerTests, timeCodeRoundTrip_QuarterSecondWrongWindow)
{
	auto clock = 200000;
	auto scale = TimeScale::ms250;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock + 0xC0000000));

	ASSERT_EQ(result, clock);
}

TEST_F(TimeManagerTests, timeCodeRoundTrip_QuarterSecondWrongWindowCorrected)
{
	auto clock = 200000 + 0xC0000000;
	auto scale = TimeScale::ms250;
	timeManager->setClock(clock - 5000000);
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock);
}

TEST_F(TimeManagerTests, wasNeverSet_true)
{
	ASSERT_TRUE(timeManager->wasNeverSet());
}

TEST_F(TimeManagerTests, wasNeverSet_false)
{
	timeManager->_clockSet = 5;
	ASSERT_FALSE(timeManager->wasNeverSet());
}