#pragma once
#include "CommonRely.hpp"
#include "AlignedBase.hpp"
#include <atomic>

namespace common
{

template<typename T>
class RefHolder
{
protected:
    static constexpr size_t Offset = sizeof(std::atomic_uint32_t);
    static void AllocateRef(std::byte* ptr) noexcept
    {
        new (ptr)std::atomic_uint32_t(1);
    }
    void Increase() noexcept
    {
        auto ptrcnt = static_cast<T*>(this)->GetCountor();
        if (ptrcnt)
            (*ptrcnt)++;
    }
    void Decrease() noexcept
    {
        auto ptrcnt = static_cast<T*>(this)->GetCountor();
        if (ptrcnt && (*ptrcnt)-- == 1)
            free(ptrcnt);
    }
};


template<typename T, typename E>
class COMMON_EMPTY_BASES FixedLenRefHolder : public RefHolder<T>
{
    friend RefHolder<T>;
private:
    static constexpr size_t Offset = std::max(sizeof(std::atomic_uint32_t), alignof(E));
protected:
    [[nodiscard]] static forceinline span<E> Allocate(const size_t length) noexcept
    {
        if (std::byte* ptr = (std::byte*)malloc(Offset + sizeof(E) * length); ptr)
        {
            RefHolder<T>::AllocateRef(ptr);
            E* const ptrArray = reinterpret_cast<E*>(ptr + Offset);
            return { ptrArray, length };
        }
        return {};
    }
    [[nodiscard]] forceinline std::atomic_uint32_t* GetCountor() const noexcept
    {
        uintptr_t ptr = static_cast<const T*>(this)->GetDataPtr();
        if (ptr)
            return reinterpret_cast<std::atomic_uint32_t*>(ptr - Offset);
        else
            return nullptr;
    }
};


}

