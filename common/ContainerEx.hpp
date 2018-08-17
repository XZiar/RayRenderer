#pragma once

#include "CommonUtil.hpp"
#include <cstdint>
#include <optional>
#include <tuple>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>

namespace common::container
{

template<typename Child, typename StringType>
struct COMMONTPL NamedSetValue
{
    bool operator<(const Child& other) const noexcept { return ((const Child*)this)->Name < other.Name; }
};
template<typename Child, typename StringType>
forceinline bool operator<(const NamedSetValue<Child, StringType>& obj, const StringType& name) noexcept { return ((const Child*)&obj)->Name < name; }
template<typename Child, typename StringType>
forceinline bool operator<(const StringType& name, const NamedSetValue<Child, StringType>& obj) noexcept { return name < ((const Child*)&obj)->Name; }

namespace detail
{
struct LessTester
{
    template<typename T>
    auto operator()(const T& left, const T& right) -> decltype(left < right);
};
}

template<typename T, auto Key>
struct SetKeyLess
{
    using is_transparent = void;
    template<typename C>
    constexpr bool operator()(const T& left, const C& right) const noexcept
    {
        return ((left.*Key) < right);
    }
    template<typename C>
    constexpr bool operator()(const C& left, const T& right) const noexcept
    {
        return (left < (right.*Key));
    }
    constexpr bool operator()(const T& left, const T& right) const noexcept
    {
        if constexpr(std::is_invocable<detail::LessTester, T, T>::value)
            return (left < right);
        else
            return ((left.*Key) < (right.*Key));
    }
};

template<typename Key, typename Val>
struct PairLess
{
    using is_transparent = void;
    constexpr bool operator()(const std::pair<Key, Val>& left, const std::pair<Key, Val>& right) const
    {
        return (left < right);
    }
    constexpr bool operator()(const std::pair<Key, Val>& left, const Key& right) const
    {
        return (left.first < right);
    }
    constexpr bool operator()(const Key& left, const std::pair<Key, Val>& right) const
    {
        return (left < right.first);
    }
};

template<typename Key, typename Val>
struct PairGreater
{
    using is_transparent = void;
    constexpr bool operator()(const std::pair<Key, Val>& left, const std::pair<Key, Val>& right) const
    {
        return (left > right);
    }
    constexpr bool operator()(const std::pair<Key, Val>& left, const Key& right) const
    {
        return (left.first > right);
    }
    constexpr bool operator()(const Key& left, const std::pair<Key, Val>& right) const
    {
        return (left > right.first);
    }
};

namespace detail
{

template<typename T, typename Ele>
struct EleTyper
{
    using type = /*typename*/ Ele;
};
template<typename T, typename Ele>
struct EleTyper<const T, Ele>
{
    using type = /*typename*/ const Ele;
};

}

template<class Set, typename Key, typename Val = typename detail::EleTyper<Set, typename Set::value_type>::type>
inline const Val* FindInSet(Set& theset, const Key& key)
{
    const auto it = theset.find(key);
    if (it == theset.end())//not exist
        return nullptr;
    return &(*it);
}

template<class Set, typename Key, typename Val = typename detail::EleTyper<Set, typename Set::value_type>::type>
inline std::optional<Val> FindInSet(Set& theset, const Key& key, const std::in_place_t)
{
    const auto it = theset.find(key);
    if (it == theset.end())//not exist
        return {};
    return *it;
}

template<class Set, typename Key, typename Val = typename detail::EleTyper<Set, typename Set::value_type>::type>
inline Val FindInSetOrDefault(Set& theset, const Key& key, const Val def = Val{})
{
    const auto it = theset.find(key);
    if (it == theset.end())//not exist
        return def;
    return *it;
}

template<class Map, typename Key = typename Map::key_type, typename Val = typename detail::EleTyper<Map, typename Map::mapped_type>::type>
inline Val* FindInMap(Map& themap, const Key& key)
{
    const auto it = themap.find(key);
    if (it == themap.end())//not exist
        return nullptr;
    return &it->second;
}

template<class Map, typename Key = typename Map::key_type, typename Val = typename Map::mapped_type>
inline std::optional<Val> FindInMap(Map& themap, const Key& key, const std::in_place_t)
{
    const auto it = themap.find(key);
    if (it == themap.end())//not exist
        return {};
    return it->second;
}

template<class Map, typename Key = typename Map::key_type, typename Val = typename detail::EleTyper<Map, typename Map::mapped_type>::type>
inline Val FindInMapOrDefault(Map& themap, const Key& key, const Val def = Val{})
{
    const auto it = themap.find(key);
    if (it == themap.end())//not exist
        return def;
    return it->second;
}

template<class Vec, typename Predictor, typename Val = typename detail::EleTyper<Vec, typename Vec::value_type>::type>
inline Val* FindInVec(Vec& thevec, const Predictor& pred)
{
    const auto it = std::find_if(thevec.begin(), thevec.end(), pred);
    if (it == thevec.end())//not exist
        return nullptr;
    return &(*it);
}

template<class Vec, typename Predictor, typename Val = typename Vec::value_type>
inline std::optional<Val> FindInVec(Vec& thevec, const Predictor& pred, const std::in_place_t)
{
    const auto it = std::find_if(thevec.begin(), thevec.end(), pred);
    if (it == thevec.end())//not exist
        return {};
    return *it;
}

template<class Vec>
inline bool ContainInVec(Vec& thevec, const typename Vec::value_type& val)
{
    for (const auto& ele : thevec)
        if (ele == val)
            return true;
    return false;
}

template<class Vec, typename Predictor, typename Val = typename Vec::value_type>
inline Val FindInVecOrDefault(Vec& thevec, const Predictor& pred, const Val def = Val{})
{
    const auto it = std::find_if(thevec.begin(), thevec.end(), pred);
    if (it == thevec.end())//not exist
        return def;
    return *it;
}

template<class Vec, typename Predictor, typename Val = typename detail::EleTyper<Vec, typename Vec::value_type>::type>
inline size_t ReplaceInVec(Vec& thevec, const Predictor& pred, const Val& val, const size_t cnt = SIZE_MAX)
{
    if (cnt == 0)
        return 0;
    size_t replacedCnt = 0;
    for (auto& obj : thevec)
    {
        if (pred(obj))
        {
            obj = val;
            if (++replacedCnt >= cnt)
                break;
        }
    }
    return replacedCnt;
}

namespace detail
{
template<typename ItType>
struct KeyIterator
{
private:
    ItType InnerIt;
    using KeyType = decltype(InnerIt->first);
public:
    KeyIterator(ItType it) : InnerIt(it) { }
    KeyIterator& operator++()
    {
        InnerIt++; return *this;
    }
    bool operator!=(const KeyIterator<ItType>& other) const { return InnerIt != other.InnerIt; }
    KeyType& operator*() const
    {
        return InnerIt->first;
    }
};
template<typename ItType>
struct ValIterator
{
private:
    ItType InnerIt;
    using PairType = typename std::remove_reference_t<typename std::iterator_traits<ItType>::reference>;
    using ValType = std::conditional_t<std::is_const_v<PairType>, const typename PairType::second_type, typename PairType::second_type>;
public:
    ValIterator(ItType it) : InnerIt(it) { }
    ValIterator& operator++()
    {
        InnerIt++; return *this;
    }
    bool operator!=(const ValIterator<ItType>& other) const { return InnerIt != other.InnerIt; }
    const ValType& operator*() const
    {
        return InnerIt->second;
    }
    ValType& operator*()
    {
        return InnerIt->second;
    }
};
}

template<typename Map>
struct KeySet
{
private:
    Map& TheMap;
    using ItType = decltype(TheMap.begin());
    using ConstItType = decltype(TheMap.cbegin());
public:
    KeySet(Map& themap) : TheMap(themap) { }
    detail::KeyIterator<ItType> begin()
    {
        return detail::KeyIterator<ItType>(TheMap.begin());
    }
    detail::KeyIterator<ItType> end()
    {
        return detail::KeyIterator<ItType>(TheMap.end());
    }
    detail::KeyIterator<ConstItType> begin() const
    {
        return detail::KeyIterator<ConstItType>(TheMap.cbegin());
    }
    detail::KeyIterator<ConstItType> end() const
    {
        return detail::KeyIterator<ConstItType>(TheMap.cend());
    }
};
template<typename Map>
struct ValSet
{
private:
    Map & TheMap;
    using ItType = decltype(TheMap.begin());
    using ConstItType = decltype(TheMap.cbegin());
public:
    ValSet(Map& themap) : TheMap(themap) { }
    detail::ValIterator<ItType> begin()
    {
        return detail::ValIterator<ItType>(TheMap.begin());
    }
    detail::ValIterator<ItType> end()
    {
        return detail::ValIterator<ItType>(TheMap.end());
    }
    detail::ValIterator<ConstItType> begin() const
    {
        return detail::ValIterator<ConstItType>(TheMap.cbegin());
    }
    detail::ValIterator<ConstItType> end() const
    {
        return detail::ValIterator<ConstItType>(TheMap.cend());
    }
};

namespace detail
{

template<class... Args>
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
    const std::tuple<Args&&...> srcs;
public:
    ZIPContainer(Args&&... args) : srcs(std::forward_as_tuple(std::forward<Args>(args)...)) {}
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

template<class... Args>
inline constexpr detail::ZIPContainer<Args...> zip(Args&&... args)
{
    return detail::ZIPContainer<Args...>(std::forward<Args>(args)...);
}
}