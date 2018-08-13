#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/interfaces/ISerialStream.h"
#include "../kwh-modbus/noArduino/SystemFunctions.h"
#include "../kwh-modbus/libraries/resillientTask/ResillientTask.h"
#include "test_helpers.h"
#include "WindowsSystemFunctions.h"
#include <functional>
#include <queue>

#define USE_FAKE_SYSTEM Mock<ISystemFunctions> fakeSystem

using namespace fakeit;

class TestResillientTask : public ResillientTask<ISystemFunctions>
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

class ResillientTaskTests : public ::testing::Test
{
protected:
	TestResillientTask *task;
	WindowsSystemFunctions *system;
public:
	void SetUp()
	{
		task = new TestResillientTask();
		system = new WindowsSystemFunctions();
		task->setSystem(system);
	}

	void TearDown()
	{
		delete task, system;
	}
};

TEST_F(ResillientTaskTests, Task_CompletesImmediately)
{
	task->beginLambda = []() {return TaskComplete; };
	int workCount = 1;
	auto status = TaskNotStarted;

	while (!task->work(status))
		workCount++;

	ASSERT_EQ(status, TaskComplete);
	ASSERT_EQ(task->_currentTries, 1);
	ASSERT_EQ(workCount, 1);
}

TEST_F(ResillientTaskTests, Task_CompletesOneCheck)
{
	task->beginLambda = []() { return TaskInProgress; };
	task->checkLambda = []() { return TaskComplete; };
	int workCount = 1;
	auto status = TaskNotStarted;

	while (!task->work(status))
		workCount++;

	ASSERT_EQ(status, TaskComplete);
	ASSERT_EQ(task->_currentTries, 1);
	ASSERT_EQ(workCount, 2);
}

TEST_F(ResillientTaskTests, Task_CompletesThreeChecks)
{
	int checks = 3;
	task->beginLambda = []() { return TaskInProgress; };
	task->checkLambda = [&]()
	{
		checks--;
		if (checks == 0)
			return TaskComplete;
		else
			return TaskInProgress;
	};
	int workCount = 1;
	auto status = TaskNotStarted;

	while (!task->work(status))
		workCount++;

	ASSERT_EQ(status, TaskComplete);
	ASSERT_EQ(task->_currentTries, 1);
	ASSERT_EQ(workCount, 4);
}

TEST_F(ResillientTaskTests, Task_CompletesThreeChecks_FourAttempts)
{
	int checks = 3;
	int attempts = 4;
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
	int workCount = 1;
	auto status = TaskNotStarted;

	while (!task->work(status))
		workCount++;

	ASSERT_EQ(status, TaskComplete);
	ASSERT_EQ(task->_currentTries, 4);
	ASSERT_EQ(workCount, 16);
}

TEST_F(ResillientTaskTests, Task_ExceededMaxTries)
{
	int checks = 3;
	int attempts = 4;
	task->setMaxTries(3);
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
	int workCount = 1;
	auto status = TaskNotStarted;

	while (!task->work(status))
		workCount++;

	ASSERT_EQ(status, TaskFullyAttempted);
	ASSERT_EQ(task->_currentTries, 3);
	ASSERT_EQ(workCount, 13);
}