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