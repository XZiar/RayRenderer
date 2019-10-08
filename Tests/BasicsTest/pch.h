//
// pch.h
// Header for standard system include files.
//

#pragma once

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <iterator>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>
#include <array>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <limits>
#include <tuple>


template<uint8_t Base, uint8_t Exp>
inline constexpr uint64_t Pow()
{
    uint64_t ret = Base;
    for (uint8_t i = 1; i < Exp; ++i)
        ret *= Base;
    return ret;
}


std::mt19937& GetRanEng();
uint32_t GetARand();
