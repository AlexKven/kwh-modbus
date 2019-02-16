#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/timeManager/TimeManager.h"
#include "test_helpers.h"
#include "PointerTracker.h"

using namespace fakeit;

class TimeManagerTests : public ::testing::Test
{
protected:
	TimeManager *timeManager;

	PointerTracker tracker;

public:
	void SetUp()
	{
		timeManager = new TimeManager();
		tracker.addPointer(timeManager);
	}

	void TearDown()
	{
	}
};

TEST_F_TRAITS(TimeManagerTests, getClock,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	timeManager->_initialClock = 537408000;
	timeManager->_clockSet = 20000000000;
	timeManager->_curTime = 22000500000;
	auto clock = timeManager->getClock();

	ASSERT_EQ(clock, 537410000);
}

TEST_F_TRAITS(TimeManagerTests, setClock,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	timeManager->_curTime = 4000000000;
	timeManager->setClock(537408000);

	ASSERT_EQ(timeManager->_initialClock, 537408000);
	ASSERT_EQ(timeManager->_clockSet, 4000000000);
}

TEST_F_TRAITS(TimeManagerTests, getTimeCodeForClock_simpleCaseWithSeconds,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = timeManager->getTimeCodeForClock(TimeScale::sec1, 703);

	ASSERT_EQ(result, 703);
}

TEST_F_TRAITS(TimeManagerTests, getTimeCodeForClock_simpleCaseWithMinutes,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = timeManager->getTimeCodeForClock(TimeScale::min1, 360);

	ASSERT_EQ(result, 6);
}

TEST_F_TRAITS(TimeManagerTests, getTimeCodeForClock_lastSecondOfAMinute,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto result = timeManager->getTimeCodeForClock(TimeScale::min1, 359);

	ASSERT_EQ(result, 5);
}

TEST_F_TRAITS(TimeManagerTests, getTimeCodeForClock_currentTimeWithHours,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	timeManager->setClock(8000);
	auto result = timeManager->getTimeCodeForClock(TimeScale::hr1);

	ASSERT_EQ(result, 2);
}

TEST_F_TRAITS(TimeManagerTests, getTimeCodeForClock_simpleCaseWithMilliseconds,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = timeManager->getTimeCodeForClock(TimeScale::ms250, 1000);

	ASSERT_EQ(result, 4000);
}

TEST_F_TRAITS(TimeManagerTests, getTimeCodeForClock_overflowWithMilliseconds_belowEdge,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto clock = maxOfUnsigned<uint32_t>();
	clock >>= 2; // 2 ^ 30 - 1;
	auto result = timeManager->getTimeCodeForClock(TimeScale::ms250, clock);

	ASSERT_EQ(result, maxOfUnsigned<uint32_t>() - 3);
}

TEST_F_TRAITS(TimeManagerTests, getTimeCodeForClock_overflowWithMilliseconds_atEdge,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto clock = maxOfUnsigned<uint32_t>();
	clock >>= 2;
	clock += 1; // 2 ^ 30;
	auto result = timeManager->getTimeCodeForClock(TimeScale::ms250, clock);

	ASSERT_EQ(result, 0);
}

TEST_F_TRAITS(TimeManagerTests, getTimeCodeForClock_overflowWithMilliseconds_aboveEdge,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto clock = maxOfUnsigned<uint32_t>();
	clock >>= 2;
	clock += 20; // 2 ^ 30 + 19;
	auto result = timeManager->getTimeCodeForClock(TimeScale::ms250, clock);

	ASSERT_EQ(result, 76);
}

TEST_F_TRAITS(TimeManagerTests, getTimeCodeForClock_overflowWithMilliseconds_atSecondEdge,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto clock = maxOfUnsigned<uint32_t>();
	clock >>= 1;
	clock += 1; // 2 ^ 31;
	auto result = timeManager->getTimeCodeForClock(TimeScale::ms250, clock);

	ASSERT_EQ(result, 0);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_simpleCaseWithSeconds,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = timeManager->getClockForTimeCode(TimeScale::sec1, 703);

	ASSERT_EQ(result, 703);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_simpleCaseWithMilliseconds,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, 24);

	ASSERT_EQ(result, 6);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_fractionalCaseWithMilliseconds,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, 30);

	ASSERT_EQ(result, 7);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_simpleCaseWithMinutes,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = timeManager->getClockForTimeCode(TimeScale::min1, 111);

	ASSERT_EQ(result, 6660);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_simpleCaseWithDays,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = timeManager->getClockForTimeCode(TimeScale::hr24, 3);

	ASSERT_EQ(result, 259200);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case0,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = maxOfUnsigned<uint32_t>();
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, 1);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case1,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = maxOfUnsigned<uint32_t>();
	auto edge = 0x40000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge - 10);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case2,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = maxOfUnsigned<uint32_t>();
	auto edge = 0x40000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge + 10);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case3,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = maxOfUnsigned<uint32_t>() - 703;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, 1);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case4,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = maxOfUnsigned<uint32_t>() - 703;
	auto edge = 0x40000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge - 10);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case5,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = maxOfUnsigned<uint32_t>() - 703;
	auto edge = 0x40000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge + 10);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case6,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = 703;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, 1);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case7,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = 703;
	auto edge = 0x40000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge - 10);
	auto expected = 0x40000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case8,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = 703;
	auto edge = 0x40000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge + 10);
	auto expected = 0x40000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case6_WithCurrentClock,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = 703;
	timeManager->setClock(1);
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode);
	auto expected = timeCode >> 2;

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case7_WithCurrentClock,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = 703;
	auto edge = 0x40000000;
	timeManager->setClock(edge - 10);
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode);
	auto expected = 0x40000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case9,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = 703;
	auto edge = 0x80000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge - 10);
	auto expected = 0x80000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case10,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = 703;
	auto edge = 0x80000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge + 10);
	auto expected = 0x80000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case11,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = 0x80000000 + 703;
	auto edge = 0x80000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge - 10);
	auto expected = 0x40000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, getClockForTimeCode_edgeCaseWithMilliseconds_case12,
	Type, Unit, Threading, Single, Determinism, Static, Case, Edge)
{
	auto timeCode = 0x80000000 + 703;
	auto edge = 0x80000000;
	auto result = timeManager->getClockForTimeCode(TimeScale::ms250, timeCode, edge + 10);
	auto expected = 0x40000000 + (timeCode >> 2);

	ASSERT_EQ(result, expected);
}

TEST_F_TRAITS(TimeManagerTests, timeCodeRoundTrip_Days,
	Type, Stress, Threading, Single, Determinism, Static, Case, Typical)
{
	auto clock = 200000;
	auto scale = TimeScale::hr24;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock - (clock % (TimeManager::getPeriodFromTimeScale(scale) / 1000)));
}

TEST_F_TRAITS(TimeManagerTests, timeCodeRoundTrip_Hours,
	Type, Stress, Threading, Single, Determinism, Static, Case, Typical)
{
	auto clock = 200000;
	auto scale = TimeScale::hr1;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock - (clock % (TimeManager::getPeriodFromTimeScale(scale) / 1000)));
}

TEST_F_TRAITS(TimeManagerTests, timeCodeRoundTrip_HalfHour,
	Type, Stress, Threading, Single, Determinism, Static, Case, Typical)
{
	auto clock = 200000;
	auto scale = TimeScale::min30;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock - (clock % (TimeManager::getPeriodFromTimeScale(scale) / 1000)));
}

TEST_F_TRAITS(TimeManagerTests, timeCodeRoundTrip_TenMinutes,
	Type, Stress, Threading, Single, Determinism, Static, Case, Typical)
{
	auto clock = 200000;
	auto scale = TimeScale::min10;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock - (clock % (TimeManager::getPeriodFromTimeScale(scale) / 1000)));
}

TEST_F_TRAITS(TimeManagerTests, timeCodeRoundTrip_Minutes,
	Type, Stress, Threading, Single, Determinism, Static, Case, Typical)
{
	auto clock = 200000;
	auto scale = TimeScale::min1;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock - (clock % (TimeManager::getPeriodFromTimeScale(scale) / 1000)));
}

TEST_F_TRAITS(TimeManagerTests, timeCodeRoundTrip_QuarterMinute,
	Type, Stress, Threading, Single, Determinism, Static, Case, Typical)
{
	auto clock = 200000;
	auto scale = TimeScale::sec15;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock - (clock % (TimeManager::getPeriodFromTimeScale(scale) / 1000)));
}

TEST_F_TRAITS(TimeManagerTests, timeCodeRoundTrip_Seconds,
	Type, Stress, Threading, Single, Determinism, Static, Case, Typical)
{
	auto clock = 200000;
	auto scale = TimeScale::sec1;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock);
}

TEST_F_TRAITS(TimeManagerTests, timeCodeRoundTrip_QuarterSecond,
	Type, Stress, Threading, Single, Determinism, Static, Case, Typical)
{
	auto clock = 200000;
	auto scale = TimeScale::ms250;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock);
}

TEST_F_TRAITS(TimeManagerTests, timeCodeRoundTrip_QuarterSecondWrongWindow,
	Type, Stress, Threading, Single, Determinism, Static, Case, Typical)
{
	auto clock = 200000;
	auto scale = TimeScale::ms250;
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock + 0xC0000000));

	ASSERT_EQ(result, clock);
}

TEST_F_TRAITS(TimeManagerTests, timeCodeRoundTrip_QuarterSecondWrongWindowCorrected,
	Type, Stress, Threading, Single, Determinism, Static, Case, Typical)
{
	auto clock = 200000 + 0xC0000000;
	auto scale = TimeScale::ms250;
	timeManager->setClock(clock - 5000000);
	auto result = timeManager->getClockForTimeCode(scale, timeManager->getTimeCodeForClock(scale, clock));

	ASSERT_EQ(result, clock);
}

TEST_F_TRAITS(TimeManagerTests, wasNeverSet_true,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	ASSERT_TRUE(timeManager->wasTimeNeverSet());
}

TEST_F_TRAITS(TimeManagerTests, wasNeverSet_false,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	timeManager->_clockSet = 5;
	ASSERT_FALSE(timeManager->wasTimeNeverSet());
}