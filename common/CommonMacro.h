#pragma once

#include <type_traits>

/*
* Concatenate preprocessor tokens A and B without expanding macro definitions
* (however, if invoked from a macro, macro arguments are expanded).
*/
#define PPCAT_NX(A, B) A ## B

/*
* Concatenate preprocessor tokens A and B after macro-expanding them.
*/
#define PPCAT(A, B) PPCAT_NX(A, B)

/*
* Turn A into a string literal without expanding macro definitions
* (however, if invoked from a macro, macro arguments are expanded).
*/
#define STRINGIZE_NX(A) #A

/*
* Turn A into a string literal after macro-expanding it.
*/
#define STRINGIZE(A) STRINGIZE_NX(A)


#define ENUM_CLASS_BITFIELD_FUNC(T, U) \
inline constexpr T operator & (const T x, const T y) { return static_cast<T>(static_cast<U>(x) & static_cast<U>(y)); } \
inline constexpr T& operator &= (T& x, const T y) { x = x & y; return x; } \
inline constexpr T operator | (const T x, const T y) { return static_cast<T>(static_cast<U>(x) | static_cast<U>(y)); } \
inline constexpr T& operator |= (T& x, const T y) { x = x | y; return x; } \
inline constexpr T operator ^ (const T x, const T y) { return static_cast<T>(static_cast<U>(x) ^ static_cast<U>(y)); } \
inline constexpr T& operator ^= (T& x, const T y) { x = x ^ y; return x; } \
inline constexpr T operator ~ (const T x) { return static_cast<T>(~static_cast<U>(x)); }

#define MAKE_ENUM_BITFIELD(T) ENUM_CLASS_BITFIELD_FUNC(T, std::underlying_type_t<T>)