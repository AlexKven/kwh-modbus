#pragma once

template<class... Types>
class DiscriminatedUnion
{
public:
	// Thanks to the following SO question for the following code:
	// https://stackoverflow.com/a/36806233/6706737

	template <typename T>
	static constexpr T static_max(T a, T b)
	{
		return a < b ? b : a;
	}

	template <int = 0>
	static constexpr int maxSizeOf()
	{
		return 0u;
	};

	template <typename T, typename ... Ts>
	static constexpr int maxSizeOf()
	{
		return static_max((int)sizeof(T), maxSizeOf<Ts...>());
	};

private:
	char bytes[maxSizeOf<Types...>()];

public:
	template<class T>
	T* Pointer()
	{
		return (T*)(&bytes);
	}

	template<class T>
	T& As()
	{
		return T(*Pointer<T>());
	}
};