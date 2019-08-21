#pragma once

#include "CommonRely.hpp"
#include <string_view>


namespace common
{

struct Enumer
{
    template<auto E>
    constexpr std::string_view ToName()
    {
#if COMPILER_MSVC
        std::string_view funcName = __FUNCSIG__;
        constexpr size_t suffix = 3;
#elif COMPILER_CLANG // need clang>4
        std::string_view funcName = __PRETTY_FUNCTION__;
        constexpr size_t suffix = 4;
#elif COMPILER_GCC // need gcc>9
        std::string_view funcName = __PRETTY_FUNCTION__;
        constexpr size_t suffix = 51;
#endif
        funcName.remove_suffix(suffix);
        for (auto i = funcName.length(); i > 0; --i)
            if (funcName[i - 1] == ':')
                return { funcName.data() + i, funcName.length() - i };
        return funcName;
    }
};

}
