#include "../modbusSlave/ModbusSlave.hpp"
#include "../resillientTask/ResillientTask.h"

template<typename TBase, typename TSystemFunctions>
class ModbusSlaveTask<TBase, TSystemFunctions> : public ResillientTask<TSystemFunctions>, public TBase
{
protected_testable:
	TaskStatus begin()
	{
		return TaskNotStarted;
	}

	TaskStatus check()
	{
		return TaskInProgress;
	}

	TaskStatus retry()
	{
		return TaskNotStarted;
	}
};