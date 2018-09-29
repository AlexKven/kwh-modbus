#include <tuple>
#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#else
#include "../arduinoMacros/arduinoMacros.h"
#endif

#define _C ,
#define VARS(...) std::tuple<__VA_ARGS__>
#define DEFINE_TASK(FNAME, T_RET, TUPLE_VAR, ...) \
typedef AsyncTaskSpecific<T_RET, TUPLE_VAR, ##__VA_ARGS__> FNAME ## _Task

#define ASYNC_FUNC(FNAME, ...) \
static bool FNAME(FNAME ## _Task::StateParam &state, ##__VA_ARGS__)

#define CREATE_TASK(FNAME, ...) FNAME ## _Task(FNAME, ##__VA_ARGS__);

#define AWAIT_RESULT(FNAME, TNAME) \
*state._line = __LINE__ \
if (TNAME()) { \
*state._result = TNAME.result(); \
return true; \
} \
else \
return false; \
case __LINE__:

#define AWAIT_RETURN(FNAME, TNAME) \
*state._line = __LINE__ \
return (TNAME()); \
case __LINE__:

#define ASYNC_VAR(NUM, NAME) auto &NAME = std::get<NUM>(*state._variables);
#define ASYNC_VAR_INIT(NUM, NAME, INIT) auto &NAME = std::get<NUM>(*state._variables); \
if (*state._line == 0) NAME = INIT;

#define RETURN_ASYNC return true;
#define RESULT_ASYNC(RESULT) *state._result = RESULT; \
return true;

#define START_ASYNC \
switch (*state._line) \
{ \
	case 0:

#define YIELD_ASYNC *state._line = __LINE__; \
return false; \
case __LINE__:

#define END_ASYNC } return true

class IAsyncTask
{
public:
	virtual bool operator()() = 0;
	virtual bool completed() = 0;
};

template<class T>
class AsyncTask : public IAsyncTask
{
protected:
	char _resultVal[sizeof(T)];
	T *_result = (T*)&_resultVal;
	bool _isCompleted = false;
	virtual bool work() = 0;

public:
	bool operator()()
	{
		return work();
	}
	
	bool completed()
	{
		return _isCompleted;
	}

	T& result()
	{
		return (T&)_resultVal;
	}

	T& runSynchronously()
	{
		while (!work());
		return result();
	}
};

template<>
class AsyncTask<void> : public IAsyncTask
{
protected:
	char _resultVal[1];
	void *_result = (void*)&_resultVal;
	bool _isCompleted = false;
	virtual bool work() = 0;

public:
	bool operator()()
	{
		return work();
	}

	bool completed()
	{
		return _isCompleted;
	}

	void result()
	{
	}

	void runSynchronously()
	{
		while (!work());
	}
};

template<class TReturn, class TupleVar, class ...TParams>
class AsyncTaskSpecific : public AsyncTask<TReturn>
{
public:
	struct StateParam
	{
	public:
		StateParam(TupleVar *vars, int *line, TReturn *result)
		{
			_variables = vars;
			_line = line;
			_result = result;
		}
		TupleVar *_variables;
		int *_line;
		TReturn *_result;
	};
private:
	std::tuple<TParams...> _parameters;
	bool(*_func)(StateParam&, TParams...);
	TupleVar _variables;
	int _curLine = 0;

	template<int ...> struct seq {};

	template<int N, int ...S> struct gens : gens<N - 1, N - 1, S...> {};

	template<int ...S> struct gens<0, S...> { typedef seq<S...> type; };

protected:
	bool work()
	{
		_isCompleted = callFunc(typename gens<sizeof...(TParams)>::type());
		return _isCompleted;
	}

	template<int ...S>
	bool callFunc(seq<S...>)
	{
		StateParam sp = StateParam(&_variables, &_curLine, _result);
		return _func(sp, std::get<S>(_parameters) ...);
	}
public:

	AsyncTaskSpecific(bool(*ptr)(StateParam&, TParams...), TParams... params)
	{
		_func = ptr;
		_parameters = std::make_tuple(params...);
	}
};