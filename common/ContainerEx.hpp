#pragma once

#include <cstdint>
#include <optional>
#include <tuple>
#include <array>
#include <vector>
#include <set>
#include <algorithm>
#include <type_traits>
#include <utility>

namespace common::container
{


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
        return ((left.*Key) < (right.*Key));
    }
};

template<typename T, auto Key>
struct SetPtrKeyLess
{
    using is_transparent = void;
    template<typename C>
    constexpr bool operator()(const T& left, const C& right) const noexcept
    {
        return (((*left).*Key) < right);
    }
    template<typename C>
    constexpr bool operator()(const C& left, const T& right) const noexcept
    {
        return (left < ((*right).*Key));
    }
    constexpr bool operator()(const T& left, const T& right) const noexcept
    {
        return (((*left).*Key) < ((*right).*Key));
    }
};

struct PairLess
{
    using is_transparent = void;
    template<typename K1, typename V1, typename K2, typename V2>
    constexpr bool operator()(const std::pair<K1, V1>& left, const std::pair<K2, V2>& right) const
    {
        if (left.first < right.first) return true;
        if (right.first < left.first) return false;
        return left.second < right.second;
    }
    template<typename Key, typename Val>
    constexpr bool operator()(const std::pair<Key, Val>& left, const Key& right) const
    {
        return (left.first < right);
    }
    template<typename Key, typename Val>
    constexpr bool operator()(const Key& left, const std::pair<Key, Val>& right) const
    {
        return (left < right.first);
    }
};

struct PairGreater
{
    using is_transparent = void;
    template<typename K1, typename V1, typename K2, typename V2>
    constexpr bool operator()(const std::pair<K1, V1>& left, const std::pair<K2, V2>& right) const
    {
        if (left.first > right.first) return true;
        if (right.first > left.first) return false;
        return left.second > right.second;
    }
    template<typename Key, typename Val>
    constexpr bool operator()(const std::pair<Key, Val>& left, const Key& right) const
    {
        return (left.first > right);
    }
    template<typename Key, typename Val>
    constexpr bool operator()(const Key& left, const std::pair<Key, Val>& right) const
    {
        return (left > right.first);
    }
};

namespace detail
{

template<typename T>
using IteratorTyper = decltype(std::declval<T>().begin());
template<typename T>
using ElementTyper = decltype(*std::declval<IteratorTyper<T>>());
template<typename T>
using MapTyper = decltype(std::declval<ElementTyper<T>>().second);

}

template<class Set, typename Key>
inline auto FindInSet(Set& theset, const Key& key) -> decltype(&*theset.begin())
{
    const auto it = theset.find(key);
    if (it == theset.end())//not exist
        return nullptr;
    return &(*it);
}

template<class Set, typename Key, typename Val = detail::ElementTyper<Set>>
inline std::optional<Val> FindInSet(Set& theset, const Key& key, const std::in_place_t)
{
    const auto it = theset.find(key);
    if (it == theset.end())//not exist
        return {};
    return *it;
}

template<class Set, typename Key, typename Val = detail::ElementTyper<Set>>
inline Val FindInSetOrDefault(Set& theset, const Key& key, Val def = Val{})
{
    const auto it = theset.find(key);
    if (it == theset.end())//not exist
        return def;
    return *it;
}

template<class Map, typename Key = typename Map::key_type>
inline auto FindInMap(Map& themap, const Key& key) -> decltype(&(themap.begin()->second))
{
    const auto it = themap.find(key);
    if (it == themap.end())//not exist
        return nullptr;
    return &it->second;
}

template<class Map, typename Key = typename Map::key_type, typename Val = detail::MapTyper<Map>>
inline std::optional<Val> FindInMap(Map& themap, const Key& key, const std::in_place_t)
{
    const auto it = themap.find(key);
    if (it == themap.end())//not exist
        return {};
    return it->second;
}

template<class Map, typename Key = typename Map::key_type, typename Val = detail::MapTyper<Map>>
inline Val FindInMapOrDefault(Map& themap, const Key& key, const Val def = Val{})
{
    const auto it = themap.find(key);
    if (it == themap.end())//not exist
        return def;
    return it->second;
}

template<class Vec, typename Predictor>
inline auto FindInVec(Vec& thevec, const Predictor& pred) -> decltype(&*thevec.begin())
{
    const auto it = std::find_if(thevec.begin(), thevec.end(), pred);
    if (it == thevec.end())//not exist
        return nullptr;
    return &(*it);
}

template<class Vec, typename Predictor, typename Val = detail::ElementTyper<Vec>>
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

template<class Vec, typename Predictor, typename Val = detail::ElementTyper<Vec>>
inline size_t ReplaceInVec(Vec& thevec, const Predictor& pred, const Val& val, const size_t cnt = SIZE_MAX)
{
    static_assert(std::is_invocable_r_v<Predictor, bool, const Val&>, "Predictor type mismatch");
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
    using RawValType = decltype(InnerIt->second);
    using PairType = typename std::remove_reference_t<typename std::iterator_traits<ItType>::reference>;
    using ValType = std::conditional_t<std::is_const_v<PairType>, const RawValType, RawValType>;
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
template<typename Map, bool IsKey>
struct KVSet
{
private:
    Map& TheMap;
    template<typename T> using RetType = std::conditional_t<IsKey, KeyIterator<T>, ValIterator<T>>;
    using ItType = decltype(TheMap.begin());
    using ConstItType = decltype(TheMap.cbegin());
public:
    KVSet(Map& themap) : TheMap(themap) { }
    RetType<ItType> begin()
    {
        return TheMap.begin();
    }
    RetType<ItType> end()
    {
        return TheMap.end();
    }
    RetType<ConstItType> begin() const
    {
        return TheMap.cbegin();
    }
    RetType<ConstItType> end() const
    {
        return TheMap.cend();
    }
};
}
template<typename Map>
struct KeySet : public detail::KVSet<Map, true>
{
    KeySet(Map& themap) : detail::KVSet<Map, true>(themap) { }
};
template<typename Map>
struct ValSet : public detail::KVSet<Map, false>
{
    ValSet(Map& themap) : detail::KVSet<Map, false>(themap) { }
};


template<typename T, typename Compare = std::less<>>
class FrozenDenseSet
{
private:
    std::vector<T> Data;
public:
    FrozenDenseSet() {}
    template<typename T1, typename Alloc>
    FrozenDenseSet(const std::set<T1, Compare, Alloc>& data)
    {
        Data.reserve(data.size());
        for (const auto& dat : data)
            Data.emplace_back(dat);
    }
    template<typename T1, typename Alloc>
    FrozenDenseSet(const std::vector<T1, Alloc>& data)
    {
        Data.resize(data.size());
        std::partial_sort_copy(data.cbegin(), data.cend(), Data.begin(), Data.end(), Compare());
    }
    decltype(auto) begin() const { return Data.cbegin();}
    decltype(auto) end() const { return Data.cend();}


    template<typename E>
    bool Has(E&& element) const
    {
        return std::binary_search(Data.cbegin(), Data.cend(), element, Compare());
    }
};

}