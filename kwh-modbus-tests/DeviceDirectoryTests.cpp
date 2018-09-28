#include "pch.h"
#include "fakeit.hpp"

#include "../kwh-modbus/libraries/deviceDirectory/DeviceDirectory.hpp"
#include "test_helpers.h"
#include <tuple>
#define getMock() Mock<DeviceDirectory<byte*>>(*deviceDirectory)

#define C_START int COUNTER_OFFSET = __COUNTER__
#define C_CURRENT __COUNTER__ - COUNTER_OFFSET

#define _C ,
#define TUPLE(...) std::tuple<__VA_ARGS__>
#define DEFINE_TASK(FNAME, T_RET, TUPLE_VAR, ...) \
typedef AsyncTaskSpecific<T_RET, TUPLE_VAR, __VA_ARGS__> FNAME ## _Task

#define ASYNC_FUNC(FNAME, ...) \
static bool FNAME(FNAME ## _Task::StateParam &state, __VA_ARGS__)

#define ASYNC_VAR(NUM, NAME) auto &NAME = std::get<NUM>;

#define START_ASYNC \
switch (__resume_line__) \
{

#define YIELD_ASYNC case __LINE__:

#define END_ASYNC }

using namespace fakeit;

template<class T>
class AsyncTask
{
protected:
	T *_result = nullptr;
	virtual bool work() = 0;

public:
	bool operator()()
	{
		return work();
	}
	
	bool completed()
	{
		return _result != nullptr;
	}

	T& result()
	{
		std::tuple<int, int> t;
		auto &num = std::get<1>(t);
		return *_result;
	}
};

template<class TReturn, class TupleVar, class ...TParams>
class AsyncTaskSpecific : public AsyncTask<TReturn>
{
public:
	struct StateParam
	{
	public:
		StateParam(TupleVar *vars)
		{
			_variables = vars;
		}
		TupleVar *_variables;
	};
private:
	std::tuple<TParams...> _parameters;
	bool(*_func)(StateParam&, TParams...);
	TupleVar _variables;
	int _curLine;

	template<int ...> struct seq {};

	template<int N, int ...S> struct gens : gens<N - 1, N - 1, S...> {};

	template<int ...S> struct gens<0, S...> { typedef seq<S...> type; };

protected:
	bool work()
	{
		return callFunc(typename gens<sizeof...(TParams)>::type());
	}

	template<int ...S>
	bool callFunc(seq<S...>)
	{
		StateParam sp = StateParam(&_variables);
		return _func(sp, std::get<S>(_parameters) ...);
	}
public:

	AsyncTaskSpecific(bool(*ptr)(StateParam&, TParams...), TParams... params)
	{
		_func = ptr;
		_parameters = std::make_tuple(params...);
	}
};

typedef int blah;


class DeviceDirectoryTests : public ::testing::Test
{
protected:
	word * registerArray;
	DeviceDirectory<byte*> *deviceDirectory = new DeviceDirectory<byte*>();

	inline void setupCompareName_ReturnTrueOn(Mock<DeviceDirectory<byte*>> &mock, word index)
	{
		for (int i = 0; i < index; i++)
		{
			When(Method(mock, compareName).Using(i, Any<byte*>())).AlwaysReturn(false);
		}
		When(Method(mock, compareName).Using(index, Any<byte*>())).AlwaysReturn(true);
	}
public:
	void SetUp()
	{
		int two = asyncTest(5);
	}

	void TearDown()
	{
		delete deviceDirectory;
	}

	bool test(int p1, int p2)
	{
		int v1 = 0;
		int v2 = 1;
		return false;
	}

	DEFINE_TASK(asyncFunc, char*, TUPLE(byte, word), word, int*);
	ASYNC_FUNC(asyncFunc, word p1, int* p2)
	{
		return true;
	}

	int asyncTest(int __resume_line__)
	{
		asyncFunc_Task tsk(asyncFunc, 3, new int());
		
		START_ASYNC;
		YIELD_ASYNC;
		return 1;
		YIELD_ASYNC;
		return 2;
		YIELD_ASYNC;
		return 3;
		END_ASYNC;
	}
};

TEST_F(DeviceDirectoryTests, getDeviceName)
{
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceNames = new byte[100];
	for (int i = 0; i < 100; i++)
	{
		deviceDirectory->_deviceNames[i] = i;
	}

	assertArrayEq<byte, byte, byte, byte, byte>
		(deviceDirectory->getDeviceName(0),
			0, 1, 2, 3, 4);
	assertArrayEq<byte, byte, byte, byte, byte>
		(deviceDirectory->getDeviceName(3),
			15, 16, 17, 18, 19);
	assertArrayEq<byte, byte, byte, byte, byte>
		(deviceDirectory->getDeviceName(12),
			60, 61, 62, 63, 64);
}

TEST_F(DeviceDirectoryTests, getNameChar)
{
	deviceDirectory->_deviceNameLength = 7;
	deviceDirectory->_deviceNames = new byte[100];
	for (int i = 0; i < 100; i++)
	{
		deviceDirectory->_deviceNames[i] = i;
	}

	ASSERT_EQ(deviceDirectory->getNameChar(1, 2), 9);
	ASSERT_EQ(deviceDirectory->getNameChar(1, 6), 13);
	ASSERT_EQ(deviceDirectory->getNameChar(4, 0), 28);
	ASSERT_EQ(deviceDirectory->getNameChar(7, 5), 54);
	ASSERT_EQ(deviceDirectory->getNameChar(13, 3), 94);
}

TEST_F(DeviceDirectoryTests, compareName)
{
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceNames = (byte*)"dev00dev01dev02dev03dev04dev05dev06oth00oth01oth02oth03";

	ASSERT_TRUE(deviceDirectory->compareName(0, (byte*)"dev00"));
	ASSERT_TRUE(deviceDirectory->compareName(0, (byte*)"dev00dev01"));
	// Doesn't check past device name length
	ASSERT_TRUE(deviceDirectory->compareName(0, (byte*)"dev00hijklmnop"));
	ASSERT_TRUE(deviceDirectory->compareName(5, (byte*)"dev05"));
	ASSERT_TRUE(deviceDirectory->compareName(8, (byte*)"oth01"));


	ASSERT_FALSE(deviceDirectory->compareName(0, (byte*)"dev01"));
	ASSERT_FALSE(deviceDirectory->compareName(0, (byte*)"dve00dev01"));
	// Doesn't check past device name length
	ASSERT_FALSE(deviceDirectory->compareName(0, (byte*)"dev0ghijklmnop"));
	ASSERT_FALSE(deviceDirectory->compareName(5, (byte*)"deb05"));
	ASSERT_FALSE(deviceDirectory->compareName(8, (byte*)"oth0"));

	// This test only. Since this test uses a C-style string and
	// directly assigns it to _deviceNames, that means it's deleted
	// after this scope ends, so I need to set it to nullptr,
	// otherwise the destructor will try to delete it and will
	// cause a segfault.
	deviceDirectory->_deviceNames = nullptr;
}

TEST_F(DeviceDirectoryTests, init_default)
{
	deviceDirectory->init(11, 5);
	for (int i = 0; i < 55; i++)
	{
		deviceDirectory->_deviceNames[i] = i;
	}
	for (int i = 0; i < 5; i++)
	{
		deviceDirectory->_deviceTypes[i] = i;
		deviceDirectory->_slaveIds[i] = i;
	}

	ASSERT_EQ(deviceDirectory->_maxDevices, 5);
	ASSERT_EQ(deviceDirectory->_deviceNameLength, 11);

	ASSERT_EQ(deviceDirectory->_deviceNames[0], 0);
	ASSERT_EQ(deviceDirectory->_deviceNames[54], 54);
	ASSERT_EQ(deviceDirectory->_deviceTypes[0], 0);
	ASSERT_EQ(deviceDirectory->_deviceTypes[4], 4);
	ASSERT_EQ(deviceDirectory->_slaveIds[0], 0);
	ASSERT_EQ(deviceDirectory->_slaveIds[4], 4);
	ASSERT_EQ(deviceDirectory->_persistentStore, nullptr);
}

TEST_F(DeviceDirectoryTests, init_persistentStore)
{
	byte *persistentStore = new byte();

	deviceDirectory->init(11, 5, &persistentStore);
	for (int i = 0; i < 55; i++)
	{
		deviceDirectory->_deviceNames[i] = i;
	}

	ASSERT_EQ(deviceDirectory->_maxDevices, 5);
	ASSERT_EQ(deviceDirectory->_deviceNameLength, 11);

	ASSERT_EQ(deviceDirectory->_deviceNames[0], 0);
	ASSERT_EQ(deviceDirectory->_deviceNames[54], 54);
	ASSERT_EQ(deviceDirectory->_deviceTypes[0], 0);
	ASSERT_EQ(deviceDirectory->_deviceTypes[4], 0);
	ASSERT_EQ(deviceDirectory->_slaveIds[0], 0);
	ASSERT_EQ(deviceDirectory->_slaveIds[4], 0);
	ASSERT_EQ(deviceDirectory->_persistentStore, &persistentStore);
}

TEST_F(DeviceDirectoryTests, init_maxMemory)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	Fake(OverloadedMethod(mock, init, void(word, word, byte**)));

	word numDevices;
	deviceDirectory->init(500, 11, numDevices);

	ASSERT_EQ(numDevices, 35);
	Verify(OverloadedMethod(mock, init, void(word, word, byte**)).Using(11, 35, nullptr)).Once();
}

TEST_F(DeviceDirectoryTests, findDeviceForName_empty)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	setupCompareName_ReturnTrueOn(mock, 3);
	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceTypes = new word(0);
	deviceDirectory->_slaveIds = new byte(0);

	word devType;
	byte slaveId;
	int row;
	bool found = deviceDirectory->findDeviceForName((byte*)"fiver", devType, slaveId, row);

	ASSERT_FALSE(found);
	ASSERT_EQ(slaveId, 0);
	ASSERT_EQ(devType, 0);
	ASSERT_EQ(row, -1);
	Verify(Method(mock, compareName).Using(Any<word>(), Any<byte*>())).Never();
}

TEST_F(DeviceDirectoryTests, findDeviceForName_emptyish)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	setupCompareName_ReturnTrueOn(mock, 3);
	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceTypes = new word[2]{ 1, 0 };
	deviceDirectory->_slaveIds = new byte[2]{ 0, 0 };

	word devType;
	byte slaveId;
	int row;
	bool found = deviceDirectory->findDeviceForName((byte*)"fiver", devType, slaveId, row);

	ASSERT_FALSE(found);
	ASSERT_EQ(slaveId, 0);
	ASSERT_EQ(devType, 0);
	ASSERT_EQ(row, -1);
	Verify(Method(mock, compareName).Using(Any<word>(), Any<byte*>())).Never();
}

TEST_F(DeviceDirectoryTests, findDeviceForName_exhaustedList)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	setupCompareName_ReturnTrueOn(mock, 6);
	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceTypes = new word[6]{ 1, 4, 7, 10, 13, 16 };
	deviceDirectory->_slaveIds = new byte[6]{ 0, 3, 6, 9, 12, 15 };

	word devType;
	byte slaveId;
	int row;
	bool found = deviceDirectory->findDeviceForName((byte*)"fiver", devType, slaveId, row);

	ASSERT_FALSE(found);
	ASSERT_EQ(slaveId, 0);
	ASSERT_EQ(devType, 0);
	ASSERT_EQ(row, -1);
	Verify(Method(mock, compareName).Using(0, Any<byte*>())).Never();
	Verify(Method(mock, compareName).Using(1, Any<byte*>())).Once();
	Verify(Method(mock, compareName).Using(5, Any<byte*>())).Once();
}

TEST_F(DeviceDirectoryTests, findDeviceForName_found)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	setupCompareName_ReturnTrueOn(mock, 3);
	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceTypes = new word[6] { 1, 4, 7, 10, 13, 16 };
	deviceDirectory->_slaveIds = new byte[6] { 0, 3, 6, 9, 12, 15 };

	word devType;
	byte slaveId;
	int row;
	bool found = deviceDirectory->findDeviceForName((byte*)"fiver", devType, slaveId, row);

	ASSERT_TRUE(found);
	ASSERT_EQ(slaveId, 9);
	ASSERT_EQ(devType, 10);
	ASSERT_EQ(row, 3);
	Verify(Method(mock, compareName).Using(0, Any<byte*>())).Never();
	Verify(Method(mock, compareName).Using(1, Any<byte*>())).Once();
	Verify(Method(mock, compareName).Using(3, Any<byte*>())).Once();
	Verify(Method(mock, compareName).Using(5, Any<byte*>())).Never();
}

TEST_F(DeviceDirectoryTests, insertIntoRow)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceTypes = new word[6];
	deviceDirectory->_slaveIds = new byte[6];

	byte *name = new byte[5];
	When(Method(mock, getDeviceName).Using((word)3)).Return(name);

	deviceDirectory->insertIntoRow(3, (byte*)"Aleah", 29, 31);

	ASSERT_EQ(deviceDirectory->_deviceTypes[3], 29);
	ASSERT_EQ(deviceDirectory->_slaveIds[3], 31);
	assertArrayEq<byte, byte, byte, byte, byte>(name,
		'A', 'l', 'e', 'a', 'h');
}

TEST_F(DeviceDirectoryTests, updateItemInDeviceDirectory_notFound)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();

	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceTypes = new word[6];
	deviceDirectory->_slaveIds = new byte[6];

	When(Method(mock, findDeviceForName)).Do([](byte* name, word &type, byte &slave, int &row)
	{
		row = -1;
		return false;
	});
	Fake(Method(mock, insertIntoRow));

	bool success = deviceDirectory->updateItemInDeviceDirectory((byte*)"Aleah", 29, 31);

	ASSERT_FALSE(success);
	Verify(Method(mock, insertIntoRow)).Never();
}

TEST_F(DeviceDirectoryTests, updateItemInDeviceDirectory_foundButSame)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();

	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceTypes = new word[6];
	deviceDirectory->_slaveIds = new byte[6];

	When(Method(mock, findDeviceForName)).Do([](byte* name, word &type, byte &slave, int &row)
	{
		row = 5;
		type = 29;
		slave = 31;
		return true;
	});
	Fake(Method(mock, insertIntoRow));

	bool success = deviceDirectory->updateItemInDeviceDirectory((byte*)"Aleah", 29, 31);

	ASSERT_FALSE(success);
	Verify(Method(mock, insertIntoRow)).Never();
}

TEST_F(DeviceDirectoryTests, updateItemInDeviceDirectory_foundAndChanged)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();

	deviceDirectory->_maxDevices = 6;
	deviceDirectory->_deviceNameLength = 5;
	deviceDirectory->_deviceTypes = new word[6];
	deviceDirectory->_slaveIds = new byte[6];

	When(Method(mock, findDeviceForName)).Do([](byte* name, word &type, byte &slave, int &row)
	{
		row = 5;
		type = 29;
		slave = 37;
		return true;
	});
	Fake(Method(mock, insertIntoRow));

	bool success = deviceDirectory->updateItemInDeviceDirectory((byte*)"Aleah", 29, 31);

	ASSERT_TRUE(success);
	Verify(Method(mock, insertIntoRow).Using(5, Any<byte*>(), 29, 31)).Once();
}

TEST_F(DeviceDirectoryTests, clearDeviceDirectoryRow_topDevice)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_slaveIds = new byte[7]{ 1, 2, 3, 4, 5, 0, 0 };
	deviceDirectory->_deviceTypes = new word[7]{ 1, 2, 1, 4, 1, 0, 0 };

	deviceDirectory->clearDeviceDirectoryRow(4);

	assertArrayEq<byte, byte, byte, byte, byte, byte, byte>
		(deviceDirectory->_slaveIds,
			1, 2, 3, 4, 0, 0, 0);
	assertArrayEq<word, word, word, word, word, word, word>
		(deviceDirectory->_deviceTypes,
			1, 2, 1, 4, 0, 0, 0);
}

TEST_F(DeviceDirectoryTests, clearDeviceDirectoryRow_penultimateDevice)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_slaveIds = new byte[7]{ 1, 2, 3, 4, 5, 0, 0 };
	deviceDirectory->_deviceTypes = new word[7]{ 1, 2, 1, 4, 1, 0, 0 };

	deviceDirectory->clearDeviceDirectoryRow(3);

	assertArrayEq<byte, byte, byte, byte, byte, byte, byte>
		(deviceDirectory->_slaveIds,
			1, 2, 3, 0, 5, 0, 0);
	assertArrayEq<word, word, word, word, word, word, word>
		(deviceDirectory->_deviceTypes,
			1, 2, 1, 1, 1, 0, 0);
}

TEST_F(DeviceDirectoryTests, clearDeviceDirectoryRow_penultimateDeviceEmptyBelow)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_slaveIds = new byte[7]{ 1, 0, 0, 4, 5, 0, 0 };
	deviceDirectory->_deviceTypes = new word[7]{ 1, 1, 1, 4, 1, 0, 0 };

	deviceDirectory->clearDeviceDirectoryRow(3);

	assertArrayEq<byte, byte, byte, byte, byte, byte, byte>
		(deviceDirectory->_slaveIds,
			1, 0, 0, 0, 5, 0, 0);
	assertArrayEq<word, word, word, word, word, word, word>
		(deviceDirectory->_deviceTypes,
			1, 1, 1, 1, 1, 0, 0);
}

TEST_F(DeviceDirectoryTests, clearDeviceDirectoryRow_topDeviceEmptyBelow)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_slaveIds = new byte[7]{ 1, 0, 0, 0, 5, 0, 0 };
	deviceDirectory->_deviceTypes = new word[7]{ 1, 1, 1, 1, 1, 0, 0 };

	deviceDirectory->clearDeviceDirectoryRow(4);

	assertArrayEq<byte, byte, byte, byte, byte, byte, byte>
		(deviceDirectory->_slaveIds,
			1, 0, 0, 0, 0, 0, 0);
	assertArrayEq<word, word, word, word, word, word, word>
		(deviceDirectory->_deviceTypes,
			1, 0, 0, 0, 0, 0, 0);
}

TEST_F(DeviceDirectoryTests, findFreeRow_empty)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_slaveIds = new byte[7]{ 0, 0, 0, 0, 0, 0, 0 };
	deviceDirectory->_deviceTypes = new word[7]{ 0, 0, 0, 0, 0, 0, 0 };

	int result = deviceDirectory->findFreeRow();

	ASSERT_EQ(result, 0);
}

TEST_F(DeviceDirectoryTests, findFreeRow_beginning)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_slaveIds = new byte[7]{ 0, 0, 0, 0, 3, 0, 0 };
	deviceDirectory->_deviceTypes = new word[7]{ 1, 1, 1, 1, 3, 0, 0 };

	int result = deviceDirectory->findFreeRow();

	ASSERT_EQ(result, 0);
}

TEST_F(DeviceDirectoryTests, findFreeRow_middle)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_slaveIds = new byte[7]{ 2, 3, 4, 0, 0, 0, 0 };
	deviceDirectory->_deviceTypes = new word[7]{ 1, 1, 1, 0, 0, 0, 0 };

	int result = deviceDirectory->findFreeRow();

	ASSERT_EQ(result, 3);
}

TEST_F(DeviceDirectoryTests, findFreeRow_middleWithOthersAfter)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_slaveIds = new byte[7]{ 2, 3, 4, 0, 0, 5, 0 };
	deviceDirectory->_deviceTypes = new word[7]{ 1, 1, 1, 1, 1, 3, 0 };

	int result = deviceDirectory->findFreeRow();

	ASSERT_EQ(result, 3);
}

TEST_F(DeviceDirectoryTests, findFreeRow_last)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_slaveIds = new byte[7]{ 2, 3, 4, 5, 6, 7, 0 };
	deviceDirectory->_deviceTypes = new word[7]{ 1, 1, 2, 3, 5, 8, 0 };

	int result = deviceDirectory->findFreeRow();

	ASSERT_EQ(result, 6);
}

TEST_F(DeviceDirectoryTests, findFreeRow_full)
{
	deviceDirectory->_maxDevices = 7;
	deviceDirectory->_slaveIds = new byte[7]{ 2, 3, 4, 5, 6, 7, 8 };
	deviceDirectory->_deviceTypes = new word[7]{ 1, 1, 2, 3, 5, 8, 13 };

	int result = deviceDirectory->findFreeRow();

	ASSERT_EQ(result, -1);
}

TEST_F(DeviceDirectoryTests, findFreeSlaveID_empty)
{
	deviceDirectory->_maxDevices = 10;
	deviceDirectory->_slaveIds = new byte[10]{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	deviceDirectory->_deviceTypes = new word[10]{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	byte freeSlaveId = deviceDirectory->findFreeSlaveID();

	ASSERT_EQ(freeSlaveId, 1);
}

TEST_F(DeviceDirectoryTests, findFreeSlaveID_full)
{
	deviceDirectory->_maxDevices = 246;
	deviceDirectory->_slaveIds = new byte[246];
	for (int i = 0; i < 246; i++)
	{
		deviceDirectory->_slaveIds[i] = i + 1;
	}

	byte freeSlaveId = deviceDirectory->findFreeSlaveID();

	ASSERT_EQ(freeSlaveId, 0);
}

TEST_F(DeviceDirectoryTests, findFreeSlaveID_partiallyFilled_case1)
{
	deviceDirectory->_maxDevices = 10;
	deviceDirectory->_slaveIds = new byte[10]{ 1, 1, 2, 3, 0, 0, 0, 0, 0, 0 };
	deviceDirectory->_deviceTypes = new word[10]{ 1, 2, 3, 2, 0, 0, 0, 0, 0, 0 };

	byte freeSlaveId = deviceDirectory->findFreeSlaveID();

	ASSERT_EQ(freeSlaveId, 4);
}

TEST_F(DeviceDirectoryTests, findFreeSlaveID_partiallyFilled_case2)
{
	deviceDirectory->_maxDevices = 10;
	deviceDirectory->_slaveIds = new byte[10]{ 1, 0, 2, 3, 0, 0, 0, 0, 0, 0 };
	deviceDirectory->_deviceTypes = new word[10]{ 1, 1, 3, 2, 0, 0, 0, 0, 0, 0 };

	byte freeSlaveId = deviceDirectory->findFreeSlaveID();

	ASSERT_EQ(freeSlaveId, 4);
}

TEST_F(DeviceDirectoryTests, findFreeSlaveID_partiallyFilled_case3)
{
	deviceDirectory->_maxDevices = 10;
	deviceDirectory->_slaveIds = new byte[10]{ 1, 1, 4, 3, 0, 0, 0, 0, 0, 0 };
	deviceDirectory->_deviceTypes = new word[10]{ 1, 2, 3, 2, 0, 0, 0, 0, 0, 0 };

	byte freeSlaveId = deviceDirectory->findFreeSlaveID();

	ASSERT_EQ(freeSlaveId, 2);
}

TEST_F(DeviceDirectoryTests, findFreeSlaveID_partiallyFilled_case4)
{
	deviceDirectory->_maxDevices = 10;
	deviceDirectory->_slaveIds = new byte[10]{ 0, 1, 4, 3, 0, 0, 0, 0, 0, 0 };
	deviceDirectory->_deviceTypes = new word[10]{ 1, 2, 3, 2, 0, 0, 0, 0, 0, 0 };

	byte freeSlaveId = deviceDirectory->findFreeSlaveID();

	ASSERT_EQ(freeSlaveId, 2);
}

TEST_F(DeviceDirectoryTests, addDevice_success)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	When(Method(mock, findFreeRow)).Return(6);
	Fake(Method(mock, insertIntoRow));

	byte* name = (byte*)"Aleah";
	int row = deviceDirectory->addDevice(name, 5, 1);

	ASSERT_EQ(row, 6);
	Verify(Method(mock, insertIntoRow).Using(6, name, 5, 1)).Once();
}

TEST_F(DeviceDirectoryTests, addDevice_failure)
{
	Mock<DeviceDirectory<byte*>> mock = getMock();
	When(Method(mock, findFreeRow)).Return(-1);
	Fake(Method(mock, insertIntoRow));

	byte* name = (byte*)"Aleah";
	int row = deviceDirectory->addDevice(name, 5, 1);

	ASSERT_EQ(row, -1);
	Verify(Method(mock, insertIntoRow)).Never();
}

TEST_F(DeviceDirectoryTests, filterDevicesForSlave_empty)
{
	deviceDirectory->_maxDevices = 10;
	deviceDirectory->_slaveIds = new byte[10]{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	deviceDirectory->_deviceTypes = new word[10]{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	int numDeleted = deviceDirectory->filterDevicesForSlave(
		new byte*[3]{ (byte*)"Aleah", (byte*)"Chloe", (byte*)"Asher" },
		3, 4);

	ASSERT_EQ(numDeleted, 0);
}
//
//TEST_F(DeviceDirectoryTests, filterDevicesForSlave_empty)
//{
//	Mock<DeviceDirectory<byte*>> mock = getMock();
//	deviceDirectory->_maxDevices = 10;
//	deviceDirectory->_slaveIds = new byte[10]{ 1, 1, 2, 2, 2, 0, 0, 0, 0, 0 };
//	deviceDirectory->_deviceTypes = new word[10]{ 2, 3, 2, 3, 4, 0, 0, 0, 0, 0 };
//
//	byte freeSlaveId = deviceDirectory->findFreeSlaveID();
//
//	ASSERT_EQ(freeSlaveId, 1);
//}