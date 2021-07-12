#pragma once
#include "SystemCommonRely.h"
#include <cstdlib>
#include <cstring>

namespace common
{

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class SYSCOMMONAPI CopyManager
{
public:
    using TBroadcast2 = void(uint16_t* dest, const uint16_t src, size_t count) noexcept;
    using TBroadcast4 = void(uint32_t* dest, const uint32_t src, size_t count) noexcept;
    using TZExtCopy12 = void(uint16_t* dest, const uint8_t* src, size_t count) noexcept;
    using TZExtCopy14 = void(uint32_t* dest, const uint8_t* src, size_t count) noexcept;
    using TZExtCopy24 = void(uint32_t* dest, const uint16_t* src, size_t count) noexcept;
    using TNarrowCopy21 = void(uint8_t* dest, const uint16_t* src, size_t count) noexcept;
    using TNarrowCopy41 = void(uint8_t* dest, const uint32_t* src, size_t count) noexcept;
    using TNarrowCopy42 = void(uint16_t* dest, const uint32_t* src, size_t count) noexcept;
    using TCvtI32F32 = void(float* dest, const int32_t* src, size_t count) noexcept;
    using TCvtI16F32 = void(float* dest, const int16_t* src, size_t count) noexcept;
    using TCvtI8F32  = void(float* dest, const int8_t* src, size_t count) noexcept;
private:
    using VarItem = std::pair<std::string_view, std::string_view>;
    TBroadcast2* Broadcast2 = nullptr;
    TBroadcast4* Broadcast4 = nullptr;
    TZExtCopy12* ZExtCopy12 = nullptr;
    TZExtCopy14* ZExtCopy14 = nullptr;
    TZExtCopy24* ZExtCopy24 = nullptr;
    TNarrowCopy21* NarrowCopy21 = nullptr;
    TNarrowCopy41* NarrowCopy41 = nullptr;
    TNarrowCopy42* NarrowCopy42 = nullptr;
    TCvtI32F32* CvtI32F32 = nullptr;
    TCvtI16F32* CvtI16F32 = nullptr;
    TCvtI8F32 * CvtI8F32  = nullptr;
    std::vector<VarItem> VariantMap;
public:
    [[nodiscard]] static common::span<const VarItem> GetSupportMap() noexcept;
    CopyManager(common::span<const VarItem> requests) noexcept;
    CopyManager() noexcept : CopyManager(GetSupportMap()) { }
    [[nodiscard]] common::span<const VarItem> GetIntrinMap() const noexcept
    {
        return VariantMap;
    }
    [[nodiscard]] bool IsComplete() const noexcept
    {
        return Broadcast2 && Broadcast4 && ZExtCopy12 && ZExtCopy14 && ZExtCopy24 && NarrowCopy21 && NarrowCopy41 && NarrowCopy42;
    }

    template<typename T>
    forceinline void BroadcastMany(T* const dest, const T& src, const size_t count) const noexcept
    {
        if constexpr (sizeof(T) == 1)
        {
            memset(dest, *reinterpret_cast<const uint8_t*>(&src), count);
        }
        else if constexpr (sizeof(T) == 2)
            Broadcast2(reinterpret_cast<uint16_t*>(dest), *reinterpret_cast<const uint16_t*>(&src), count);
        else if constexpr (sizeof(T) == 4)
            Broadcast4(reinterpret_cast<uint32_t*>(dest), *reinterpret_cast<const uint32_t*>(&src), count);
        else
        {
            for (size_t i = 0; i < count; ++i)
                *dest++ = src;
        }
    }
    template<typename T, typename U>
    forceinline void ZExtCopy(T* const dest, const U* src, const size_t count) const noexcept
    {
        constexpr size_t SizeT = sizeof(T), SizeU = sizeof(U);
        static_assert(SizeT >= SizeU);
        if constexpr (SizeT == SizeU)
        {
            memcpy_s(dest, count * SizeT, src, count * SizeU);
        }
        else if constexpr (SizeU == 1)
        {
            if constexpr (SizeT == 2)
                ZExtCopy12(reinterpret_cast<uint16_t*>(dest), reinterpret_cast<const uint8_t*>(src), count);
            else if constexpr (SizeT == 4)
                ZExtCopy14(reinterpret_cast<uint32_t*>(dest), reinterpret_cast<const uint8_t*>(src), count);
            else
                static_assert(AlwaysTrue<T>, "datatype casting not supported");
        }
        else if constexpr (SizeU == 2)
        {
            if constexpr (SizeT == 4)
                ZExtCopy24(reinterpret_cast<uint32_t*>(dest), reinterpret_cast<const uint16_t*>(src), count);
            else
                static_assert(AlwaysTrue<T>, "datatype casting not supported");
        }
        else
            static_assert(AlwaysTrue<T>, "datatype casting not supported");
    }
    template<typename T, typename U>
    forceinline void NarrowCopy(T* const dest, const U* src, const size_t count) const noexcept
    {
        constexpr size_t SizeT = sizeof(T), SizeU = sizeof(U);
        static_assert(SizeT <= SizeU);
        if constexpr (SizeT == SizeU)
        {
            memcpy_s(dest, count * SizeT, src, count * SizeU);
        }
        else if constexpr (SizeT == 1)
        {
            if constexpr (SizeU == 2)
                NarrowCopy21(reinterpret_cast<uint8_t*>(dest), reinterpret_cast<const uint16_t*>(src), count);
            else if constexpr (SizeU == 4)
                NarrowCopy41(reinterpret_cast<uint8_t*>(dest), reinterpret_cast<const uint32_t*>(src), count);
            else
                static_assert(AlwaysTrue<T>, "datatype casting not supported");
        }
        else if constexpr (SizeT == 2)
        {
            if constexpr (SizeU == 4)
                NarrowCopy42(reinterpret_cast<uint16_t*>(dest), reinterpret_cast<const uint32_t*>(src), count);
            else
                static_assert(AlwaysTrue<T>, "datatype casting not supported");
        }
        else
            static_assert(AlwaysTrue<T>, "datatype casting not supported");
    }
};

SYSCOMMONAPI extern const CopyManager CopyEx;


#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

}