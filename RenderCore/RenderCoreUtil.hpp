#pragma once
#include "RenderCoreRely.h"


namespace rayr
{

template<typename T>
struct AtomicBitfiled : protected std::atomic<std::underlying_type_t<T>>
{
    using DT = std::underlying_type_t<T>;
public:
    constexpr AtomicBitfiled() : std::atomic<std::underlying_type_t<T>>(0) {}
    constexpr AtomicBitfiled(const T field) : std::atomic<std::underlying_type_t<T>>((DT)field) {}
    void Add(const T field) { *this |= (DT)field; }
    bool Extract(const T field) 
    {
        return HAS_FIELD((T)this->fetch_and(~(DT)field), field);
    }
};


}
