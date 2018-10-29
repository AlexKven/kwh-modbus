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