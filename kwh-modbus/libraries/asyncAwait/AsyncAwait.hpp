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
#define DEFINE_CLASS_TASK(CNAME, FNAME, T_RET, TUPLE_VAR, ...) \
typedef AsyncClassTaskSpecific<CNAME, T_RET, TUPLE_VAR, ##__VA_ARGS__> FNAME ## _Task_ ## CNAME

#define ASYNC_FUNC(FNAME, ...) \
static bool FNAME(FNAME ## _Task::StateParam &state, ##__VA_ARGS__)

#define ASYNC_CLASS_FUNC(CNAME, FNAME, ...) \
bool FNAME(FNAME ## _Task_ ## CNAME::StateParam &state, ##__VA_ARGS__)

#define CREATE_TASK(FNAME, ...) FNAME ## _Task(FNAME, ##__VA_ARGS__);

#define CREATE_ASSIGN_TASK(VNAME, FNAME, ...) VNAME = FNAME ## _Task(FNAME, ##__VA_ARGS__);

#define CREATE_CLASS_TASK(CNAME, CVALUE, FNAME, ...) FNAME ## _Task_ ## CNAME(&CNAME::FNAME, CVALUE, ##__VA_ARGS__);

#define CREATE_ASSIGN_CLASS_TASK(VNAME, CNAME, CVALUE, FNAME, ...) VNAME = FNAME ## _Task_ ## CNAME(FNAME, CVALUE, ##__VA_ARGS__);

#define AWAIT_RESULT(TASK) \
*state._line = __LINE__; \
case __LINE__: \
if (TASK()) { \
*state._result = TNAME.result(); \
return true; \
} \
else \
return false

#define AWAIT_RETURN(TASK) \
*state._line = __LINE__; \
case __LINE__: \
return (TASK())

#define AWAIT(TASK) \
*state._line = __LINE__; \
case __LINE__: \
if (!TASK()) return false

#define ASYNC_VAR(NUM, NAME) auto &NAME = std::get<NUM>(*state._variables);
#define ASYNC_VAR_INIT(NUM, NAME, INIT) auto &NAME = std::get<NUM>(*state._variables); \
if (*state._line == 0) NAME = INIT;

#define RETURN_ASYNC return true;
#define RESULT_ASYNC(TYPE, RESULT) *state._result = (TYPE)RESULT; \
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

	T result()
	{
		T res = *(T*)_resultVal;
		return res;
	}

	T runSynchronously()
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
		StateParam sp = StateParam(&_variables, &_curLine, (TReturn*)&_resultVal);
		return _func(sp, std::get<S>(_parameters) ...);
	}

	template <std::size_t ...I, typename T1, typename T2>
	void copy_tuple_impl(T1 const & from, T2 & to, std::index_sequence<I...>)
	{
		int dummy[] = { (std::get<I>(to) = std::get<I>(from), 0)... };
		static_cast<void>(dummy);
	}
public:
	AsyncTaskSpecific(bool(*ptr)(StateParam&, TParams...), TParams... params)
	{
		_func = ptr;
		_parameters = std::make_tuple(params...);
	}

	AsyncTaskSpecific(AsyncTaskSpecific<void, TReturn, TupleVar, TParams...> &copy)
	{
		_func = copy._func;
		copy_tuple_impl(copy._parameters, _parameters);
		copy_tuple_impl(copy._variables, _variables);
		_isCompleted = copy._isCompleted;
		for (int i = 0; i < sizeof(TResult); i++)
		{
			_resultVal[i] = copy._resultVal[i];
		}
		_curLine = copy._curLine;
	}

	AsyncTaskSpecific()
	{
		_func = nullptr;
	}
};

template<class TCls, class TReturn, class TupleVar, class ...TParams>
class AsyncClassTaskSpecific : public AsyncTask<TReturn>
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
	bool (TCls::*_func)(StateParam&, TParams...);
	TCls *_funcLocation;
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
		StateParam sp = StateParam(&_variables, &_curLine, (TReturn*)&_resultVal);
		return (_funcLocation->*_func)(sp, std::get<S>(_parameters) ...);
	}

	template <std::size_t ...I, typename T1, typename T2>
	void copy_tuple_impl(T1 const & from, T2 & to, std::index_sequence<I...>)
	{
		int dummy[] = { (std::get<I>(to) = std::get<I>(from), 0)... };
		static_cast<void>(dummy);
	}
public:
	AsyncClassTaskSpecific(bool(TCls::*ptr)(StateParam&, TParams...), TCls *funcLocation, TParams... params)
	{
		_func = ptr;
		_funcLocation = funcLocation;
		_parameters = std::make_tuple(params...);
	}

	AsyncClassTaskSpecific(AsyncTaskSpecific<TCls, TReturn, TupleVar, TParams...> &copy)
	{
		_func = copy._func;
		_funcLocation = copy._funcLocation;
		copy_tuple_impl(copy._parameters, _parameters);
		copy_tuple_impl(copy._variables, _variables);
		_isCompleted = copy._isCompleted;
		for (int i = 0; i < sizeof(TResult); i++)
		{
			_resultVal[i] = copy._resultVal[i];
		}
		_curLine = copy._curLine;
	}

	AsyncClassTaskSpecific()
	{
		_func = nullptr;
	}
};