#include "../modbusSlave/ModbusSlave.hpp"
#include "../resilientTask/ResilientTask.hpp"

template<typename TBase, typename TSystemFunctions>
class ModbusSlaveTask<TBase, TSystemFunctions> : public ResilientTask<TSystemFunctions>, public TBase
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