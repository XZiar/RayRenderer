#pragma once

#include "CommonRely.hpp"

#include <vector>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace common
{

template <class T>
struct AlignAllocator
{
    using value_type = T;
    static constexpr size_t Align = AlignBaseHelper<T>::Align;

    AlignAllocator() noexcept = default;
    template<class U>
    AlignAllocator(const AlignAllocator<U>&) noexcept { }

    template<class U>
    bool operator==(const AlignAllocator<U>&) const noexcept
    {
        return true;
    }
    template<class U>
    bool operator!=(const AlignAllocator<U>&) const noexcept
    {
        return false;
    }
    T* allocate(const size_t n) const
    {
        T *ptr = nullptr;
        if (n == 0)
            return ptr;
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_array_new_length();
        ptr = (T*)malloc_align(n * sizeof(T), Align);
        if (!ptr)
            throw std::bad_alloc();
        else
            return ptr;
    }
    void deallocate(T* const p, const size_t) const noexcept
    {
        free_align(p);
    }
};


template<size_t Align>
struct COMMONTPL AlignedBuffer
{
protected:
    std::byte *Data = nullptr;
    size_t Size_ = 0;
    void Alloc(const bool zero = false)
    {
        Release();
        Data = (std::byte*)malloc_align(Size_, Align);
        if (zero)
            memset(Data, 0x0, Size_);
    }
    void Release()
    {
        if (Data) free_align(Data);
        Data = nullptr;
    };
public:
    AlignedBuffer() noexcept { }
    AlignedBuffer(const size_t size) : Size_(size) { Alloc(); }
    AlignedBuffer(const size_t size, const std::byte fill) : Size_(size) 
    {
        Alloc();
        memset(Data, std::to_integer<uint8_t>(fill), Size_);
    }
    AlignedBuffer(const AlignedBuffer& other) : Size_(other.Size_)
    {
        Alloc();
        memcpy_s(Data, Size_, other.Data, Size_);
    }
    AlignedBuffer(AlignedBuffer&& other) noexcept : Size_(other.Size_), Data(other.Data)
    {
        other.Data = nullptr;
    }
    ~AlignedBuffer() { Release(); }

    AlignedBuffer& operator= (const AlignedBuffer& other)
    {
        Size_ = other.Size_;
        Alloc();
        memcpy_s(Data, Size_, other.Data, Size_);
        return *this;
    }
    AlignedBuffer& operator= (AlignedBuffer&& other) noexcept
    {
        Release();
        Size_ = other.Size_;
        Data = other.Data;
        other.Data = nullptr;
        return *this;
    }
    std::byte& operator[](std::ptrdiff_t idx) { return Data[idx]; }
    const std::byte& operator[] (std::ptrdiff_t idx) const { return Data[idx]; }
    template<typename T = std::byte>
    T* GetRawPtr() noexcept { return reinterpret_cast<T*>(Data); }
    template<typename T = std::byte>
    const T* GetRawPtr() const noexcept { return reinterpret_cast<const T*>(Data); }
    size_t GetSize() const noexcept { return Size_; }
};


template<class T>
class vectorEx : public std::vector<T, common::AlignAllocator<T>>
{
public:
    using std::vector<T, common::AlignAllocator<T>>::vector;
    operator const std::vector<T>&() const
    {
        return *(const std::vector<T>*)this;
    }
};


template<class T>
using dequeEx = std::deque<T, common::AlignAllocator<T>>;

template<class K, class V, class C = std::less<K>>
using mapEx = std::map<K, V, C, common::AlignAllocator<std::pair<const K, V>>>;

template<class T, class C = std::less<T>>
using setEx = std::set<T, C, common::AlignAllocator<T>>;

template<class K, class V, class H = std::hash<K>, class E = std::equal_to<K>>
using hashmapEx = std::unordered_map<K, H, E, V, common::AlignAllocator<std::pair<const K, V>>>;

template<class T, class H = std::hash<T>, class E = std::equal_to<K>>
using hashsetEx = std::unordered_set<T, H, E, common::AlignAllocator<T>>;


}
