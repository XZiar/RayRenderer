#pragma once
#include "SystemCommonRely.h"

namespace common::com
{

template<typename T>
struct PtrProxy
{
    void* Pointer;
    constexpr PtrProxy() noexcept : Pointer(nullptr) {}
    constexpr PtrProxy(const PtrProxy<T>& ptr) noexcept : Pointer(ptr.Pointer)
    {
        if (Pointer)
            Ptr()->AddRef();
    }
    constexpr PtrProxy(PtrProxy<T>&& ptr) noexcept : Pointer(ptr.Pointer)
    {
        ptr.Pointer = nullptr;
    }
    explicit constexpr PtrProxy(void* ptr) noexcept : Pointer(ptr) {}
    ~PtrProxy()
    {
        if (Pointer)
        {
            Ptr()->Release();
            Pointer = nullptr;
        }
    }
    PtrProxy<T>& operator=(const PtrProxy<T>& ptr) noexcept
    {
        auto tmp = ptr;
        std::swap(tmp.Pointer, Pointer);
        return *this;
    }
    PtrProxy<T>& operator=(PtrProxy<T>&& ptr) noexcept
    {
        std::swap(ptr.Pointer, Pointer);
        ptr.~PtrProxy();
        return *this;
    }
    auto Ptr() const noexcept
    {
        return reinterpret_cast<T::RealType*>(Pointer);
    }
    template<typename U>
    PtrProxy<U>& AsDerive() noexcept
    {
        static_assert(std::is_base_of_v<typename T::RealType, typename U::RealType>);
        return *reinterpret_cast<PtrProxy<U>*>(this);
    }
    operator T* () const noexcept
    {
        return reinterpret_cast<T*>(Pointer);
    }
    constexpr explicit operator bool() const noexcept
    {
        return Pointer != nullptr;
    }
    auto& operator*() const noexcept
    {
        return *Ptr();
    }
    auto operator->() const noexcept
    {
        return Ptr();
    }
    auto operator&() noexcept
    {
        return reinterpret_cast<typename T::RealType**>(&Pointer);
    }
    auto operator&() const noexcept
    {
        return reinterpret_cast<typename T::RealType* const*>(&Pointer);
    }
};

}