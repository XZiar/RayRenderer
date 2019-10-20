#pragma once

#include "CommonRely.hpp"
#include <string_view>
#include <initializer_list>


#define ENUM_CLASS_BITFIELD_FUNCS(T, U) \
inline constexpr T  operator &  (const T x, const T y) { return static_cast<T>(static_cast<U>(x) & static_cast<U>(y)); }    \
inline constexpr T  operator |  (const T x, const T y) { return static_cast<T>(static_cast<U>(x) | static_cast<U>(y)); }    \
inline constexpr T  operator ^  (const T x, const T y) { return static_cast<T>(static_cast<U>(x) ^ static_cast<U>(y)); }    \
inline constexpr T& operator &= (T& x, const T y) { x = x & y; return x; }                                                  \
inline constexpr T& operator |= (T& x, const T y) { x = x | y; return x; }                                                  \
inline constexpr T& operator ^= (T& x, const T y) { x = x ^ y; return x; }                                                  \
inline constexpr T  operator ~  (const T x) { return static_cast<T>(~static_cast<U>(x)); }                                  \
inline constexpr bool HAS_FIELD(const T x, const T obj) { return static_cast<U>(x & obj) != 0; }                            \
template<typename... Masks>                                                                                                 \
inline constexpr T REMOVE_MASK(const T x, const Masks... masks)                                                             \
{                                                                                                                           \
    const T mask = (... | masks);                                                                                           \
    return x & (~mask);                                                                                                     \
}                                                                                                                           \


#define MAKE_ENUM_BITFIELD(T) ENUM_CLASS_BITFIELD_FUNCS(T, std::underlying_type_t<T>)

//for std::byte
#if (defined(_HAS_STD_BYTE) && _HAS_STD_BYTE) || (defined(__cplusplus) && (__cplusplus >= 201703L)) || defined(__cpp_lib_byte)
inline constexpr bool HAS_FIELD(const std::byte b, const uint8_t bits) { return static_cast<uint8_t>(b & std::byte(bits)) != 0; }
#endif


#define ENUM_CLASS_RANGE_FUNCS(T, U) \
inline constexpr bool operator <  (const T x, const T y) { return static_cast<U>(x) <  static_cast<U>(y); }                 \
inline constexpr bool operator <= (const T x, const T y) { return static_cast<U>(x) <= static_cast<U>(y); }                 \
inline constexpr bool operator >  (const T x, const T y) { return static_cast<U>(x) >  static_cast<U>(y); }                 \
inline constexpr bool operator >= (const T x, const T y) { return static_cast<U>(x) >= static_cast<U>(y); }                 \
inline constexpr bool operator <  (const T x, const U y) { return static_cast<U>(x) <                 y ; }                 \
inline constexpr bool operator <= (const T x, const U y) { return static_cast<U>(x) <=                y ; }                 \
inline constexpr bool operator >  (const T x, const U y) { return static_cast<U>(x) >                 y ; }                 \
inline constexpr bool operator >= (const T x, const U y) { return static_cast<U>(x) >=                y ; }                 \
inline constexpr bool operator == (const T x, const U y) { return static_cast<U>(x) ==                y ; }                 \
inline constexpr bool operator != (const T x, const U y) { return static_cast<U>(x) !=                y ; }                 \
inline constexpr bool operator <  (const U x, const T y) { return                x  <  static_cast<U>(y); }                 \
inline constexpr bool operator <= (const U x, const T y) { return                x  <= static_cast<U>(y); }                 \
inline constexpr bool operator >  (const U x, const T y) { return                x  >  static_cast<U>(y); }                 \
inline constexpr bool operator >= (const U x, const T y) { return                x  >= static_cast<U>(y); }                 \
inline constexpr bool operator == (const U x, const T y) { return                x  == static_cast<U>(y); }                 \
inline constexpr bool operator != (const U x, const T y) { return                x  != static_cast<U>(y); }                 \
inline constexpr T  operator +  (const T x, const T y) { return static_cast<T>(static_cast<U>(x) + static_cast<U>(y)); }    \
inline constexpr T  operator -  (const T x, const T y) { return static_cast<T>(static_cast<U>(x) - static_cast<U>(y)); }    \
inline constexpr T  operator +  (const T x, const U y) { return static_cast<T>(static_cast<U>(x) +                y ); }    \
inline constexpr T  operator -  (const T x, const U y) { return static_cast<T>(static_cast<U>(x) -                y ); }    \
inline constexpr T  operator +  (const U x, const T y) { return static_cast<T>(               x  + static_cast<U>(y)); }    \
inline constexpr T  operator -  (const U x, const T y) { return static_cast<T>(               x  - static_cast<U>(y)); }    \
inline constexpr T& operator += (T& x, const T y) { x = x + y; return x; }                                                  \
inline constexpr T& operator -= (T& x, const T y) { x = x - y; return x; }                                                  \
inline constexpr T& operator += (T& x, const U y) { x = x + y; return x; }                                                  \
inline constexpr T& operator -= (T& x, const U y) { x = x - y; return x; }                                                  \
inline constexpr T& operator ++ (T& x) { x = x + static_cast<U>(1); return x; }                                             \
inline constexpr T& operator -- (T& x) { x = x - static_cast<U>(1); return x; }                                             \
inline constexpr T  operator ++ (T& x, int) { const T ret = x; ++x; return ret; }                                           \
inline constexpr T  operator -- (T& x, int) { const T ret = x; --x; return ret; }                                           \


#define MAKE_ENUM_RANGE(T) ENUM_CLASS_RANGE_FUNCS(T, std::underlying_type_t<T>)


namespace common
{

template<typename T>
[[nodiscard]] inline constexpr std::enable_if_t<std::is_enum_v<T>, std::underlying_type_t<T>>
enum_cast(const T val)
{
    return static_cast<std::underlying_type_t<T>>(val);
}

struct Enumer
{
    template<auto E>
    [[nodiscard]] constexpr std::string_view ToName() noexcept
    {
#if COMPILER_MSVC
        std::string_view funcName = __FUNCSIG__;
        constexpr size_t suffix = 3;
#elif COMPILER_CLANG
#  if __clang_major__ < 4
        static_assert(AlwaysTrue<E>(), "Requires at least Clang 4 to correctly reflect");
#  endif
        std::string_view funcName = __PRETTY_FUNCTION__;
        constexpr size_t suffix = 4;
#elif COMPILER_GCC
#  if __GNUC__ < 9
        static_assert(AlwaysTrue<E>(), "Requires at least GCC 9 to correctly reflect");
#  endif
        std::string_view funcName = __PRETTY_FUNCTION__;
        constexpr size_t suffix = 51;
#endif
        funcName.remove_suffix(suffix);
        for (auto i = funcName.length(); i > 0; --i)
            if (funcName[i - 1] == ':')
                return { funcName.data() + i, funcName.length() - i };
        return funcName;
    }
};

}
