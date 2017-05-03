#pragma once

#include <iterator>
#include <functional>
#include <vector>
#include <optional>

namespace common::linq
{

namespace detail
{


template<class T>
struct ITSource
{
	using ITT = typename T::iterator;
	ITT cur, end;
	auto& current() const
	{
		return *cur;
	}
	bool toNext()
	{
		return ++cur == end;
	}
	bool empty()
	{
		return cur == end;
	}
	ITSource(const ITT& cur_, const ITT& end_) : cur(cur_), end(end_) {}
	template<class = typename std::enable_if<!std::is_same<T, ITSource>::value>::type>
	ITSource(T& container) : cur(container.begin()), end(container.end()) {}
	ITSource() : end(cur) {}
	ITSource(const ITSource& rhs) = default;
};



template<class T>
struct ArraySource
{
	T *cur, *end;
	auto& current()
	{
		return *cur;
	}
	bool toNext()
	{
		return ++cur == end;
	}
	bool empty()
	{
		return cur == end;
	}
	ArraySource(const T *cur_, const T *end_) : cur(cur_), end(end_) {}
};

template<class SRC, typename Selector>
struct SelectSource
{
	SRC source;
	Selector selector;
	auto& current() const
	{
		return selector(source.current());
	}
	bool toNext()
	{
		return source.toNext();
	}
	bool empty()
	{
		return source.toNext();
	}
	SelectSource(const SRC& src, const Selector& func) : source(src), selector(func) {}
};

template<class SRC, class SUBSRC, typename Selector>
struct MultiSelectSource
{
	SRC source;
	SUBSRC subsrc;
	Selector selector;
	auto& current() const
	{
		return subsrc.current();
	}
	bool toNext()
	{
		if (!subsrc.toNext())
		{
			if (!source.toNext())
				return false;
			subsrc = SUBSRC(selector(source.current()));
		}
		return true;
	}
	bool empty()
	{
		return subsrc.empty();
	}
	MultiSelectSource(const SRC& src, const Selector& func) : source(src), selector(func) 
	{
		if (!source.empty())
			subsrc = SUBSRC(selector(source.current()));
	}
};

template<class SRC, typename Predictor>
struct WhereSource
{
	SRC source;
	Predictor filter;
	auto& current() const
	{
		return source.current();
	}
	bool toNext()
	{
		while (source.toNext())
			if (filter(source.current()))
				return true;
		return false;
	}
	bool empty()
	{
		return source.empty();
	}
	WhereSource(const SRC& src, const Predictor& func) : source(src), filter(func)
	{
		while (!filter(source.current()))
			if (!source.toNext())
				break;
	}
};


}



template<class SRC>
struct Linq
{
protected:
	SRC source;
public:
	Linq(SRC src) : source(src) {}
	template<typename Action>
	void foreach(const Action& func)
	{
		while (!source.empty())
		{
			func(source.current());
			source.toNext();
		}
	}
	template<typename Selector>
	auto select(const Selector& func)
	{
		using NEWSRC = detail::SelectSource<SRC, Selector>;
		auto newsrc = NEWSRC(source, func);
		return Linq<NEWSRC>(newsrc);
	}
	template<typename Selector>
	auto selectMany(const Selector& func)
	{
		using T = decltype(func(source.current()));
		using NEWSRC = detail::MultiSelectSource<SRC, detail::ITSource<T>, Selector>;
		auto newsrc = NEWSRC(source, func);
		return Linq<NEWSRC>(newsrc);
	}
	template<typename Predictor>
	auto where(const Predictor& func)
	{
		using NEWSRC = detail::WhereSource<SRC, Predictor>;
		auto newsrc = NEWSRC(source, func);
		return Linq<NEWSRC>(newsrc);
	}
	auto to_vector()
	{
		std::vector<decltype(source.current())> ret;
		for (; !source.empty(); source.toNext())
			ret.push_back(source.current());
		return ret;
	}
	std::optional<decltype(&source.current())> first()
	{
		if (source.empty())
			return {};
		return &source.current();
	}
};






template<class T>
inline auto from(T& obj)
{
	auto src = detail::ITSource<T>(obj);
	return Linq<detail::ITSource<T>>(src);
}

template<class T, size_t N>
inline auto from(T(&obj)[N])
{
	auto src = detail::ArraySource(&obj[0], &obj[N - 1] + 1);
	return Linq<detail::ArraySource<T>>(src);
}

}