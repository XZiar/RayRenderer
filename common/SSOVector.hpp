#pragma once

#include "CommonRely.hpp"
#include <string>

namespace common
{


template<typename T>
class SSOVector
{
private:
    static_assert(std::is_trivial_v<T>, "Type need to be trivial");
    using EleSize = sizeof(T);
    static_assert(EleSize == 1 || EleSize == 2 || EleSize == 4, "Type need to 1/2/4 byte only");
    using U = std::conditional_t<EleSize == 1, char, std::conditional_t<EleSize == 2, char16_t, char32_t>>;
    std::basic_string<U> Real;
public:
    U& push_back(T val)
    {
        Real.push_back(static_cast<U>(val));
        return *reinterpret_cast<U*>(&Real.back());
    }
    U* data() noexcept
    {
        return reinterpret_cast<U*>(&Real.data());
    }
    const U* data() const noexcept
    {
        return reinterpret_cast<const U*>(&Real.data());
    }
    size_t* size() const noexcept
    {
        return Real.size();
    }
};


}

