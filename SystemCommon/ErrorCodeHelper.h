#pragma once
#include "SystemCommonRely.h"

namespace common
{

#if COMMON_OS_WIN

struct HResultHolder
{
    long Value;

    constexpr HResultHolder() noexcept : Value(0) {}
    template<typename T, typename = std::enable_if_t<std::is_same_v<T, long>>>
    constexpr HResultHolder(T hr) noexcept : Value(hr) {}

    [[nodiscard]] forceinline std::u16string ToStr() const { return FormatHr(Value); }
    constexpr HResultHolder& operator=(long hr) noexcept
    {
        Value = hr;
        return *this;
    }
    constexpr explicit operator long() const noexcept { return Value; }
    constexpr explicit operator bool() const noexcept { return Value >= 0; }

    [[nodiscard]] SYSCOMMONAPI static std::u16string FormatHr(long hresult);
};


struct Win32ErrorHolder
{
    unsigned long Value;

    constexpr Win32ErrorHolder() noexcept : Value(0) {}
    explicit constexpr Win32ErrorHolder(unsigned long err) noexcept : Value(err) {}

    [[nodiscard]] forceinline std::u16string ToStr() const { return FormatError(Value); }
    constexpr Win32ErrorHolder& operator=(unsigned long err) noexcept
    {
        Value = err;
        return *this;
    }
    constexpr explicit operator unsigned long() const noexcept { return Value; }
    constexpr explicit operator bool() const noexcept { return Value >= 0; }

    [[nodiscard]] SYSCOMMONAPI static std::u16string FormatError(unsigned long err);
    [[nodiscard]] SYSCOMMONAPI static Win32ErrorHolder GetLastError();
};

#endif

struct ErrnoHolder
{
    int32_t Value;

    constexpr ErrnoHolder() noexcept : Value(0) {}
    template<typename T, typename = std::enable_if_t<std::is_same_v<T, int32_t>>>
    constexpr ErrnoHolder(T err) noexcept : Value(err) {}

    [[nodiscard]] forceinline std::u16string ToStr() const { return FormatErrno(Value); }
    constexpr ErrnoHolder& operator=(int32_t err) noexcept
    {
        Value = err;
        return *this;
    }
    constexpr explicit operator int32_t() const noexcept { return Value; }
    constexpr explicit operator bool() const noexcept { return Value == 0; }

    [[nodiscard]] SYSCOMMONAPI static std::u16string FormatErrno(int32_t err);
    [[nodiscard]] SYSCOMMONAPI static ErrnoHolder GetCurError();
};

}

