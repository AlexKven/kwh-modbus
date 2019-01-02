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
	T* addPointer(T* ptr)
	{
		pointerVector.push_back(ptr);
		deleterVector.push_back(del<T>);
		return ptr;
	}

	template<typename T>
	T* addArray(T* ptr)
	{
		pointerVector.push_back(ptr);
		deleterVector.push_back(delArray<T>);
		return ptr;
	}

	template<class A>
	void addPointers(A* head)
	{
		addPointer(head);
	}

	template<class A, class ...B>
	void addPointers(A* head, B*... args)
	{
		addPointer(head);
		addPointers(args...);
	}

	template<class A>
	void addArrays(A* head)
	{
		addArray(head);
	}

	template<class A, class ...B>
	void addArrays(A* head, B*... args)
	{
		addArray(head);
		addArrays(args...);
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

