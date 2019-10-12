#pragma once

#include "CommonRely.hpp"
#include <type_traits>
#include <utility>
#include <atomic>


namespace common
{

template<typename T>
class RefObject;

namespace detail
{

class RefBlock
{
private:
    static void DeleteSelf(RefBlock* block, void*) noexcept
    {
        delete block;
    }
protected:
    void* Pointer;
private:
    void (*DeleterObj )(void*) noexcept;
    void (*DeleterSelf)(RefBlock*, void*);
    void* Cookie;
    RefBlock* Parent;
    std::atomic_uint32_t RefCount;
protected:
    constexpr RefBlock(void* ptr, void (*deleteObj)(void*) noexcept, void (*deleteSelf)(RefBlock*, void*) noexcept = &DeleteSelf, void* cookie = nullptr) noexcept
        : Pointer(ptr), DeleterObj(deleteObj), DeleterSelf(deleteSelf), Cookie(cookie), Parent(nullptr), RefCount(1)
    { }
public:
    RefBlock(const RefBlock&) = delete;
    RefBlock(RefBlock&&) = delete;
    RefBlock& operator=(const RefBlock&) = delete;
    RefBlock& operator=(RefBlock&&) = delete;

    void AttachParent(RefBlock* parent) noexcept
    {
        if (Parent != parent)
        {
            std::swap(Parent, parent);
            Increase(Parent);
            Decreace(parent);
        }
    }

    static void Increase(RefBlock* block) noexcept
    {
        if (block)
            block->RefCount++;
    }
    static bool Decreace(RefBlock * &block) noexcept
    {
        if (block && block->RefCount-- == 1)
        {
            block->DeleterObj(block->Pointer);
            Decreace(block->Parent);
            block->DeleterSelf(block, block->Cookie);
            block = nullptr;
            return true;
        }
        return false;
    }
};

class AllocTemp
{
private:
    void* Ptr;
public:
    AllocTemp(const size_t size, const size_t align) : Ptr(nullptr)
    {
        Ptr = malloc_align(size, align);
    }
    ~AllocTemp()
    {
        if (Ptr)
            free_align(Ptr);
    }
    std::byte* Get() const noexcept
    {
        return reinterpret_cast<std::byte*>(Ptr);
    }
    void Giveup() noexcept
    {
        Ptr = nullptr;
    }
};

template<typename T>
class RefBlockT : public RefBlock
{
private:
    static void DeleteObj(void* ptr) noexcept
    {
        delete reinterpret_cast<T*>(ptr);
    }
    static void DeleteCombineObj(void* ptr) noexcept
    {
        reinterpret_cast<T*>(ptr)->~T();
    }
    static void DeleteCombine(RefBlock*, void* cookie) noexcept
    {
        free_align(cookie);
    }
    RefBlockT(void* ptr, void* mem) noexcept : RefBlock(ptr, &DeleteCombineObj, &DeleteCombine, mem)
    { }
public:
    RefBlockT(void* ptr) noexcept : RefBlock(ptr, &DeleteObj)
    { }
    T* Get() { return reinterpret_cast<T*>(Pointer); }

    template<typename... Args>
    static RefBlockT<T>* CreateBlockTCombined(Args... args)
    {
        static_assert(std::is_constructible_v<T, Args...>, "Args mismatch to construct object");
        constexpr auto TSize = sizeof(T);
        constexpr auto BSize = sizeof(RefBlock);
        constexpr auto TAlign = alignof(T);
        constexpr auto BAlign = alignof(RefBlock);
        constexpr auto TMem = ((TSize + BAlign - 1) / BAlign) * BAlign;
        AllocTemp mem(TMem + BSize, std::max(TAlign, BAlign));
        const auto memPtr = mem.Get();
        T* obj = new (memPtr)T(std::forward<Args>(args)...);
        RefBlockT<T>* blk = new (memPtr + TMem)RefBlockT<T>(obj, memPtr);
        mem.Giveup();
        return blk;
    }
};


}

template<typename T>
class RefObject;
template<typename T, typename U>
T RefCast(const RefObject<U>& obj);

template<typename T>
class RefObject
{
    template<typename> friend class RefObject;
    template<typename X, typename Y> friend X RefCast(const RefObject<Y>&);
private:
    T* Pointer;
    detail::RefBlock* Control;
    constexpr RefObject(detail::RefBlockT<T>* control) noexcept : Pointer(control->Get()), Control(control)
    { }
    template<typename U>
    constexpr RefObject(detail::RefBlock* control, U* ptr) noexcept : Pointer(dynamic_cast<T*>(ptr)), Control(ptr ? control : nullptr)
    {
        detail::RefBlock::Increase(Control);
    }
protected:
    using RefType = RefObject<T>;
    T& Self() { return *Pointer; }
    const T& Self() const { return *Pointer; }
    template<typename... Args>
    static RefObject<T> Create(Args... args)
    {
        auto control = detail::RefBlockT<T>::CreateBlockTCombined(std::forward<Args>(args)...);
        return RefObject<T>(control);
    }
    template<typename U, typename... Args>
    RefObject<U> CreateWith(Args... args)
    {
        auto control = detail::RefBlockT<U>::CreateBlockTCombined(std::forward<Args>(args)...);
        control->AttachParent(Control);
        return RefObject<U>(control);
    }
public:
    using ObjectType = T;

    constexpr RefObject() noexcept : Pointer(nullptr), Control(nullptr) 
    { }
    RefObject(const RefObject<T>& other) noexcept : Pointer(other.Pointer), Control(other.Control)
    {
        detail::RefBlock::Increase(Control);
    }
    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    RefObject(const RefObject<U>& other) noexcept : Pointer(other.Pointer), Control(other.Control) 
    {
        detail::RefBlock::Increase(Control);
    }
    RefObject(RefObject<T>&& other) noexcept : Pointer(other.Pointer), Control(other.Control)
    {
        other.Pointer = nullptr;
        other.Control = nullptr;
    }
    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    constexpr RefObject(RefObject<U>&& other) noexcept : Pointer(other.Pointer), Control(other.Control)
    {
        other.Pointer = nullptr;
        other.Control = nullptr;
    }
    ~RefObject()
    {
        Release();
    }

    void Release() 
    {
        detail::RefBlock::Decreace(Control);
        Control = nullptr;
        Pointer = nullptr;
    }
    operator bool() const
    {
        return Control != nullptr;
    }

    RefObject<T>& operator=(const RefObject<T>& other) noexcept
    {
        detail::RefBlock::Increase(other.Control);
        detail::RefBlock::Decreace(Control);
        Pointer = other.Pointer;
        Control = other.Control;
        return *this;
    }
    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    RefObject<T>& operator=(const RefObject<U>& other) noexcept
    {
        detail::RefBlock::Increase(other.Control);
        detail::RefBlock::Decreace(Control);
        Pointer = other.Pointer;
        Control = other.Control;
        return *this;
    }
    RefObject<T>& operator=(RefObject<T>&& other) noexcept
    {
        std::swap(Control, other.Control);
        Pointer = other.Pointer;
        other.Release();
        return *this;
    }
    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    RefObject<T>& operator=(RefObject<U>&& other) noexcept
    {
        std::swap(Control, other.Control);
        Pointer = other.Pointer;
        other.Release();
        return *this;
    }
};

template<typename T, typename U>
T RefCast(const RefObject<U>& obj)
{
    using V = typename T::ObjectType;
    static_assert(std::is_base_of_v<RefObject<V>, T>, "Need to be cast into RefObject");
    return RefObject<V>(obj.Control, obj.Pointer);
}

#define INIT_REFOBJ(Name, Data)                                 \
Name(::common::RefObject<Data>&& other) :                       \
    ::common::RefObject<Data>(std::move(other)) { }             \
template<typename U, typename =                                 \
    std::enable_if_t<std::is_convertible_v<U*, Data*>>>         \
Name& operator=(const RefObject<U>& other) noexcept             \
{                                                               \
    *static_cast<RefObject<Data>*>(this) = other;               \
    return *this;                                               \
}                                                               \
template<typename U, typename =                                 \
    std::enable_if_t<std::is_convertible_v<U*, Data*>>>         \
Name& operator=(RefObject<U>&& other) noexcept                  \
{                                                               \
    *static_cast<RefObject<Data>*>(this) = std::move(other);    \
    return *this;                                               \
}                                                               \

}