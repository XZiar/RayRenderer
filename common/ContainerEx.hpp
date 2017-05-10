#pragma once

#include <cstdint>
#include <optional>
#include <tuple>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>


namespace common
{


namespace detail
{

template<class T,class Ele>
struct EleTyper
{
	using type = typename Ele;
};
template<class T, class Ele>
struct EleTyper<const T, Ele>
{
	using type = typename const Ele;
};

}

template<class Map, class Key = typename Map::key_type, class Val = typename detail::EleTyper<Map, Map::mapped_type>::type>
inline std::optional<Val*> findmap(Map& themap, const Key& key)
{
	const auto it = themap.find(key);
	if (it == themap.end())//not exist
		return {};
	return &it->second;
}

template<class Vec, class Predictor, class Val = typename detail::EleTyper<Vec, Vec::value_type>::type>
inline std::optional<Val*> findvec(Vec& thevec, const Predictor& pred)
{
	auto it = std::find_if(thevec.begin(), thevec.end(), pred);
	if (it == thevec.end())//not exist
		return {};
	return &(*it);
}

namespace detail
{
template<class... ARGS>
class ZIPContainer
{
private:
	static constexpr auto tsize = std::tuple_size<std::tuple<ARGS&&...>>::value;
	const std::tuple<ARGS&&...> srcs;
	template<size_t index>
	size_t getSize(const size_t cursize = SIZE_MAX) const
	{
		auto newsize = std::min(cursize, std::get<index>(srcs).size());
		return getSize<index - 1>(newsize);
	}
	template<>
	size_t getSize<0>(const size_t cursize) const
	{
		return std::min(cursize, std::get<0>(srcs).size());
	}
	template<size_t index>
	auto getBegin() const
	{
		auto cur = std::make_tuple(std::get<index>(srcs).begin());
		return std::tuple_cat(getBegin<index - 1>(), cur);
	}
	template<>
	auto getBegin<0>() const
	{
		auto cur = std::make_tuple(std::get<0>(srcs).begin());
		return cur;
	}
	void inc_helper()
	{}
	template<class T, class... ARGS2>
	void inc_helper(T& arg, ARGS2&&... args)
	{
		++arg;
		inc_helper(args...);
	}
	template<class T, std::size_t... indexes>
	void incer(T& iterators, std::index_sequence<indexes...>)
	{
		inc_helper(std::get<indexes>(iterators)...);
	}
	template<class T>
	void inc(T& iterators)
	{
		incer(iterators, std::make_index_sequence<tsize>{});
	}
public:
	ZIPContainer(ARGS&&... args) : srcs(std::forward_as_tuple(args...)) {}
	size_t size() const
	{
		return getSize<tsize - 1>();
	}
	template<class T>
	void foreach(const T& func)
	{
		auto begins = getBegin<tsize - 1>();
		for (auto a = size(); a--;)
		{
			std::apply(func, begins);
			inc(begins);
		}
	}
};
}

template<class... ARGS>
inline constexpr detail::ZIPContainer<ARGS...> zip(ARGS&&... args)
{
	return detail::ZIPContainer<ARGS...>(std::forward<ARGS>(args)...);
}
}