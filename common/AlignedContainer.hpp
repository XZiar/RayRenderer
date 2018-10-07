#pragma once

#include "CommonRely.hpp"
#include <atomic>
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

struct AlignedBuffer
{
private:
    struct BufInfo
    {
        const size_t Size;
        std::atomic_uint32_t RefCount;
        const uint32_t Cookie = 0xabadcafe;
        BufInfo(const size_t size) noexcept : Size(size), RefCount(1) {}
    };
    BufInfo* CoreInfo = nullptr;
    std::byte* GetPtr() const noexcept
    {
        if (CoreInfo)
            return reinterpret_cast<std::byte*>(CoreInfo) - CoreInfo->Size;
        else
            return nullptr;
    }
    void CopyCore() const noexcept
    {
        if (CoreInfo)
            CoreInfo->RefCount++;
    }
    void ReleaseCore() const noexcept
    {
        if (CoreInfo)
        {
            if (--CoreInfo->RefCount == 0)
                free_align(GetPtr());
            else
                GetPtr();
        }
    }
protected:
    std::byte *Data = nullptr;
    size_t Size = 0;
    size_t Align = 0;
    void Alloc(const bool zero = false) noexcept
    {
        ReleaseCore();
        {
            const auto rawPtr = (std::uint8_t*)malloc_align(Size + sizeof(BufInfo), Align);
            CoreInfo = new (rawPtr + Size)BufInfo(Size);
        }
        Data = GetPtr();
        if (zero)
            memset(Data, 0x0, Size);
    }
    void Release() noexcept
    {
        ReleaseCore();
        Data = nullptr;
        Size = 0;
    };

    AlignedBuffer(BufInfo* coreInfo, std::byte *ptr, const size_t size, const size_t align = 64) noexcept
        : CoreInfo(coreInfo), Data(ptr), Size(size), Align(align) { CopyCore(); }
public:
    constexpr AlignedBuffer() noexcept : Align(64) { }
    AlignedBuffer(const size_t size, const size_t align = 64) noexcept : Size(size), Align(align)
    {
        Alloc();
    }
    AlignedBuffer(const size_t size, const std::byte fill, const size_t align = 64) noexcept : Size(size), Align(align)
    {
        Alloc();
        memset(Data, std::to_integer<uint8_t>(fill), Size);
    }
    AlignedBuffer(const AlignedBuffer& other) noexcept : Size(other.Size), Align(other.Align)
    {
        Alloc();
        memcpy_s(Data, Size, other.Data, Size);
    }
    AlignedBuffer(AlignedBuffer&& other) noexcept : CoreInfo(other.CoreInfo), Data(other.Data), Size(other.Size), Align(other.Align)
    {
        other.CoreInfo = nullptr; other.Data = nullptr; other.Size = 0;
    }
    ~AlignedBuffer() noexcept { ReleaseCore(); }

    AlignedBuffer& operator= (const AlignedBuffer& other) noexcept
    {
        if (other.Align != Align || other.Size != Size)
        {
            Size = other.Size;
            Align = other.Align;
            Alloc();
        }
        memcpy_s(Data, Size, other.Data, Size);
        return *this;
    }
    AlignedBuffer& operator= (AlignedBuffer&& other) noexcept
    {
        ReleaseCore();
        CoreInfo = other.CoreInfo; other.CoreInfo = nullptr;
        Data = other.Data; other.Data = nullptr;
        Size = other.Size; other.Size = 0;
        Align = other.Align;
        return *this;
    }
    std::byte& operator[](std::ptrdiff_t idx) noexcept { return Data[idx]; }
    const std::byte& operator[] (std::ptrdiff_t idx) const noexcept { return Data[idx]; }
    template<typename T = std::byte>
    T* GetRawPtr() noexcept { return reinterpret_cast<T*>(Data); }
    template<typename T = std::byte>
    const T* GetRawPtr() const noexcept { return reinterpret_cast<const T*>(Data); }
    size_t GetSize() const noexcept { return Size; }
    size_t GetAlignment() const noexcept { return Align; }
    AlignedBuffer CreateSubBuffer(const size_t offset, const size_t size) const
    {
        if (offset + size > Size)
            throw std::bad_alloc(); // sub buffer range overflow
        return AlignedBuffer(CoreInfo, Data + offset, size, std::gcd(offset + Align, Align));
    }
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

template<class T, class H = std::hash<T>, class E = std::equal_to<T>>
using hashsetEx = std::unordered_set<T, H, E, common::AlignAllocator<T>>;


}
