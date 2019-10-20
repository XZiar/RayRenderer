#pragma once

#include "CommonRely.hpp"
#include <atomic>

namespace common
{


class AlignedBuffer
{
    friend struct AlignBufLessor;
public:
    struct ExternBufInfo
    {
        virtual ~ExternBufInfo() = 0;
        [[nodiscard]] virtual size_t GetSize() const noexcept = 0;
        [[nodiscard]] virtual std::byte* GetPtr() const noexcept = 0;
    };
    class BufInfo
    {
        friend class AlignedBuffer;
    private:
        const ExternBufInfo* ExternInfo = nullptr;
        size_t Size;
        mutable std::atomic_uint32_t RefCount;
        uint32_t Cookie;
        BufInfo(const size_t size) noexcept : ExternInfo(nullptr), Size(size),
            RefCount(1), Cookie(0xabadcafe) { }
        BufInfo(std::unique_ptr<const ExternBufInfo>&& externInfo) noexcept : ExternInfo(externInfo.release()), Size(ExternInfo->GetSize()),
            RefCount(1), Cookie(0xdeadbeef) { }
    public:
        [[nodiscard]] static std::byte* GetPtr(const BufInfo* info) noexcept
        {
            if (info)
            {
                if (info->ExternInfo)
                    return info->ExternInfo->GetPtr();
                else
                    return reinterpret_cast<std::byte*>(reinterpret_cast<uintptr_t>(info) - info->Size);
            }
            else
                return nullptr;
        }
        static void AddReference(const BufInfo* info) noexcept
        {
            if (info)
                info->RefCount++;
        }
        static bool ReduceReference(const BufInfo*& info) noexcept
        {
            if (info && info->RefCount-- == 1)
            {
                if (info->ExternInfo)
                {
                    delete info->ExternInfo;
                    delete info;
                }
                else
                {
                    free_align(GetPtr(info));
                }
                info = nullptr;
                return true;
            }
            return false;
        }
        [[nodiscard]] static std::byte* AllocNew(const size_t size, const size_t align, const BufInfo*& info) noexcept
        {
            Ensures(IsPower2(align));
            ReduceReference(info);
            if (size != 0)
            {
                constexpr size_t InfoAlign = alignof(BufInfo);
                const size_t realSize = (size + InfoAlign - 1) / InfoAlign * InfoAlign;
                const auto rawPtr = (std::byte*)malloc_align(realSize + sizeof(BufInfo), align);
                if (rawPtr != nullptr)
                {
                    info = new (rawPtr + realSize)BufInfo(realSize);
                    return GetPtr(info);
                }
            }
            return nullptr;
        }
    };
private:
    const BufInfo* CoreInfo;
    AlignedBuffer(const BufInfo* coreInfo, std::byte* ptr, const size_t size, const size_t align) noexcept
        : CoreInfo(coreInfo), Size(size), Align(align), Data(ptr)
    {
        if (CoreInfo)
            CoreInfo->RefCount++;
    }
protected:
    size_t Size = 0;
    size_t Align = 0;
    std::byte* Data = nullptr;
    void Fill(const std::byte fill = std::byte{ 0x0 }) noexcept
    {
        if (Data)
            memset(Data, std::to_integer<uint8_t>(fill), Size);
    }
    void Release() noexcept
    {
        BufInfo::ReduceReference(CoreInfo);
        Data = nullptr;
        Size = 0;
    };
    void ReAlloc(const size_t size, size_t align) noexcept
    {
        Data = BufInfo::AllocNew(size, align, CoreInfo);
        Size = size; Align = align;
    }
public:
    constexpr AlignedBuffer() noexcept : CoreInfo(nullptr), Size(0), Align(64), Data(nullptr) { }
    AlignedBuffer(const size_t size, const size_t align = 64) noexcept : 
        CoreInfo(nullptr), Size(size), Align(align), Data(BufInfo::AllocNew(Size, Align, CoreInfo))
    { }
    AlignedBuffer(const size_t size, const std::byte fill, const size_t align = 64) noexcept : 
        AlignedBuffer(size, align)
    {
        Fill(fill);
    }
    AlignedBuffer(const AlignedBuffer& other) noexcept : 
        AlignedBuffer(other.Size, other.Align)
    {
        if (Data)
            memcpy_s(Data, Size, other.Data, Size);
    }
    constexpr AlignedBuffer(AlignedBuffer&& other) noexcept : 
        CoreInfo(other.CoreInfo), Size(other.Size), Align(other.Align), Data(other.Data)
    {
        other.CoreInfo = nullptr; other.Data = nullptr; other.Size = 0;
    }
    ~AlignedBuffer() noexcept { Release(); }

    AlignedBuffer& operator= (const AlignedBuffer& other) noexcept
    {
        if (other.Align != Align || other.Size != Size)
        {
            auto align = other.Align;
            Data = BufInfo::AllocNew(other.Size, align, CoreInfo);
            Align = align;
        }
        if (Data)
            memcpy_s(Data, Size, other.Data, Size);
        return *this;
    }
    AlignedBuffer& operator= (AlignedBuffer&& other) noexcept
    {
        if (this != &other)
        {
            Release();
            CoreInfo = other.CoreInfo; other.CoreInfo = nullptr;
            Data = other.Data; other.Data = nullptr;
            Size = other.Size; other.Size = 0;
            Align = other.Align;
        }
        return *this;
    }

    [[nodiscard]] constexpr size_t GetSize()      const noexcept { return Size; }
    [[nodiscard]] constexpr size_t GetAlignment() const noexcept { return Align; }
    
    [[nodiscard]] constexpr       std::byte& operator[] (std::ptrdiff_t idx)       noexcept { return Data[idx]; }
    [[nodiscard]] constexpr const std::byte& operator[] (std::ptrdiff_t idx) const noexcept { return Data[idx]; }
    template<typename T = std::byte>
    [[nodiscard]] constexpr       T* GetRawPtr()         noexcept { return reinterpret_cast<T*>(Data); }
    template<typename T = std::byte>
    [[nodiscard]] constexpr const T* GetRawPtr()   const noexcept { return reinterpret_cast<const T*>(Data); }
    template<typename T = std::byte>
    [[nodiscard]] constexpr span<      T> AsSpan()       noexcept { return span<T>(GetRawPtr<T>(), Size / sizeof(T)); }
    template<typename T = std::byte>
    [[nodiscard]] constexpr span<const T> AsSpan() const noexcept { return span<T>(GetRawPtr<T>(), Size / sizeof(T)); }

    [[nodiscard]] AlignedBuffer CreateSubBuffer(const size_t offset = 0, size_t size = SIZE_MAX) const
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
