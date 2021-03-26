#pragma once

#include "CommonRely.hpp"


/* small bitset that utilize small string optimization */

#include <string>
namespace common
{

class SmallBitset
{
private:
    std::basic_string<char> Real;
public:
    SmallBitset() noexcept { }
    SmallBitset(size_t count, bool initial) noexcept :
        Real((count + 7) / 8, static_cast<char>(initial ? 0xffu : 0x0u))
    { }
    bool Get(size_t idx) const noexcept
    {
        const auto bit = static_cast<uint8_t>(0x1u << (idx % 8));
        return static_cast<uint8_t>(Real[idx / 8]) & bit;
    }
    void Set(size_t idx, const bool val) noexcept
    {
        const auto bit = static_cast<char>(0x1u << (idx % 8));
        char& target = Real[idx / 8];
        if (val)
            target |= bit;
        else
            target &= (~bit);
    }
    bool Flip(size_t idx) noexcept
    {
        const auto bit = static_cast<char>(0x1u << (idx % 8));
        char& target = Real[idx / 8];
        const bool ret = target & bit;
        target ^= bit;
        return ret;
    }
};

}



/* param pack helper */

#include <tuple>
namespace common
{
struct ParamPack
{
    template<typename T, typename... Ts>
    struct FirstType
    {
        using Type = T;
    };

#if COMMON_CPP_17
    template<typename... Ts>
    struct LastType
    {
        using Type = typename decltype((FirstType<Ts>{}, ...))::Type;
    };
#else
    template <typename... Args>
    struct LastType;
    template <typename T>
    struct LastType<T>
    {
        using Type = T;
    };
    template <typename T, typename... Args>
    struct LastType<T, Args...>
    {
        using Type = typename LastType<Args...>::Type;
    };
#endif

    template<size_t N, typename... Ts>
    struct NthType
    {
        using Tuple = std::tuple<FirstType<Ts>...>;
        using Type = typename std::tuple_element<N, Tuple>::type::Type;
    };
};

template<typename F>
inline constexpr auto UnpackedFunc(F&& func) noexcept
{
    return[=](auto&& tuple) { return std::apply(func, tuple); };
}

}



/* variant extra support */

#   include <variant>
#if defined(__cpp_lib_variant)
namespace common
{

template <typename> struct variant_tag { };
template <typename T, typename... Ts>
inline constexpr size_t get_variant_index(variant_tag<std::variant<Ts...>>) noexcept
{
    return std::variant<variant_tag<Ts>...>(variant_tag<T>()).index();
}
template <typename T, typename V>
inline constexpr size_t get_variant_index_v() noexcept
{
    return get_variant_index<T>(variant_tag<V>());
}

template<typename V>
struct VariantHelper
{
    static constexpr auto Indexes = std::make_index_sequence<std::variant_size_v<V>>{};
    template<typename U, size_t... I>
    static constexpr bool Contains_(std::index_sequence<I...>) noexcept
    {
        return (std::is_same_v<std::variant_alternative_t<I, V>, U> || ...);
    }
    template<typename U>
    static constexpr bool Contains() noexcept
    {
        return Contains_<U>(Indexes);
    }
    template<typename V2, size_t... I>
    static constexpr bool ContainsAll_(std::index_sequence<I...>) noexcept
    {
        return (Contains<std::variant_alternative_t<I, V2>>() && ...);
    }
    template<typename V2>
    static constexpr bool ContainsAll() noexcept
    {
        return ContainsAll_<V2>(std::make_index_sequence<std::variant_size_v<V2>>{});
    }
    template<typename V2>
    static constexpr V Convert(V2&& var)
    {
        static_assert(ContainsAll<V2>(), "Not all types in V2 exists in V");
        return std::visit([](auto&& arg) -> V { return std::forward<decltype(arg)>(arg); }, var);
    }
};

}
#endif



/* optioanl wrapper */

#include <optional>
#if defined(__cpp_lib_variant)
namespace common
{

namespace detail
{
template<typename T>
struct OptionalT
{
    using Type = T;
    template<typename U>
    static constexpr std::optional<Type> Create(U&& val) noexcept 
    {
        return { std::forward<U>(val) };
    }
};
template<typename T>
struct OptionalT<std::optional<T>>
{
    using Type = T;
    template<typename U>
    static constexpr std::optional<Type> Create(U&& val) noexcept
    {
        return { std::forward<U>(val) };
    }
};
template<typename T>
struct OptionalT<T*>
{
    using Type = T&;
    static constexpr std::optional<Type> Create(T* val) noexcept
    {
        return { val };
    }
};
template<typename T>
struct OptionalT<T&>
{
    using Type = T&;
    static constexpr std::optional<T*> Create(T& val) noexcept
    {
        return { &val };
    }
};
}

template<typename T>
struct Optional
{
    static constexpr bool IsRef = std::is_reference_v<T>;
    static constexpr bool IsNothrowCopy = std::is_nothrow_copy_constructible_v<remove_cvref_t<T>>;
    static constexpr bool IsNothrowMove = std::is_nothrow_move_constructible_v<remove_cvref_t<T>>;
    using TNoRef = std::remove_reference_t<T>;
    using V = std::conditional_t<IsRef, std::add_pointer_t<std::remove_reference_t<T>>, T>;
    std::optional<V> Inner;
    constexpr Optional() noexcept {}
    template<typename... Args>
    constexpr Optional(std::in_place_t, Args&&... args) noexcept : 
        Inner(std::in_place_t{}, std::forward<Args>(args)...) 
    {
        if constexpr (std::is_pointer_v<V>)
        {
            if (Inner.has_value() && Inner.value() == nullptr)
                Inner.reset();
        }
    }
    constexpr Optional(std::optional<V> val) noexcept : Inner(std::move(val))
    {
        if constexpr (std::is_pointer_v<V>)
        {
            if (Inner.has_value() && Inner.value() == nullptr)
                Inner.reset();
        }
    }
    constexpr Optional(Optional<T>&& other) noexcept : Inner(std::move(other.Inner)) {}
    constexpr Optional(const Optional<T>& other) noexcept = delete;
    constexpr Optional& operator=(Optional<T>&& other) noexcept = delete;
    constexpr Optional& operator=(const Optional<T>& other) noexcept = delete;
    constexpr T& Value() noexcept
    {
        if constexpr (std::is_pointer_v<V>)
            return *Inner.value();
        else
            return *Inner;
    }
    constexpr const T& Value() const noexcept
    {
        if constexpr (std::is_pointer_v<V>)
            return *Inner.value();
        else
            return *Inner;
    }
    template<typename R, typename U = TNoRef>
    Optional<typename detail::OptionalT<R&>::Type> Field(R(U::* member)) noexcept
    {
        static_assert(std::is_same_v<U, std::decay_t<T>>);
        if (Inner.has_value())
            return { detail::OptionalT<R&>::Create(Value().*member) };
        else
            return {};
    }
    template<typename R, typename U = TNoRef>
    Optional<typename detail::OptionalT<const R&>::Type> Field(R(U::* member)) const noexcept
    {
        static_assert(std::is_same_v<U, std::decay_t<T>>);
        if (Inner.has_value())
            return { detail::OptionalT<const R&>::Create(Value().*member) };
        else
            return {};
    }
    template<typename R, typename U = TNoRef>
    constexpr Optional<typename detail::OptionalT<R>::Type> To(R(U::* member)) const 
        noexcept(std::is_nothrow_copy_constructible_v<R>)
    {
        static_assert(std::is_same_v<U, std::decay_t<T>>);
        if (Inner.has_value())
            return { detail::OptionalT<R>::Create(Value().*member) };
        else
            return {};
    }
    template<typename... Args, typename R, typename U = TNoRef>
    constexpr Optional<typename detail::OptionalT<R>::Type> To(R(U::* func)(Args...), Args&&... args)
    {
        static_assert(std::is_same_v<U, std::decay_t<T>>);
        if (Inner.has_value())
            return { detail::OptionalT<R>::Create((Value().*func)(std::forward<Args>(args)...)) };
        else
            return {};
    }
    template<typename... Args, typename R, typename U = TNoRef>
    constexpr Optional<typename detail::OptionalT<R>::Type> To(R(U::* func)(Args...) noexcept, Args&&... args) noexcept
    {
        static_assert(std::is_same_v<U, std::decay_t<T>>);
        if (Inner.has_value())
            return { detail::OptionalT<R>::Create((Value().*func)(std::forward<Args>(args)...)) };
        else
            return {};
    }
    template<typename... Args, typename R, typename U = TNoRef>
    constexpr Optional<typename detail::OptionalT<R>::Type> To(R(U::* func)(Args...) const, Args&&... args) const
    {
        static_assert(std::is_same_v<U, std::decay_t<T>>);
        if (Inner.has_value())
            return { detail::OptionalT<R>::Create((Value().*func)(std::forward<Args>(args)...)) };
        else
            return {};
    }
    template<typename... Args, typename R, typename U = TNoRef>
    constexpr Optional<typename detail::OptionalT<R>::Type> To(R(U::* func)(Args...) const noexcept, Args&&... args) const noexcept
    {
        static_assert(std::is_same_v<U, std::decay_t<T>>);
        if (Inner.has_value())
            return { detail::OptionalT<R>::Create((Value().*func)(std::forward<Args>(args)...)) };
        else
            return {};
    }
    constexpr T Or(T val) const noexcept(IsNothrowCopy || IsNothrowMove)
    {
        if (Inner.has_value())
            return Value();
        else
            return std::move(val);
    }
    constexpr T OrMove(T val) noexcept(IsNothrowMove)
    {
        static_assert(std::is_const_v<std::remove_reference_t<T>>, "cannot move a cosnt value");
        if (Inner.has_value())
            return std::move(Value());
        else
            return std::move(val);
    }
    template<typename F>
    constexpr T OrLazy(F&& func) const noexcept(IsNothrowCopy && noexcept(func()))
    {
        if (Inner.has_value())
            return Value();
        else
            return func();
    }
    template<typename F>
    constexpr T OrMoveLazy(F&& func) noexcept(IsNothrowMove && noexcept(func()))
    {
        static_assert(std::is_const_v<std::remove_reference_t<T>>, "cannot move a cosnt value");
        if (Inner.has_value())
            return std::move(Value());
        else
            return func();
    }
};
template<typename T>
using OptionalT = Optional<typename detail::OptionalT<T>::Type>;

}
#endif

