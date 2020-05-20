#pragma once

#include "CommonRely.hpp"
#include <string>
#include <vector>
#include <string_view>

namespace common
{

template<typename T>
class StringPool;

template<typename T>
class StringPiece
{
    friend class StringPool<T>;
    uint32_t Offset, Length;
public:
    constexpr StringPiece() noexcept : Offset(0), Length(0) {}
    constexpr StringPiece(const uint32_t offset, const uint32_t len) noexcept : Offset(offset), Length(len)
    { }
};

template<typename T>
class StringPool
{
private:
    std::vector<T> Pool;
protected:
    StringPiece<T> AllocateString(const std::basic_string_view<T> str)
    {
        const uint32_t offset = gsl::narrow_cast<uint32_t>(Pool.size()),
            size = gsl::narrow_cast<uint32_t>(str.size());
        Pool.insert(Pool.end(), str.cbegin(), str.cend());
        return { offset, size };
    }
    std::basic_string_view<T> GetStringView(StringPiece<T> piece) const noexcept
    {
        return { &Pool[piece.Offset], piece.Length };
    }
};

}

