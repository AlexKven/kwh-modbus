#ifdef NO_ARDUINO
#include "../../noArduino/TestHelpers.h"
#include "../../noArduino/ArduinoMacros.h"
#else
#include "../arduinoMacros/arduinoMacros.h"
#endif
#include "../tuple/Tuple.hpp"

//
// Tuple implementation below from https://codereview.stackexchange.com/questions/44546/very-basic-tuple-implementation, question part
//

#define _C ,
#define ESCAPE(...) __VA_ARGS__
#define VARS(...) Tuple<__VA_ARGS__>
#define DEFINE_TASK(FNAME, T_RET, TUPLE_VAR, ...) \
typedef AsyncTaskSpecific<T_RET, TUPLE_VAR, ##__VA_ARGS__> FNAME ## _Task; \
typedef StateParam<T_RET, TUPLE_VAR> FNAME ## _Param

#define DEFINE_CLASS_TASK(CNAME, FNAME, T_RET, TUPLE_VAR, ...) \
typedef AsyncClassTaskSpecific<CNAME, T_RET, TUPLE_VAR, ##__VA_ARGS__> FNAME ## _Task; \
typedef StateParam<T_RET, TUPLE_VAR> FNAME ## _Param

#define ASYNC_FUNC(FNAME, ...) \
static bool FNAME(FNAME ## _Param &state, ##__VA_ARGS__)

#define ASYNC_CLASS_FUNC(CNAME, FNAME, ...) \
bool FNAME(FNAME ## _Param &state, ##__VA_ARGS__)

#define CREATE_TASK(FNAME, ...) FNAME ## _Task(FNAME, ##__VA_ARGS__);

#define CREATE_ASSIGN_TASK(VNAME, FNAME, ...) VNAME = FNAME ## _Task(FNAME, ##__VA_ARGS__);

#define CREATE_CLASS_TASK(CNAME, CVALUE, FNAME, ...) FNAME ## _Task(&CNAME::FNAME, CVALUE, ##__VA_ARGS__);

#define CREATE_ASSIGN_CLASS_TASK(VNAME, CNAME, CVALUE, FNAME, ...) VNAME = FNAME ## _Task(FNAME, CVALUE, ##__VA_ARGS__);

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

#define ASYNC_VAR(NUM, NAME) auto &NAME = Get<NUM>(*state._variables);
#define ASYNC_VAR_INIT(NUM, NAME, INIT) auto &NAME = Get<NUM>(*state._variables); \
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

	void setIsCompleted(bool val)
	{
		_isCompleted = val;
	}

	char *getResultVal()
	{
		return _resultVal;
	}

	void setResultVal(char* val)
	{
		for (int i = 0; i < sizeof(T); i++)
		{
			_resultVal[i] = val[i];
		}
	}

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

	void setIsCompleted(bool val)
	{
		_isCompleted = val;
	}

	char *getResultVal()
	{
		return _resultVal;
	}

	void setResultVal(char* val)
	{
		_resultVal[0] = val[0];
	}

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

template<class TReturn, class TupleVar>
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

template<class TReturn, class TupleVar, class ...TParams>
class AsyncTaskSpecific : public AsyncTask<TReturn>
{
private:
	Tuple<TParams...> _parameters;
	bool(*_func)(StateParam<TReturn, TupleVar>&, TParams...);
	TupleVar _variables;
	int _curLine = 0;

	template<int ...> struct seq {};

	template<int N, int ...S> struct gens : gens<N - 1, N - 1, S...> {};

	template<int ...S> struct gens<0, S...> { typedef seq<S...> type; };

protected:
	bool work()
	{
		setIsCompleted(callFunc(typename gens<sizeof...(TParams)>::type()));
		return completed();
	}

	template<int ...S>
	bool callFunc(seq<S...>)
	{
		StateParam<TReturn, TupleVar> sp = StateParam<TReturn, TupleVar>(&_variables, &_curLine, (TReturn*)getResultVal());
		return _func(sp, Get<S>(_parameters) ...);
	}
public:
	AsyncTaskSpecific(bool(*ptr)(StateParam<TReturn, TupleVar>&, TParams...), TParams... params)
	{
		_func = ptr;
		Set<0, TParams...>(_parameters, params...);
		//_parameters.setItems(params...);
		//_parameters = std::make_tuple(params...);
	}

	AsyncTaskSpecific(AsyncTaskSpecific<void, TReturn, TupleVar, TParams...> &copy)
	{
		_func = copy._func;
		copy_tuple_impl(copy._parameters, _parameters);
		copy_tuple_impl(copy._variables, _variables);
		setIsCompleted(copy.completed());
		setResultVal(copy.getResultVal());
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
private:
	Tuple<TParams...> _parameters;
	bool (TCls::*_func)(StateParam<TReturn, TupleVar>&, TParams...);
	TCls *_funcLocation;
	TupleVar _variables;
	int _curLine = 0;

	template<int ...> struct seq {};

	template<int N, int ...S> struct gens : gens<N - 1, N - 1, S...> {};

	template<int ...S> struct gens<0, S...> { typedef seq<S...> type; };

protected:
	bool work()
	{
		setIsCompleted(callFunc(typename gens<sizeof...(TParams)>::type()));
		return completed();
	}

	template<int ...S>
	bool callFunc(seq<S...>)
	{
		StateParam<TReturn, TupleVar> sp = StateParam<TReturn, TupleVar>(&_variables, &_curLine, (TReturn*)getResultVal());
		return (_funcLocation->*_func)(sp, Get<S>(_parameters) ...);
	}
public:
	AsyncClassTaskSpecific(bool(TCls::*ptr)(StateParam<TReturn, TupleVar>&, TParams...), TCls *funcLocation, TParams... params)
	{
		_func = ptr;
		_funcLocation = funcLocation;
		Set<0, TParams...>(_parameters, params...);
	}

	AsyncClassTaskSpecific(AsyncTaskSpecific<TCls, TReturn, TupleVar, TParams...> &copy)
	{
		_func = copy._func;
		_funcLocation = copy._funcLocation;

		Copy(_parameters, copy._parameters);
		Copy(_variables, copy._variables);

		setIsCompleted(copy.completed());
		setResultVal(copy.getResultVal());
		_curLine = copy._curLine;
	}

	AsyncClassTaskSpecific()
	{
		_func = nullptr;
	}
};