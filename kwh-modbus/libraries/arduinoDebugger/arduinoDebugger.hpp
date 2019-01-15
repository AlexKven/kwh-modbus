#pragma once
#include "../asyncAwait/AsyncAwait.hpp"

struct DebugStatus
{

};

template<class TupleVar, class ...TParams>
class ArduinoDebugger : AsyncTask<DebugStatus>
{
private:
	TupleVar variables;
	Tuple<TParams...> parameters;
	int inputBufferLength = 100;
	char *inputBuffer;
public:


	virtual bool work()
	{
	}
};