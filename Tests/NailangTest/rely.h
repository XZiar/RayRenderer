#pragma once
#include "3rdParty/Projects/googletest/gtest-enhanced.h"
#include <type_traits>
#include <string>

template<typename T>
std::string WideToChar(std::basic_string_view<T> str)
{
    std::string ret; ret.reserve(str.size());
    for (const auto ch : str)
        ret.push_back(static_cast<char>(ch));
    return ret;
}

