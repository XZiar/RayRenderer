#pragma once

#include <optional>
#include <algorithm>
#include <type_traits>


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

using std::optional;

template<class Map, class Key = typename Map::key_type, class Val = typename detail::EleTyper<Map, Map::mapped_type>::type>
optional<Val*> findmap(Map& themap, const Key& key)
{
	const auto it = themap.find(key);
	if (it == themap.end())//not exist
		return {};
	return &it->second;
}

template<class Vec, class Predictor, class Val = typename detail::EleTyper<Vec, Vec::value_type>::type>
optional<Val*> findvec(Vec& thevec, const Predictor& pred)
{
	auto it = std::find_if(thevec.begin(), thevec.end(), pred);
	if (it == thevec.end())//not exist
		return {};
	return &(*it);
}


}