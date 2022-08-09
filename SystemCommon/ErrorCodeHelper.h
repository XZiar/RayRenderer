#pragma once
#include "SystemCommonRely.h"
//#include "SystemCommon/StringFormat.h"

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

    /*[[nodiscard]] SYSCOMMONAPI typename ::fmt::u16format_context::iterator Format(fmt::u16format_context& ctx) const;
    [[nodiscard]] SYSCOMMONAPI typename ::fmt::u32format_context::iterator Format(fmt::u32format_context& ctx) const;
    [[nodiscard]] SYSCOMMONAPI typename ::fmt::  wformat_context::iterator Format(fmt::  wformat_context& ctx) const;*/
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

    /*[[nodiscard]] SYSCOMMONAPI typename ::fmt::u16format_context::iterator Format(fmt::u16format_context& ctx) const;
    [[nodiscard]] SYSCOMMONAPI typename ::fmt::u32format_context::iterator Format(fmt::u32format_context& ctx) const;
    [[nodiscard]] SYSCOMMONAPI typename ::fmt::  wformat_context::iterator Format(fmt::  wformat_context& ctx) const;*/
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

    /*[[nodiscard]] SYSCOMMONAPI typename ::fmt::u16format_context::iterator Format(fmt::u16format_context& ctx) const;
    [[nodiscard]] SYSCOMMONAPI typename ::fmt::u32format_context::iterator Format(fmt::u32format_context& ctx) const;
    [[nodiscard]] SYSCOMMONAPI typename ::fmt::wformat_context::iterator Format(fmt::wformat_context& ctx) const;*/
    [[nodiscard]] SYSCOMMONAPI static std::u16string FormatErrno(int32_t err);
    [[nodiscard]] SYSCOMMONAPI static ErrnoHolder GetCurError();
};

}

//#if COMMON_OS_WIN
//
//template<typename Char>
//struct fmt::formatter<common::HResultHolder, Char>
//{
//    template<typename ParseContext>
//    constexpr auto parse(ParseContext& ctx)
//    {
//        return ctx.begin();
//    }
//    template<typename FormatContext>
//    auto format(const common::HResultHolder& hr, FormatContext& ctx)
//    {
//        return hr.Format(ctx);
//    }
//};
//
//template<typename Char>
//struct fmt::formatter<common::Win32ErrorHolder, Char>
//{
//    template<typename ParseContext>
//    constexpr auto parse(ParseContext& ctx)
//    {
//        return ctx.begin();
//    }
//    template<typename FormatContext>
//    auto format(const common::Win32ErrorHolder& err, FormatContext& ctx)
//    {
//        return err.Format(ctx);
//    }
//};
//
//#endif
//
//template<typename Char>
//struct fmt::formatter<common::ErrnoHolder, Char>
//{
//    template<typename ParseContext>
//    constexpr auto parse(ParseContext& ctx)
//    {
//        return ctx.begin();
//    }
//    template<typename FormatContext>
//    auto format(const common::ErrnoHolder& err, FormatContext& ctx)
//    {
//        return err.Format(ctx);
//    }
//};
