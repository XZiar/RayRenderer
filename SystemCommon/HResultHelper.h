#pragma once
#include "SystemCommonRely.h"
#include "StringUtil/Format.h"

namespace common
{

std::u16string SYSCOMMONAPI HrToStr(long hresult);
struct HResultHolder
{
    long Value;

    constexpr HResultHolder() noexcept : Value(0) {}
    template<typename T, typename = std::enable_if_t<std::is_same_v<T, long>>>
    constexpr HResultHolder(T hr) noexcept : Value(hr) {}

    [[nodiscard]] forceinline std::u16string ToStr() const { return HrToStr(Value); }
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
