#pragma once

#include "CommonRely.hpp"
#include <atomic>

namespace common
{

struct AlignedBuffer
{
    friend struct AlignBufLessor;
private:
    struct BufInfo
    {
        const size_t Size;
        std::atomic_uint32_t RefCount;
        const uint32_t Cookie = 0xabadcafe;
        BufInfo(const size_t size) noexcept : Size(size), RefCount(1) {}
    };
    BufInfo* CoreInfo = nullptr;
    constexpr std::byte* GetPtr() const noexcept
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
        if (Data)
            memset(Data, std::to_integer<uint8_t>(fill), Size);
    }
    AlignedBuffer(const AlignedBuffer& other) noexcept : Size(other.Size), Align(other.Align)
    {
        Alloc();
        if (Data)
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
        if (this != &other)
        {
            ReleaseCore();
            CoreInfo = other.CoreInfo; other.CoreInfo = nullptr;
            Data = other.Data; other.Data = nullptr;
            Size = other.Size; other.Size = 0;
            Align = other.Align;
        }
        return *this;
    }
    std::byte& operator[](std::ptrdiff_t idx) noexcept { return Data[idx]; }
    const std::byte& operator[] (std::ptrdiff_t idx) const noexcept { return Data[idx]; }
    template<typename T = std::byte>
    constexpr T* GetRawPtr() noexcept { return reinterpret_cast<T*>(Data); }
    template<typename T = std::byte>
    constexpr const T* GetRawPtr() const noexcept { return reinterpret_cast<const T*>(Data); }
    constexpr size_t GetSize() const noexcept { return Size; }
    constexpr size_t GetAlignment() const noexcept { return Align; }
    AlignedBuffer CreateSubBuffer(const size_t offset = 0, size_t size = SIZE_MAX) const
    {
        if (size == SIZE_MAX)
        {
            if (offset >= Size)
                throw std::bad_alloc(); // sub buffer offset overflow
            size = Size - offset;
        }
        else if (offset + size > Size)
            throw std::bad_alloc(); // sub buffer range overflow
        return AlignedBuffer(CoreInfo, Data + offset, size, std::gcd(offset + Align, Align));
    }
    constexpr bool operator==(const AlignedBuffer& other) const noexcept
    {
        return Data == other.Data && Size == other.Size;
    }
};


struct AlignBufLessor
{
    template<typename T>
    constexpr bool operator()(const T& left, const T& right) const noexcept
    {
        return left.Data == right.Data ? left.Size < right.Size : left.Data < right.Data;
    }
};

}
