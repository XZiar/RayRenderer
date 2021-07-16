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
    using TTruncCopy21 = void(uint8_t * dest, const uint16_t* src, size_t count) noexcept;
    using TTruncCopy41 = void(uint8_t * dest, const uint32_t* src, size_t count) noexcept;
    using TTruncCopy42 = void(uint16_t* dest, const uint32_t* src, size_t count) noexcept;
    using TCvtI32F32 = void(float* dest, const int32_t* src, size_t count, float mulVal) noexcept;
    using TCvtI16F32 = void(float* dest, const int16_t* src, size_t count, float mulVal) noexcept;
    using TCvtI8F32  = void(float* dest, const int8_t * src, size_t count, float mulVal) noexcept;
    using TCvtF32I32 = void(int32_t* dest, const float* src, size_t count, float mulVal, bool saturate) noexcept;
    using TCvtF32I16 = void(int16_t* dest, const float* src, size_t count, float mulVal, bool saturate) noexcept;
    using TCvtF32I8  = void(int8_t * dest, const float* src, size_t count, float mulVal, bool saturate) noexcept;
private:
    using VarItem = std::pair<std::string_view, std::string_view>;
    TBroadcast2* Broadcast2 = nullptr;
    TBroadcast4* Broadcast4 = nullptr;
    TZExtCopy12* ZExtCopy12 = nullptr;
    TZExtCopy14* ZExtCopy14 = nullptr;
    TZExtCopy24* ZExtCopy24 = nullptr;
    TTruncCopy21* TruncCopy21 = nullptr;
    TTruncCopy41* TruncCopy41 = nullptr;
    TTruncCopy42* TruncCopy42 = nullptr;
    TCvtI32F32* CvtI32F32 = nullptr;
    TCvtI16F32* CvtI16F32 = nullptr;
    TCvtI8F32 * CvtI8F32  = nullptr;
    TCvtF32I32* CvtF32I32 = nullptr;
    TCvtF32I16* CvtF32I16 = nullptr;
    TCvtF32I8 * CvtF32I8  = nullptr;
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
        return Broadcast2 && Broadcast4 && 
            ZExtCopy12 && ZExtCopy14 && ZExtCopy24 && 
            TruncCopy21 && TruncCopy41 && TruncCopy42 &&
            CvtI32F32 && CvtI16F32 && CvtI8F32;
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
    forceinline void TruncCopy(T* const dest, const U* src, const size_t count, [[maybe_unused]] const bool saturate = false) const noexcept
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
                TruncCopy21(reinterpret_cast<uint8_t*>(dest), reinterpret_cast<const uint16_t*>(src), count);
            else if constexpr (SizeU == 4)
                TruncCopy41(reinterpret_cast<uint8_t*>(dest), reinterpret_cast<const uint32_t*>(src), count);
            else
                static_assert(AlwaysTrue<T>, "datatype casting not supported");
        }
        else if constexpr (SizeT == 2)
        {
            if constexpr (SizeU == 4)
                TruncCopy42(reinterpret_cast<uint16_t*>(dest), reinterpret_cast<const uint32_t*>(src), count);
            else
                static_assert(AlwaysTrue<T>, "datatype casting not supported");
        }
        else
            static_assert(AlwaysTrue<T>, "datatype casting not supported");
    }
    template<typename T, typename U>
    forceinline void CopyToFloat(T* const dest, const U* src, const size_t count, const T range = 0) const noexcept
    {
        // TODO: negative is larger
        const T mulVal = range == 0 ? 0 : range / static_cast<T>(std::numeric_limits<U>::max());
        if constexpr (std::is_same_v<T, float>)
        {
            if constexpr (std::is_same_v<U, int32_t>)
                CvtI32F32(dest, src, count, mulVal);
            else if constexpr (std::is_same_v<U, int16_t>)
                CvtI16F32(dest, src, count, mulVal);
            else if constexpr (std::is_same_v<U, int8_t>)
                CvtI8F32(dest, src, count, mulVal);
            else
                static_assert(AlwaysTrue<T>, "datatype casting not supported");
        }
        else
            static_assert(AlwaysTrue<T>, "datatype casting not supported");
    }
    template<typename T, typename U>
    forceinline void CopyFromFloat(T* const dest, const U* src, const size_t count, const U range = 0, const bool saturate = false) const noexcept
    {
        // TODO: negative is larger
        const U mulVal = range == 0 ? 0 : static_cast<U>(std::numeric_limits<T>::max()) / range;
        if constexpr (std::is_same_v<U, float>)
        {
            if constexpr (std::is_same_v<T, int32_t>)
                CvtF32I32(dest, src, count, mulVal, saturate);
            else if constexpr (std::is_same_v<T, int16_t>)
                CvtF32I16(dest, src, count, mulVal, saturate);
            else if constexpr (std::is_same_v<T, int8_t>)
                CvtF32I8(dest, src, count, mulVal, saturate);
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