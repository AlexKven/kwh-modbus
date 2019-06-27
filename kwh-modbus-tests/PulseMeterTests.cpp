#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/pulseMeter/PulseMeter.hpp"
#include "test_helpers.h"
#include "PointerTracker.h"

using namespace fakeit;

class PulseMeterTests : public ::testing::Test
{
protected:
	PulseMeter<uint64_t, 13>* pulseMeter;

	PointerTracker tracker;

public:
	void SetUp()
	{
		pulseMeter = new PulseMeter<uint64_t, 13>();
		tracker.addPointer(pulseMeter);
	}

	void TearDown()
	{
	}
};

TEST_F_TRAITS(PulseMeterTests, init_success,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	pulseMeter->init(6, TimeScale::min10, 3.5, 40);

	ASSERT_EQ(pulseMeter->_dataBuffer->getCapacity(), 6);
	ASSERT_EQ(pulseMeter->_dataBuffer->getNumStored(), 0);
	ASSERT_EQ(pulseMeter->_usPeriod, 600000000);
	ASSERT_EQ(pulseMeter->_unitsPerPulse, 3.5);
	word deviceType;
	bool accumulateData;
	TimeScale timeScale;
	byte dataPacketSize;
	DataCollectorDevice::getParametersFromDataCollectorDeviceType(
		pulseMeter->getType(), accumulateData, timeScale, dataPacketSize);
	ASSERT_EQ(accumulateData, true);
	ASSERT_EQ(timeScale, TimeScale::min10);
	ASSERT_EQ(dataPacketSize, 40);
}

TEST_F_TRAITS(PulseMeterTests, readDataPoint_OneSecondPeriod_Success_Past,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	pulseMeter->init(6, TimeScale::sec1, 2.5, 32);
	pulseMeter->_lastUpdateClock = 2500000;
	pulseMeter->_dataBuffer->push(1);
	pulseMeter->_dataBuffer->push(2);
	pulseMeter->_dataBuffer->push(3);
	pulseMeter->_dataBuffer->push(4);
	pulseMeter->_dataBuffer->push(5);
	pulseMeter->_dataBuffer->push(6);
	pulseMeter->_dataBuffer->push(7);

	uint64_t result = 0;
	byte* resultBuffer = (byte*)& result;

	bool success = pulseMeter->readDataPoint(2499998, 0, resultBuffer, 32);

	ASSERT_TRUE(success);
	ASSERT_EQ(result, 5);
}

TEST_F_TRAITS(PulseMeterTests, readDataPoint_OneSecondPeriod_Success_Present,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	pulseMeter->init(6, TimeScale::sec1, 2.5, 32);
	pulseMeter->_lastUpdateClock = 2500000;
	pulseMeter->_dataBuffer->push(1);
	pulseMeter->_dataBuffer->push(2);
	pulseMeter->_dataBuffer->push(3);
	pulseMeter->_dataBuffer->push(4);
	pulseMeter->_dataBuffer->push(5);
	pulseMeter->_dataBuffer->push(6);
	pulseMeter->_dataBuffer->push(7);

	uint64_t result = 0;
	byte* resultBuffer = (byte*)& result;

	bool success = pulseMeter->readDataPoint(2500000, 0, resultBuffer, 32);

	ASSERT_TRUE(success);
	ASSERT_EQ(result, 7);
}

TEST_F_TRAITS(PulseMeterTests, readDataPoint_OneSecondPeriod_Failure_OutOfRangePast,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	pulseMeter->init(6, TimeScale::sec1, 2.5, 32);
	pulseMeter->_lastUpdateClock = 2500000;
	pulseMeter->_dataBuffer->push(1);
	pulseMeter->_dataBuffer->push(2);
	pulseMeter->_dataBuffer->push(3);
	pulseMeter->_dataBuffer->push(4);
	pulseMeter->_dataBuffer->push(5);
	pulseMeter->_dataBuffer->push(6);
	pulseMeter->_dataBuffer->push(7);

	uint64_t result = 0;
	byte* resultBuffer = (byte*)& result;

	bool success = pulseMeter->readDataPoint(2499994, 0, resultBuffer, 32);

	ASSERT_FALSE(success);
	ASSERT_EQ(result, 0);
}

TEST_F_TRAITS(PulseMeterTests, readDataPoint_OneSecondPeriod_Failure_OutOfRangeFuture,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	pulseMeter->init(6, TimeScale::sec1, 2.5, 32);
	pulseMeter->_lastUpdateClock = 2500000;
	pulseMeter->_dataBuffer->push(1);
	pulseMeter->_dataBuffer->push(2);
	pulseMeter->_dataBuffer->push(3);
	pulseMeter->_dataBuffer->push(4);
	pulseMeter->_dataBuffer->push(5);
	pulseMeter->_dataBuffer->push(6);
	pulseMeter->_dataBuffer->push(7);

	uint64_t result = 0;
	byte* resultBuffer = (byte*)& result;

	bool success = pulseMeter->readDataPoint(2500001, 0, resultBuffer, 32);

	ASSERT_FALSE(success);
	ASSERT_EQ(result, 0);
}