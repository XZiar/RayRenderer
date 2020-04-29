#pragma once
#include "CommonRely.hpp"
#include <string>
#include <string_view>
#include <tuple>
#include <array>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>


namespace common::str
{

template<typename T>
void Stringify(const T& data, std::string& dest);


template<typename T, size_t Index = 0>
void StringifyTuple(const T& data, std::string& dest)
{
    if constexpr (Index < std::tuple_size_v<T>)
    {
        if constexpr (Index == 0)
            dest.append("(");
        Stringify(std::get<Index>(data), dest);
        dest.append(", ");
        StringifyTuple<T, Index + 1>(data, dest);
    }
    else
    {
        dest.pop_back();
        dest.back() = ')';
    }
}

template<typename T1, typename T2>
void Stringify(const std::pair<T1, T2>& data, std::string& dest)
{
    dest.append("(");
    Stringify(data.first, dest);
    dest.append(", ");
    Stringify(data.second, dest);
    dest.append(")");
}
template<typename... Ts>
void Stringify(const std::tuple<Ts...>& data, std::string& dest)
{
    return TupleToString(data);
}
template<typename T, size_t N>
void Stringify(const T(&data)[N], std::string& dest)
{
    dest.append("[");
    for (const auto& dat : data)
    {
        Stringify(dat, dest);
        dest.append(", ");
    }
    dest.pop_back();
    dest.back() = ']';
}
template<typename T>
void Stringify(const T& data, std::string& dest)
{
    if constexpr (std::is_same_v<T, bool>)
        dest.append(data ? "true" : "false");
    else if constexpr (std::is_arithmetic_v<T>)
        dest.append(std::to_string(data));
    else if constexpr (common::is_specialization<T, std::basic_string>::value)
        dest.append("\"").append(data).append("\"");
    else if constexpr (common::is_specialization<T, std::basic_string_view>::value)
        dest.append("\"").append(data.data(), data.size()).append("\"");
    else if constexpr (common::is_specialization<T, std::vector>::value
        || common::is_specialization<T, std::list>::value
        || common::is_specialization<T, std::set>::value
        || common::is_specialization<T, std::unordered_set>::value
        || common::is_specialization<T, std::multiset>::value)
    {
        dest.append("[");
        for (const auto& dat : data)
        {
            Stringify(dat, dest);
            dest.append(", ");
        }
        dest.pop_back();
        dest.back() = ']';
    }
    else if constexpr (common::is_specialization<T, std::map>::value
        || common::is_specialization<T, std::unordered_map>::value
        || common::is_specialization<T, std::multimap>::value)
    {
        dest.append("{");
        for (const auto& [key, val] : data)
        {
            Stringify(key, dest);
            dest.append(": ");
            Stringify(val, dest);
            dest.append(", ");
        }
        dest.pop_back();
        dest.back() = '}';
    }
    else
    {
        static_assert(!common::AlwaysTrue<T>, "not support");
    }
}


template<typename T>
std::string Stringify(const T& data)
{
    std::string ret;
    Stringify(data, ret);
    return ret;
}

}