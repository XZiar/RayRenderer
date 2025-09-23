#pragma once
#include "SystemCommonRely.h"
#include "FormatInclude.h"
#include <compare>

namespace common
{

#if COMMON_OS_WIN

struct HResultHolder
{
    long Value;

    constexpr HResultHolder() noexcept : Value(0) {}
    template<typename T, typename = std::enable_if_t<std::is_same_v<T, long>>>
    constexpr HResultHolder(T hr) noexcept : Value(hr) {}

    [[nodiscard]] forceinline std::u16string ToStr() const noexcept { return FormatHr(Value); }
    SYSCOMMONAPI void FormatWith(str::FormatterHost& host, str::FormatterContext& context, const str::FormatSpec* spec) const;
    constexpr HResultHolder& operator=(long hr) noexcept
    {
        Value = hr;
        return *this;
    }
    [[nodiscard]] constexpr bool operator==(const HResultHolder& rhs) const noexcept = default;
    [[nodiscard]] constexpr bool operator!=(const HResultHolder& rhs) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(long rhs) const noexcept { return Value <=> rhs; }
    constexpr explicit operator long() const noexcept { return Value; }
    constexpr explicit operator bool() const noexcept { return Value >= 0; }

    [[nodiscard]] SYSCOMMONAPI static std::u16string FormatHr(long hresult) noexcept;
};


struct Win32ErrorHolder
{
    unsigned long Value;

    constexpr Win32ErrorHolder() noexcept : Value(0) {}
    explicit constexpr Win32ErrorHolder(unsigned long err) noexcept : Value(err) {}

    [[nodiscard]] forceinline std::u16string ToStr() const noexcept { return FormatError(Value); }
    SYSCOMMONAPI void FormatWith(str::FormatterHost& host, str::FormatterContext& context, const str::FormatSpec* spec) const;
    constexpr Win32ErrorHolder& operator=(unsigned long err) noexcept
    {
        Value = err;
        return *this;
    }
    [[nodiscard]] constexpr bool operator==(const Win32ErrorHolder& rhs) const noexcept = default;
    [[nodiscard]] constexpr bool operator!=(const Win32ErrorHolder& rhs) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(unsigned long rhs) const noexcept { return Value <=> rhs; }
    constexpr explicit operator unsigned long() const noexcept { return Value; }
    constexpr explicit operator bool() const noexcept { return Value == 0; }

    [[nodiscard]] SYSCOMMONAPI static std::u16string FormatError(unsigned long err) noexcept;
    [[nodiscard]] SYSCOMMONAPI static Win32ErrorHolder GetLastError() noexcept;
};

#endif

struct ErrnoHolder
{
    int32_t Value;

    constexpr ErrnoHolder() noexcept : Value(0) {}
    template<typename T, typename = std::enable_if_t<std::is_same_v<T, int32_t>>>
    constexpr ErrnoHolder(T err) noexcept : Value(err) {}

    [[nodiscard]] forceinline std::u16string ToStr() const noexcept { return FormatErrno(Value); }
    SYSCOMMONAPI void FormatWith(str::FormatterHost& host, str::FormatterContext& context, const str::FormatSpec* spec) const;
    constexpr ErrnoHolder& operator=(int32_t err) noexcept
    {
        Value = err;
        return *this;
    }
    [[nodiscard]] constexpr bool operator==(const ErrnoHolder& rhs) const noexcept = default;
    [[nodiscard]] constexpr bool operator!=(const ErrnoHolder& rhs) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(int32_t rhs) const noexcept { return Value <=> rhs; }
    constexpr explicit operator int32_t() const noexcept { return Value; }
    constexpr explicit operator bool() const noexcept { return Value == 0; }

    [[nodiscard]] SYSCOMMONAPI static std::u16string FormatErrno(int32_t err) noexcept;
    [[nodiscard]] SYSCOMMONAPI static ErrnoHolder GetCurError() noexcept;
};

}

