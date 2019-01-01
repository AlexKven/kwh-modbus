#pragma once
#include <vector>

using namespace std;
class PointerTracker
{
private:
	vector<void*> pointerVector;
	vector<void(*)(void*)> deleterVector;

	template<typename T>
	static void del(void *ptr)
	{
		auto typed = (T*)ptr;
		delete ptr;
	}

	template<typename T>
	static void delArray(void *ptr)
	{
		auto typed = (T*)ptr;
		delete[] ptr;
	}
public:
	template<typename T>
	void addPointer(T* ptr)
	{
		pointerVector.push_back(ptr);
		deleterVector.push_back(del<T>);
	}

	template<typename T>
	void addArray(T* ptr)
	{
		pointerVector.push_back(ptr);
		deleterVector.push_back(delArray<T>);
	}
	PointerTracker() {}
	~PointerTracker()
	{
		auto ptrIt = pointerVector.begin();
		auto delIt = deleterVector.begin();
		while (ptrIt != pointerVector.end())
		{
			// Beautiful!
			(*delIt)(*ptrIt);

			++ptrIt;
			++delIt;
		}
	}
};

