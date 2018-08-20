#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/interfaces/ISerialStream.h"
#include "../kwh-modbus/noArduino/SystemFunctions.h"
#include "../kwh-modbus/libraries/resilientTask/ResilientTask.hpp"
#include "test_helpers.h"
#include "WindowsSystemFunctions.h"
#include <functional>
#include <queue>

#define USE_FAKE_SYSTEM Mock<ISystemFunctions> fakeSystem

using namespace fakeit;

class TestResilientTask : public ResilientTask<ISystemFunctions>
{
public:
	std::function<TaskStatus()> beginLambda;
	std::function<TaskStatus()> checkLambda;
	std::function<TaskStatus()> retryLambda;
	std::function<void()> disposedLambda;
	std::function<TaskStatus(TaskStatus)> onAutoStatusChangeLambda =
		[](TaskStatus status) { return status; };

	TaskStatus begin()
	{
		return beginLambda();
	}

	TaskStatus check()
	{
		return checkLambda();
	}

	TaskStatus retry()
	{
		return retryLambda();
	}

	void disposed()
	{
		disposedLambda();
	}

	TaskStatus onAutoStatusChange(TaskStatus status)
	{
		return onAutoStatusChangeLambda(status);
	}
};

#define RTT_TIMEOUT(timeout_ms) if (system->millis() - _testStartTime >= timeout_ms) \
{ GTEST_FATAL_FAILURE_("Timed out. May be stuck in loop");}

class ResilientTaskTests : public ::testing::Test
{
protected:
	TestResilientTask *task;
	WindowsSystemFunctions *system;
	unsigned long long _testStartTime;
public:
	void SetUp()
	{
		task = new TestResilientTask();
		system = new WindowsSystemFunctions();
		task->setSystem(system);
		_testStartTime = system->millis();
	}

	void TearDown()
	{
		delete task, system;
	}
};

TEST_F(ResilientTaskTests, Task_CompletesImmediately)
{
	bool disposed = false;
	task->disposedLambda = [&disposed]() { disposed = true; };
	task->beginLambda = []() {return TaskComplete; };
	int workCount = 0;
	auto status = TaskNotStarted;

	while (!task->work(status))
	{
		workCount++;
		RTT_TIMEOUT(1000);
	}

	ASSERT_TRUE(disposed);
	ASSERT_EQ(status, TaskComplete);
	ASSERT_EQ(task->_currentTries, 1);
	ASSERT_EQ(workCount, 0);
}

TEST_F(ResilientTaskTests, Task_CompletesOneCheck)
{
	bool disposed = false;
	task->disposedLambda = [&disposed]() { disposed = true; };
	task->beginLambda = []() { return TaskInProgress; };
	task->checkLambda = []() { return TaskComplete; };
	int workCount = 0;
	auto status = TaskNotStarted;

	while (!task->work(status))
	{
		workCount++;
		RTT_TIMEOUT(1000);
	}
	
	ASSERT_TRUE(disposed);
	ASSERT_EQ(status, TaskComplete);
	ASSERT_EQ(task->_currentTries, 1);
	ASSERT_EQ(workCount, 1);
}

TEST_F(ResilientTaskTests, Task_CompletesThreeChecks)
{
	int checks = 3;
	bool disposed = false;
	task->disposedLambda = [&disposed]() { disposed = true; };
	task->beginLambda = []() { return TaskInProgress; };
	task->checkLambda = [&]()
	{
		checks--;
		if (checks == 0)
			return TaskComplete;
		else
			return TaskInProgress;
	};
	int workCount = 0;
	auto status = TaskNotStarted;

	while (!task->work(status))
	{
		workCount++;
		RTT_TIMEOUT(1000);
	}

	ASSERT_TRUE(disposed);
	ASSERT_EQ(status, TaskComplete);
	ASSERT_EQ(task->_currentTries, 1);
	ASSERT_EQ(workCount, 3);
}

TEST_F(ResilientTaskTests, Task_CompletesThreeChecks_FourAttempts)
{
	int checks = 3;
	int attempts = 4;
	bool disposed = false;
	task->disposedLambda = [&disposed]() { disposed = true; };
	task->beginLambda = []()
	{
		return TaskInProgress;
	};
	task->retryLambda = [&]()
	{
		checks = 3;
		return TaskInProgress;
	};
	task->checkLambda = [&]()
	{
		checks--;
		if (checks == 0)
		{
			attempts--;
			if (attempts == 0)
				return TaskComplete;
			else
				return TaskFailure;
		}
		else
			return TaskInProgress;
	};
	int workCount = 0;
	auto status = TaskNotStarted;

	while (!task->work(status))
	{
		workCount++;
		RTT_TIMEOUT(1000);
	}

	ASSERT_TRUE(disposed);
	ASSERT_EQ(status, TaskComplete);
	ASSERT_EQ(task->_currentTries, 4);
	ASSERT_EQ(workCount, 15);
}

TEST_F(ResilientTaskTests, Task_ExceededMaxTries)
{
	int checks = 3;
	int attempts = 4;
	task->setMaxTries(3);
	bool disposed = false;
	task->disposedLambda = [&disposed]() { disposed = true; };
	task->beginLambda = [&]()
	{
		return TaskInProgress;
	};
	task->retryLambda = [&]()
	{
		checks = 3;
		return TaskInProgress;
	};
	task->checkLambda = [&]()
	{
		checks--;
		if (checks == 0)
		{
			attempts--;
			if (attempts == 0)
				return TaskComplete;
			else
				return TaskFailure;
		}
		else
			return TaskInProgress;
	};

	int workCount = 0;
	auto status = TaskNotStarted;

	while (!task->work(status))
	{
		workCount++;
		RTT_TIMEOUT(1000);
	}

	ASSERT_TRUE(disposed);
	ASSERT_EQ(status, TaskFullyAttempted);
	ASSERT_EQ(task->_currentTries, 3);
	ASSERT_EQ(workCount, 12);
}

TEST_F(ResilientTaskTests, Task_ExceededMaxTries_Stubborn)
{
	int checks = 3;
	int attempts = 4;
	task->setMaxTries(3);
	bool disposed = false;
	task->disposedLambda = [&disposed]() { disposed = true; };
	task->beginLambda = [&]()
	{
		return TaskInProgress;
	};
	task->retryLambda = [&]()
	{
		checks = 3;
		return TaskInProgress;
	};
	task->checkLambda = [&]()
	{
		checks--;
		if (checks == 0)
		{
			attempts--;
			if (attempts == 0)
				return TaskComplete;
			else
				return TaskFailure;
		}
		else
			return TaskInProgress;
	};
	bool autoChanged = false;
	task->onAutoStatusChangeLambda = [&autoChanged](TaskStatus status)
	{
		if (status == TaskFullyAttempted)
		{
			if (autoChanged)
				return TaskFullyAttempted;
			autoChanged = true;
			return TaskFailure;
		}
	};

	int workCount = 1;
	auto status = TaskNotStarted;

	while (!task->work(status))
		workCount++;

	ASSERT_TRUE(disposed);
	ASSERT_EQ(status, TaskFullyAttempted);
	ASSERT_EQ(task->_currentTries, 3);
	ASSERT_TRUE(autoChanged);
	ASSERT_EQ(workCount, 13);
}

TEST_F(ResilientTaskTests, Task_ExceededMaxTime)
{
	task->setMaxTimeMicros(175000);
	bool disposed = false;
	task->disposedLambda = [&disposed]() { disposed = true; };
	task->beginLambda = [&]()
	{
		return TaskInProgress;
	};
	task->checkLambda = [&]()
	{
		return TaskInProgress;
	};

	int workCount = 0;
	auto status = TaskNotStarted;

	while (!task->work(status))
	{
		workCount++;
		system->delay(50);
		RTT_TIMEOUT(1000);
	}

	ASSERT_TRUE(disposed);
	ASSERT_EQ(status, TaskTimeOut);
	ASSERT_EQ(task->_currentTries, 1);
	ASSERT_NEAR(workCount, 4, 1);
}

TEST_F(ResilientTaskTests, Task_ExceededMaxTimePerTry)
{
	task->setMaxTimePerTryMicros(50000);
	bool disposed = false;
	int tries = 3;
	int attemptTimeouts = 0;
	task->disposedLambda = [&disposed]() { disposed = true; };
	task->beginLambda = [&]()
	{
		if (tries-- > 0)
			return TaskInProgress;
		else
			return TaskComplete;
	};
	task->retryLambda = task->beginLambda;
	task->checkLambda = [&]()
	{
		return TaskInProgress;
	};
	task->onAutoStatusChangeLambda = [&attemptTimeouts](TaskStatus status)
	{
		if (status == TaskAttemptTimeOut)
			attemptTimeouts++;
		return status;
	};

	int workCount = 0;
	auto status = TaskNotStarted;

	while (!task->work(status))
	{
		workCount++;
		system->delay(25);
		RTT_TIMEOUT(1000);
	}

	ASSERT_TRUE(disposed);
	ASSERT_EQ(status, TaskComplete);
	ASSERT_EQ(task->_currentTries, 4);
	ASSERT_NEAR(workCount, 6, 1);
}

TEST_F(ResilientTaskTests, Task_ExceededMaxTimePerTryAndTotal)
{
	task->setMaxTimePerTryMicros(50000);
	task->setMaxTimeMicros(105000);
	bool disposed = false;
	int tries = 3;
	int attemptTimeouts = 0;
	task->disposedLambda = [&disposed]() { disposed = true; };
	task->beginLambda = [&]()
	{
		tries++;
		return TaskInProgress;
	};
	task->retryLambda = task->beginLambda;
	task->checkLambda = [&]()
	{
		return TaskInProgress;
	};
	task->onAutoStatusChangeLambda = [&attemptTimeouts](TaskStatus status)
	{
		if (status == TaskAttemptTimeOut)
			attemptTimeouts++;
		return status;
	};

	int workCount = 0;
	auto status = TaskNotStarted;

	while (!task->work(status))
	{
		workCount++;
		system->delay(25);
		RTT_TIMEOUT(1000);
	}

	ASSERT_TRUE(disposed);
	ASSERT_EQ(status, TaskTimeOut);
	ASSERT_EQ(task->_currentTries, 3);
	ASSERT_NEAR(workCount, 5, 1);
}

TEST_F(ResilientTaskTests, Task_MinTimePerAttempt)
{
	int checks = 2;
	int attempts = 4;
	bool disposed = false;
	bool tooQuick = false;
	unsigned long long lastTryTime;
	task->disposedLambda = [&disposed]() { disposed = true; };
	task->setMinTimePerTryMicros(50000);
	task->beginLambda = [&]()
	{
		return TaskInProgress;
		lastTryTime = system->micros();
	};
	task->retryLambda = [&]()
	{
		checks = 2;
		unsigned long long time = system->micros();
		tooQuick = tooQuick && (time - lastTryTime < 50000);
		lastTryTime = time;
		return TaskInProgress;
	};
	task->checkLambda = [&]()
	{
		checks--;
		if (checks == 0)
		{
			attempts--;
			if (attempts == 0)
				return TaskComplete;
			else
				return TaskFailure;
		}
		else
			return TaskInProgress;
	};
	int workCount = 0;
	auto status = TaskNotStarted;

	while (!task->work(status))
	{
		workCount++;
		system->delay(15);
		RTT_TIMEOUT(1000);
	}

	ASSERT_FALSE(tooQuick);
	ASSERT_TRUE(disposed);
	ASSERT_EQ(status, TaskComplete);
	ASSERT_EQ(task->_currentTries, 4);
	ASSERT_NEAR(workCount, 14, 2);
}