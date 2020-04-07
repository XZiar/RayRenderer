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
    using SVType  = std::basic_string_view<T>;
    using StrType = std::string_view<T>;
    const StringPool<T>* Ptr;
    uint32_t Offset, Length;
public:
    StringPiece(const StringPool<T>* ptr, const uint32_t offset, const uint32_t len) :
        Ptr(ptr), Offset(offset), Length(len) { }
    constexpr SVType View() const noexcept
    {
        return { &Ptr.Pool[Offset], Length };
    }
    constexpr StrType Str() const noexcept
    {
        return { &Ptr.Pool[Offset], Length };
    }
    constexpr operator SVType() const noexcept
    {
        return View();
    }
    constexpr bool operator==(const SVType other) const noexcept
    {
        return SVType() == other;
    }
    constexpr bool operator!=(const SVType other) const noexcept
    {
        return SVType() != other;
    }
    constexpr bool operator==(const StrType& other) const noexcept
    {
        return SVType() == other;
    }
    constexpr bool operator!=(const StrType& other) const noexcept
    {
        return SVType() != other;
    }
    constexpr bool operator==(const T* other) const noexcept
    {
        return SVType() == other;
    }
    constexpr bool operator!=(const T* other) const noexcept
    {
        return SVType() != other;
    }
};

template<typename T>
class StringPool : common::NonCopyable
{
    friend class StringPiece<T>;
private:
    std::vector<T> Pool;
public:
    StringPiece<T> Allocate(const std::basic_string_view<T> str)
    {
        const uint32_t offset = gsl::narrow_cast<uint32_t>(Pool.size()),
            size = gsl::narrow_cast<uint32_t>(str.size());
        Pool.insert(Pool.end(), str.cbegin(), str.cend());
        return { this, offset, size };
    }
};

}

