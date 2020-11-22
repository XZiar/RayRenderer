#pragma once

#include "CommonRely.hpp"
#include "StrBase.hpp"
#include <string>
#include <vector>
#include <string_view>

namespace common
{

template<typename T>
class StringPool;
template<typename T>
class HashedStringPool;

template<typename T>
class StringPiece
{
    friend StringPool<T>;
    friend HashedStringPool<T>;
    uint32_t Offset, Length;
public:
    constexpr StringPiece() noexcept : Offset(0), Length(0) {}
    constexpr StringPiece(const uint32_t offset, const uint32_t len) noexcept : Offset(offset), Length(len)
    { }
    constexpr size_t GetLength() const noexcept { return Length; }
};

template<typename T>
class StringPool
{
protected:
    std::vector<T> Pool;
public:
    StringPiece<T> AllocateString(const std::basic_string_view<T> str)
    {
        const uint32_t offset = gsl::narrow_cast<uint32_t>(Pool.size()),
            size = gsl::narrow_cast<uint32_t>(str.size());
        Pool.insert(Pool.end(), str.cbegin(), str.cend());
        return { offset, size };
    }
    std::basic_string_view<T> GetStringView(StringPiece<T> piece) const noexcept
    {
        if (piece.Length == 0) return {};
        return { &Pool[piece.Offset], piece.Length };
    }
};

template<typename T>
class HashedStringPool : public StringPool<T>
{
private:
    static_assert(sizeof(uint64_t) % sizeof(T) == 0);
    static_assert(alignof(std::max_align_t) % alignof(uint64_t) == 0);
    static constexpr size_t UnitCount = sizeof(uint64_t) / sizeof(T);
    static constexpr size_t SizeMask = ~(UnitCount - 1);
public:
    StringPiece<T> AllocateString(const str::HashedStrView<T>& str)
    {
        const auto view = str.View;
        const auto padding = ((view.size() + UnitCount - 1) & SizeMask) - view.size();
        const uint32_t offset = gsl::narrow_cast<uint32_t>(this->Pool.size() + UnitCount),
            size = gsl::narrow_cast<uint32_t>(view.size());
        const auto* ptr = reinterpret_cast<const T*>(&str.Hash);
        this->Pool.reserve(this->Pool.size() + size + UnitCount + padding);
        this->Pool.insert(this->Pool.end(), ptr, ptr + UnitCount);
        this->Pool.insert(this->Pool.end(), view.cbegin(), view.cend());
        if (padding > 0)
            this->Pool.insert(this->Pool.end(), padding, static_cast<T>('\0'));
        return { offset, size };
    }
    str::HashedStrView<T> GetHashedStr(StringPiece<T> piece) const noexcept
    {
        if (piece.Length == 0) return {};
        Expects(piece.Offset >= UnitCount && (piece.Offset % UnitCount == 0));
        const auto* ptr = reinterpret_cast<const uint64_t*>(&this->Pool[piece.Offset - UnitCount]);
        return str::HashedStrView<T>{ *ptr, { &this->Pool[piece.Offset], piece.Length } };
    }
};


}

