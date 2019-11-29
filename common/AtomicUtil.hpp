#pragma once

#include "CommonRely.hpp"
#include <atomic>


namespace common
{


template<typename T>
inline bool UpdateAtomicMaximum(std::atomic<T>& dest, const T& newVal)
{
    T oldVal = dest.load();
    while (oldVal < newVal)
    {
        if (dest.compare_exchange_weak(oldVal, newVal))
            return true;
    }
    return false;
}


template<typename T>
inline bool UpdateAtomicMinimum(std::atomic<T>& dest, const T& newVal)
{
    T oldVal = dest.load();
    while (oldVal > newVal)
    {
        if (dest.compare_exchange_weak(oldVal, newVal))
            return true;
    }
    return false;
}


template<typename T>
struct AtomicBitfield : protected std::atomic<std::underlying_type_t<T>>
{
    using DT = std::underlying_type_t<T>;
public:
    constexpr AtomicBitfield() : std::atomic<std::underlying_type_t<T>>(0) {}
    constexpr AtomicBitfield(const T field) : std::atomic<std::underlying_type_t<T>>((DT)field) {}
    void Add(const T field) { *this |= (DT)field; }
    bool Extract(const T field)
    {
        const auto val = (DT)field;
        const auto prev = this->fetch_and(~val);
        return (prev & val) != 0;
    }
};

template<typename T>
struct NormalBitfield
{
    using DT = std::underlying_type_t<T>;
public:
    DT Value;
    constexpr NormalBitfield() : Value(0) {}
    constexpr NormalBitfield(const T field) : Value((DT)field) {}
    void Add(const T field) { Value |= (DT)field; }
    bool Extract(const T field)
    {
        const auto val = (DT)field;
        const bool has = (Value & val) != 0;
        Value &= (~val);
        return has;
    }
};


}
