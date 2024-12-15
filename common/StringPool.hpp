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
    forceinline constexpr size_t GetOffset() const noexcept { return Offset; }
    forceinline constexpr size_t GetLength() const noexcept { return Length; }
};

template<typename T>
class HashedStringPiece : public StringPiece<T>
{
    uint64_t Hash;
public:
    constexpr HashedStringPiece() noexcept : Hash(0) {}
    constexpr HashedStringPiece(StringPiece<T> piece, uint64_t hash) noexcept : StringPiece<T>(piece), Hash(hash) 
    { }
    HashedStringPiece(StringPiece<T> piece, const StringPool<T>& pool) noexcept : 
        StringPiece<T>(piece), Hash(DJBHash::HashC(pool.GetStringView(piece)))
    { }
    forceinline constexpr uint64_t GetHash() const noexcept { return Hash; }

    forceinline constexpr bool operator<(const HashedStringPiece<T>& other) const noexcept
    {
        return Hash < other.Hash;
    }
    forceinline constexpr bool operator<(const uint64_t hash) const noexcept
    {
        return Hash < hash;
    }
};


template<typename T>
class StringPool
{
protected:
    struct Allocation
    {
        StringPool<T>& Host;
        uint32_t Offset = 0, Length = 0;
        Allocation(StringPool<T>& host) : Host(host), Offset(gsl::narrow_cast<uint32_t>(Host.Pool.size())) {}
        template<typename... Args>
        Allocation& Add(const Args&... str) noexcept
        {
            Ensures(Offset + Length == Host.Pool.size());
            const auto ret = Host.AllocateConcatString(str...);
            Ensures(ret.Offset == Offset + Length);
            Length += ret.Length;
            return *this;
        }
        constexpr operator StringPiece<T>() const noexcept
        {
            return { Offset, Length };
        }
    };
    std::vector<T> Pool;
public:
    StringPiece<T> AllocateString(const std::basic_string_view<T> str) noexcept
    {
        const uint32_t offset = gsl::narrow_cast<uint32_t>(Pool.size()),
            size = gsl::narrow_cast<uint32_t>(str.size());
        Pool.insert(Pool.end(), str.cbegin(), str.cend());
        return { offset, size };
    }
    template<typename... Args>
    StringPiece<T> AllocateConcatString(const Args&... str) noexcept
    {
        const uint32_t offset = gsl::narrow_cast<uint32_t>(Pool.size());
        (..., void(Pool.insert(Pool.end(), std::begin(str), std::end(str))));
        const uint32_t size = gsl::narrow_cast<uint32_t>(Pool.size() - offset);
        return { offset, size };
    }
    Allocation Allocate() noexcept
    {
        return { *this };
    }
    forceinline std::basic_string_view<T> GetStringView(const StringPiece<T>& piece) const noexcept
    {
        if (piece.Length == 0) return {};
        return { &Pool[piece.Offset], piece.Length };
    }
    forceinline bool IsEmpty() const noexcept { return Pool.empty(); }
    forceinline void Reserve(size_t size) noexcept { return Pool.reserve(size); }
    forceinline std::basic_string_view<T> GetAllStr() const noexcept { return { Pool.data(), Pool.size() }; }
};

template<typename T>
class HashedStringPool : protected StringPool<T>
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
    using StringPool<T>::GetStringView;
    using StringPool<T>::IsEmpty;
    using StringPool<T>::Reserve;
};


}

