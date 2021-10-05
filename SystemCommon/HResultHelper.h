#pragma once
#include "SystemCommonRely.h"
#include "SystemCommon/StringFormat.h"

namespace common
{

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

    [[nodiscard]] typename ::fmt::u16format_context::iterator SYSCOMMONAPI Format(fmt::u16format_context& ctx) const;
    [[nodiscard]] typename ::fmt::u32format_context::iterator SYSCOMMONAPI Format(fmt::u32format_context& ctx) const;
    [[nodiscard]] typename ::fmt::  wformat_context::iterator SYSCOMMONAPI Format(fmt::  wformat_context& ctx) const;
    [[nodiscard]] static std::u16string SYSCOMMONAPI FormatHr(long hresult);
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

    [[nodiscard]] typename ::fmt::u16format_context::iterator SYSCOMMONAPI Format(fmt::u16format_context& ctx) const;
    [[nodiscard]] typename ::fmt::u32format_context::iterator SYSCOMMONAPI Format(fmt::u32format_context& ctx) const;
    [[nodiscard]] typename ::fmt::  wformat_context::iterator SYSCOMMONAPI Format(fmt::  wformat_context& ctx) const;
    [[nodiscard]] static std::u16string SYSCOMMONAPI FormatError(unsigned long err);
    [[nodiscard]] static Win32ErrorHolder SYSCOMMONAPI GetLastError();
};


}

template<typename Char>
struct fmt::formatter<common::HResultHolder, Char>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }
    template<typename FormatContext>
    auto format(const common::HResultHolder& hr, FormatContext& ctx)
    {
        return hr.Format(ctx);
    }
};

template<typename Char>
struct fmt::formatter<common::Win32ErrorHolder, Char>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }
    template<typename FormatContext>
    auto format(const common::Win32ErrorHolder& hr, FormatContext& ctx)
    {
        return hr.Format(ctx);
    }
};
