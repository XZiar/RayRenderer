#pragma once

#include "CommonRely.hpp"
#include <string_view>
#include <initializer_list>

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>


namespace common::enumex
{

namespace detail
{

template<typename T, size_t Lo, size_t Hi>
inline constexpr T BitRange()
{
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>, "Type must be unsigned integer");
    static_assert(Lo <= Hi, "Wrong range");
    static_assert(Hi < sizeof(T) * 8, "Range overflow");
    T ret = 0;
    for (size_t bit = Lo; bit <= Hi; ++bit)
        ret |= ((0x1) << bit);
    return ret;
}

template<typename T>
struct EnumHolderBase
{
    static_assert(std::is_unsigned_v<typename T::InnerType>);
    typename T::InnerType Value;
};

template<typename E, typename T>
struct EnumHolder : public EnumHolderBase<E>
{
    using TargetType = T;
};

template<typename T, typename U>
class EnumPartCHolder
{
    friend T;
protected:
    const T& Holder;
    constexpr EnumPartCHolder(const T* holder) noexcept : Holder(*holder) { }
public:
    using   EnumType = T;
    using TargetType = U;
    template<typename X>
    constexpr std::enable_if_t<std::is_convertible_v<X, U>, bool> operator==(const X other) const noexcept
    {
        return (Holder.Value & U::Mask_) == static_cast<U>(other).Value;
    }
    constexpr operator U() const noexcept
    {
        return U(Holder.Value & U::Mask_);
    }
    constexpr T Removed() const noexcept
    {
        return T{ Holder.Value & U::InvMask_ };
    }
};

template<typename T, typename U>
class EnumPartHolder : public EnumPartCHolder<T, U>
{
    friend T;
    constexpr EnumPartHolder(T* holder) noexcept : EnumPartCHolder<T, U>(holder) { }
public:
    using   EnumType = T;
    using TargetType = U;
    constexpr void operator=(U value) noexcept
    {
        auto& holder = const_cast<T&>(this->Holder);
        holder.Value = (holder.Value & U::InvMask_) | value.Value;
    }
    constexpr T& Remove() noexcept
    {
        auto& holder = const_cast<T&>(this->Holder);
        holder.Value &= U::InvMask_;
        return holder;
    }
};

template<typename T, typename E>
inline constexpr bool IsEnumPart() noexcept
{
    if constexpr (is_specialization<T, EnumPartHolder>::value)
    {
        return std::is_same_v<typename T::EnumType, E>;
    }
    else if constexpr (std::is_base_of_v<EnumHolderBase<E>, T>)
    {
        return !std::is_same_v<EnumHolderBase<E>, T>;
    }
    else
        return false;
}

}

template<typename T>
struct EnumBase
{
    using InnerType = T;
    T Value;
    constexpr EnumBase() noexcept : Value(0) { }
    explicit constexpr EnumBase(T value) noexcept : Value(value) { }
    constexpr operator T() const noexcept { return Value; }
protected:
    template<typename U>
    static constexpr T ReplaceVal(T value, const U& part) noexcept
    {
        if constexpr (is_specialization<U, detail::EnumPartHolder>::value)
        {
            return ReplaceVal(value, static_cast<typename U::TargetType>(part));
        }
        else if constexpr (is_specialization<U, detail::EnumHolder>::value)
        {
            return ReplaceVal(value, static_cast<typename U::TargetType>(part));
        }
        else
            return (value & U::InvMask_) | part.Value;
    }
};

template<typename T>
[[nodiscard]] inline constexpr auto enum_cast2(const EnumBase<T> val) -> typename T::InnerType
{
    return val.Value;
}

}


#define XZENUM_LOGIC_BASIC(T, U)                                                        \
constexpr T  operator &  (const U y) const noexcept { return T(Value & y.Value); }      \
constexpr T  operator |  (const U y) const noexcept { return T(Value | y.Value); }      \
constexpr T  operator ^  (const U y) const noexcept { return T(Value ^ y.Value); }      \
constexpr T  operator ~  ()          const noexcept { return T(~Value); }               \
constexpr T& operator &= (const U y)       noexcept { Value &= y.Value; return *this; } \
constexpr T& operator |= (const U y)       noexcept { Value |= y.Value; return *this; } \
constexpr T& operator ^= (const U y)       noexcept { Value ^= y.Value; return *this; } \


#define XZENUM_ITEM(name, value) static constexpr ENType name = { value };


#define XZENUM_BITSPART(name, bitLo, bitHi, ...)                                                \
struct name;                                                                                    \
constexpr ::common::enumex::detail::EnumPartHolder<Type_, name> name##_() noexcept              \
{ return this; }                                                                                \
constexpr ::common::enumex::detail::EnumPartCHolder<Type_, name> name##_() const noexcept       \
{ return this; }                                                                                \
struct name : public ::common::enumex::detail::EnumHolderBase<Type_>                            \
{                                                                                               \
    friend ::common::enumex::detail::EnumPartHolder<Type_, name>;                               \
    friend ::common::enumex::detail::EnumPartCHolder<Type_, name>;                              \
    friend Type_;                                                                               \
private:                                                                                        \
    constexpr name(InnerType value) noexcept : EnumHolderBase{ value } { }                      \
public:                                                                                         \
    static constexpr InnerType Mask_ = ::common::enumex::detail::BitRange<InnerType, bitLo, bitHi>();   \
    static constexpr InnerType InvMask_ = ~Mask_;                                               \
    using ENType = ::common::enumex::detail::EnumHolder<Type_, name>;                           \
    constexpr name(ENType value) noexcept : EnumHolderBase{ value.Value } { }                   \
    constexpr bool operator==(const name other) const noexcept { return Value == other.Value; } \
    static constexpr name From(InnerType value) noexcept                                        \
    {                                                                                           \
        Expects((value & InvMask_) == 0);                                                       \
        return value;                                                                           \
    }                                                                                           \
    __VA_ARGS__                                                                                 \
};  


#define XZENUM_MULTI(name, type, ...)                                                   \
struct name : public ::common::enumex::EnumBase<type>                                   \
{                                                                                       \
private:                                                                                \
    template<typename T>                                                                \
    constexpr name operator | (const T y) const noexcept                                \
    {                                                                                   \
        return name{ ReplaceVal(Value, y) };                                            \
    }                                                                                   \
public:                                                                                 \
    using InnerType = type;                                                             \
    using Type_ = name;                                                                 \
    using ::common::enumex::EnumBase<type>::EnumBase;                                   \
    template<typename... Args>                                                          \
    static constexpr name Build(Args... args) noexcept                                  \
    {                                                                                   \
        static_assert((::common::enumex::detail::IsEnumPart<Args, name>() && ...));     \
        name format;                                                                    \
        format = (format | ... | args);                                                 \
        return format;                                                                  \
    }                                                                                   \
    template<typename T, typename =                                                     \
        std::enable_if_t<::common::enumex::detail::IsEnumPart<T, name>()>>              \
        constexpr name& operator |= (const T y) noexcept                                \
    {                                                                                   \
        Value = ReplaceVal(Value, y);                                                   \
        return *this;                                                                   \
    }                                                                                   \
    constexpr bool operator==(const name& other) const noexcept                         \
    { return Value == other.Value; }                                                    \
    __VA_ARGS__                                                                         \
};


