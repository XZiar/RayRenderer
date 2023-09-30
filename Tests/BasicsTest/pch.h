#pragma once

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "3rdParty/Projects/googletest/gtest-enhanced.h"
#include <type_traits>
#include <iterator>
#include <utility>
#include <limits>
#include <functional>
#include <memory>
#include <optional>
#include <vector>
#include <array>
#include <tuple>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <cinttypes>


std::mt19937& GetRanEng();
uint32_t GetARand();
