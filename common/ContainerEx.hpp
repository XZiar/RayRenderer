#pragma once

#include "CommonUtil.hpp"
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

template<class Map, class Key = typename Map::key_type, class Val = typename detail::EleTyper<Map, typename Map::mapped_type>::type>
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
	struct Incer
	{
		template<typename T>
		static void Each(T& arg) { ++arg; }
	};
	struct Sizer : public func_with_cookie
	{
		using CookieType = size_t;
		static CookieType Init() { return SIZE_MAX; }
		template<typename T>
		static void Each(CookieType& cookie, T& arg) { cookie = std::min(arg.size(), cookie); }
	};
	struct MapBegin
	{
		template<typename T>
		static auto Map(T& arg) { return arg.begin(); }
	};
	const std::tuple<ARGS&&...> srcs;
public:
	ZIPContainer(ARGS&&... args) : srcs(std::forward_as_tuple(args...)) {}
	size_t size() const
	{
		return ForEach<Sizer>::EachTuple(srcs);
	}
	template<class Func>
	void foreach(const Func& func) const
	{
		auto begins = Mapping<MapBegin>::MapTuple(srcs);
		for (auto a = size(); a--;)
		{
			std::apply(func, begins);
			ForEach<Incer>::EachTuple(begins);
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