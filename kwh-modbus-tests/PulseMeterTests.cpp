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