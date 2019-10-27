#pragma once

#include "Linq2.hpp"
#include "SharedString.hpp"
#include <string>
#include <string_view>


namespace common::str
{

namespace detail
{

template<typename T>
[[nodiscard]] inline constexpr auto ToStringView(T&& val) noexcept
{
    using U = std::decay_t<T>;
    static_assert(common::has_valuetype_v<U>, "only accept type that defined value_type");
    using Char = typename U::value_type;
    if constexpr (std::is_constructible_v<std::basic_string_view<Char>, T>)
        return std::basic_string_view<Char>(std::forward<T>(val));
    else if constexpr (std::is_convertible_v<T, std::basic_string_view<Char>>)
        return (std::basic_string_view<Char>)val;
    else if constexpr (std::is_constructible_v<common::span<const Char>, T>)
    {
        common::span<const Char> space(std::forward<T>(val));
        return std::basic_string_view<Char>(space.data(), space.size());
    }
    else if constexpr (std::is_convertible_v<T, common::span<const Char>>)
    {
        auto space = (common::span<const Char>)val;
        return std::basic_string_view<Char>(space.data(), space.size());
    }
    else
        static_assert(!common::AlwaysTrue<T>(), "connot be converted into string_view");
}


template<typename Char, typename Judger>
struct SplitSource
{
public:
    using OutType = std::basic_string_view<Char>;
    static constexpr bool ShouldCache = false;
    static constexpr bool InvolveCache = false;
    static constexpr bool IsCountable = false;
    static constexpr bool CanSkipMultiple = false;

    std::basic_string_view<Char> Source;
    Judger Splitter;
    size_t CurPos, LastPos;
    bool KeepBlank;

    constexpr SplitSource(std::basic_string_view<Char> source, Judger&& splitter, const bool keepBlank) noexcept :
        Source(source), Splitter(std::forward<Judger>(splitter)), CurPos(0), LastPos(0), KeepBlank(keepBlank)
    {
        FindNextSplitPoint();
    }
    [[nodiscard]] constexpr OutType GetCurrent() const noexcept
    {
        return Source.substr(LastPos, CurPos - LastPos);
    }
    constexpr void MoveNext() noexcept
    {
        LastPos = ++CurPos;
        FindNextSplitPoint();
    }
    [[nodiscard]] constexpr bool IsEnd() const noexcept { return LastPos > Source.size(); }
private:
    constexpr void FindNextSplitPoint() noexcept
    {
        for (; CurPos < Source.size(); CurPos++)
        {
            if (Splitter(Source[CurPos]))
            {
                if (KeepBlank || CurPos != LastPos)
                    return;
                else // !KeepBlank && Cur==Last
                    LastPos++; // skip this part
            }
        }
        // Cur==Last means already reaches end, but if Cur==Size, it will report non-end for empty endpart
        if (CurPos == LastPos && !KeepBlank)
            CurPos++, LastPos++;
    }
};


template<typename T, typename Char, typename Judger>
struct ContainedSplitSource : public common::linq::detail::ObjCache<T>, public SplitSource<Char, Judger>
{
    constexpr ContainedSplitSource(T&& obj, Judger&& splitter, const bool keepBlank) noexcept :
        common::linq::detail::ObjCache<T>(std::move(obj)),
        SplitSource<Char, Judger>(ToStringView(this->Obj()), std::forward<Judger>(splitter), keepBlank) { }
};


template<typename Char, typename T, typename Judger>
[[nodiscard]] inline constexpr auto ToSplitStream(T&& source, Judger&& judger, const bool keepblank) noexcept
{
    if constexpr (!std::is_invocable_r_v<bool, Judger, Char>)
    {
        if constexpr (std::is_same_v<Char, std::decay_t<Judger>>)
            return ToSplitStream<Char>(std::forward<T>(source), [=](const Char ch) noexcept { return ch == judger; }, keepblank);
        else
            static_assert(!common::AlwaysTrue<T>(), "Judger should be delim of Char or a callable that check if Char is a delim");
        //static_assert(std::is_invocable_r_v<bool, Judger, Char>, "Splitter should accept char and return (bool) if should split here.");
    }
    else
    {
        using U = std::decay_t<T>;
        if constexpr (std::is_same_v<U, std::basic_string_view<Char>>)
            return common::linq::ToEnumerable(SplitSource<Char, Judger>
                (std::move(source), std::forward<Judger>(judger), keepblank));
        else
            return common::linq::ToEnumerable(ContainedSplitSource<U, Char, Judger>
                (std::move(source), std::forward<Judger>(judger), keepblank));
    }
}

}


template<typename T, typename Judger>
[[nodiscard]] inline constexpr auto SplitStream(T&& source, Judger&& judger, const bool keepblank = true) noexcept
{
    using U = std::decay_t<T>; 
    if constexpr (std::is_pointer_v<U>)
    {
        auto sv = std::basic_string_view(source);
        using Char = typename decltype(sv)::value_type;
        return detail::ToSplitStream<Char>(sv, std::forward<Judger>(judger), keepblank);
    }
    else if constexpr (common::has_valuetype_v<U>)
    {
        using Char = typename U::value_type;
        if constexpr (std::is_rvalue_reference_v<T>)
        {
            return detail::ToSplitStream<Char>(std::move(source), std::forward<Judger>(judger), keepblank);
        }
        else
        {
            return detail::ToSplitStream<Char>(detail::ToStringView(std::forward<T>(source)), 
                std::forward<Judger>(judger), keepblank);
        }
    }
    else
    {
        static_assert(!common::AlwaysTrue<T>(), "unsupportted Source type");
    }
}


}


