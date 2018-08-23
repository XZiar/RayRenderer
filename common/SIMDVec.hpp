#pragma once
#include "SIMD.hpp"

namespace common
{
namespace simd
{


template<typename T, size_t N>
struct alignas(T) Pack
{
    T Data[N];
    Pack(const T& val0, const T& val1) : Data{ val0,val1 } {}
    constexpr T& operator[](const size_t idx) { return Data[idx]; }
    constexpr const T& operator[](const size_t idx) const { return Data[idx]; }
};

template<typename From, typename To>
struct CastTyper
{
    using Type = std::conditional_t<(From::Count > To::Count), Pack<To, From::Count / To::Count>, To>;
};

namespace detail
{

template<typename T>
struct CommonOperators
{
    // logic operations
    T operator&(const T& other) const { return static_cast<const T*>(this)->And(other); }
    T operator|(const T& other) const { return static_cast<const T*>(this)->Or(other); }
    T operator^(const T& other) const { return static_cast<const T*>(this)->Xor(other); }
    T operator~() const { return static_cast<const T*>(this)->Not(); }
    // arithmetic operations
    T operator+(const T& other) const { return static_cast<const T*>(this)->Add(other); }
    T operator-(const T& other) const { return static_cast<const T*>(this)->Sub(other); }
};

}


}
}