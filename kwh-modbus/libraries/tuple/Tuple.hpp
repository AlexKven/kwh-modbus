
// Arduino-friendly tuple implementation based on this StackOverflow answer:
// https://stackoverflow.com/a/52208842/6706737

template<size_t i, class Item>
struct TupleLeaf {
    Item value;
};

// TupleImpl is a proxy for the final class that has an extra 
// template parameter `i`.
template<size_t i, class... Items>
struct TupleImpl;

// Base case: empty tuple
template<size_t i>
struct TupleImpl<i>{};

// Recursive specialization
template<size_t i, class HeadItem, class... TailItems>
struct TupleImpl<i, HeadItem, TailItems...> :
    public TupleLeaf<i, HeadItem>, // This adds a `value` member of type HeadItem
    public TupleImpl<i + 1, TailItems...> // This recurses
{
};

// Obtain a reference to i-th item in a tuple
template<size_t i, class HeadItem, class... TailItems>
HeadItem& Get(TupleImpl<i, HeadItem, TailItems...>& tuple) {
    // Fully qualified name for the member, to find the right one 
    // (they are all called `value`).
    return tuple.TupleLeaf<i, HeadItem>::value;
}

// Templated alias to avoid having to specify `i = 0`
template<class... Items>
using Tuple = TupleImpl<0, Items...>;

template<size_t i, class Head, class... Tail>
void Set(TupleImpl<i, Head, Tail...>& tuple, Head head, Tail... items)
{
	Get<i>(tuple) = head;
	Set<i + 1, Tail...>(tuple, items...);
}

template<size_t i, class Head>
void Set(TupleImpl<i, Head>& tuple, Head head)
{
	Get<i>(tuple) = head;
}

template<size_t i>
void Set(TupleImpl<i>& tuple)
{
}

// Templated alias to avoid having to specify `i = 0`
template<class... Items>
using Tuple = TupleImpl<0, Items...>;

template<size_t i, class Head, class... Tail>
void Copy(TupleImpl<i, Head, Tail...>& to, TupleImpl<i, Head, Tail...>& from)
{
	Get<i>(to) = Get<i>(from);
	Copy<i + 1, Tail...>(to, from);
}

template<size_t i, class Head>
void Copy(TupleImpl<i, Head>& to, TupleImpl<i, Head>& from)
{
	Get<i>(to) = Get<i>(from);
}

template<size_t i>
void Copy(TupleImpl<i>& to, TupleImpl<i>& from)
{
}