#pragma once
#include "SystemCommonRely.h"
#include <cstdlib>
#include <cstring>

namespace common
{


class CopyManager final : public RuntimeFastPath<CopyManager>
{
    friend ::common::fastpath::PathHack;
private:
    void(*Broadcast2 )(uint16_t* dest, const uint16_t src, size_t count) noexcept = nullptr;
    void(*Broadcast4 )(uint32_t* dest, const uint32_t src, size_t count) noexcept = nullptr;
    void(*ZExtCopy12 )(uint16_t* dest, const uint8_t* src, size_t count) noexcept = nullptr;
    void(*ZExtCopy14 )(uint32_t* dest, const uint8_t* src, size_t count) noexcept = nullptr;
    void(*ZExtCopy24 )(uint32_t* dest, const uint16_t* src, size_t count) noexcept = nullptr;
    void(*ZExtCopy28 )(uint64_t* dest, const uint16_t* src, size_t count) noexcept = nullptr;
    void(*ZExtCopy48 )(uint64_t* dest, const uint32_t* src, size_t count) noexcept = nullptr;
    void(*SExtCopy12 )(int16_t* dest, const int8_t* src, size_t count) noexcept = nullptr;
    void(*SExtCopy14 )(int32_t* dest, const int8_t* src, size_t count) noexcept = nullptr;
    void(*SExtCopy24 )(int32_t* dest, const int16_t* src, size_t count) noexcept = nullptr;
    void(*SExtCopy28 )(int64_t* dest, const int16_t* src, size_t count) noexcept = nullptr;
    void(*SExtCopy48 )(int64_t* dest, const int32_t* src, size_t count) noexcept = nullptr;
    void(*TruncCopy21)(uint8_t * dest, const uint16_t* src, size_t count) noexcept = nullptr;
    void(*TruncCopy41)(uint8_t * dest, const uint32_t* src, size_t count) noexcept = nullptr;
    void(*TruncCopy42)(uint16_t* dest, const uint32_t* src, size_t count) noexcept = nullptr;
    void(*TruncCopy81)(uint8_t * dest, const uint64_t* src, size_t count) noexcept = nullptr;
    void(*TruncCopy82)(uint16_t* dest, const uint64_t* src, size_t count) noexcept = nullptr;
    void(*TruncCopy84)(uint32_t* dest, const uint64_t* src, size_t count) noexcept = nullptr;
    void(*CvtI32F32  )(float* dest, const int32_t* src, size_t count, float mulVal) noexcept = nullptr;
    void(*CvtI16F32  )(float* dest, const int16_t* src, size_t count, float mulVal) noexcept = nullptr;
    void(*CvtI8F32   )(float* dest, const int8_t * src, size_t count, float mulVal) noexcept = nullptr;
    void(*CvtU32F32  )(float* dest, const uint32_t* src, size_t count, float mulVal) noexcept = nullptr;
    void(*CvtU16F32  )(float* dest, const uint16_t* src, size_t count, float mulVal) noexcept = nullptr;
    void(*CvtU8F32   )(float* dest, const uint8_t * src, size_t count, float mulVal) noexcept = nullptr;
    void(*CvtF32I32  )(int32_t* dest, const float* src, size_t count, float mulVal, bool saturate) noexcept = nullptr;
    void(*CvtF32I16  )(int16_t* dest, const float* src, size_t count, float mulVal, bool saturate) noexcept = nullptr;
    void(*CvtF32I8   )(int8_t * dest, const float* src, size_t count, float mulVal, bool saturate) noexcept = nullptr;
    void(*CvtF32U16  )(uint16_t* dest, const float* src, size_t count, float mulVal, bool saturate) noexcept = nullptr;
    void(*CvtF32U8   )(uint8_t * dest, const float* src, size_t count, float mulVal, bool saturate) noexcept = nullptr;
public:
    SYSCOMMONAPI [[nodiscard]] static common::span<const PathInfo> GetSupportMap() noexcept;
    SYSCOMMONAPI CopyManager(common::span<const VarItem> requests = {}) noexcept;
    SYSCOMMONAPI ~CopyManager();
    SYSCOMMONAPI [[nodiscard]] bool IsComplete() const noexcept final;

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
                static_assert(!AlwaysTrue<T>, "datatype casting not supported");
        }
        else if constexpr (SizeU == 2)
        {
            if constexpr (SizeT == 4)
                ZExtCopy24(reinterpret_cast<uint32_t*>(dest), reinterpret_cast<const uint16_t*>(src), count);
            else if constexpr (SizeT == 8)
                ZExtCopy28(reinterpret_cast<uint64_t*>(dest), reinterpret_cast<const uint16_t*>(src), count);
            else
                static_assert(!AlwaysTrue<T>, "datatype casting not supported");
        }
        else if constexpr (SizeU == 4)
        {
            if constexpr (SizeT == 8)
                ZExtCopy48(reinterpret_cast<uint64_t*>(dest), reinterpret_cast<const uint32_t*>(src), count);
            else
                static_assert(!AlwaysTrue<T>, "datatype casting not supported");
        }
        else
            static_assert(!AlwaysTrue<T>, "datatype casting not supported");
    }
    template<typename T, typename U>
    forceinline void SExtCopy(T* const dest, const U* src, const size_t count) const noexcept
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
                SExtCopy12(reinterpret_cast<int16_t*>(dest), reinterpret_cast<const int8_t*>(src), count);
            else if constexpr (SizeT == 4)
                SExtCopy14(reinterpret_cast<int32_t*>(dest), reinterpret_cast<const int8_t*>(src), count);
            else
                static_assert(!AlwaysTrue<T>, "datatype casting not supported");
        }
        else if constexpr (SizeU == 2)
        {
            if constexpr (SizeT == 4)
                SExtCopy24(reinterpret_cast<int32_t*>(dest), reinterpret_cast<const int16_t*>(src), count);
            else if constexpr (SizeT == 8)
                SExtCopy28(reinterpret_cast<int64_t*>(dest), reinterpret_cast<const int16_t*>(src), count);
            else
                static_assert(!AlwaysTrue<T>, "datatype casting not supported");
        }
        else if constexpr (SizeU == 4)
        {
            if constexpr (SizeT == 8)
                SExtCopy48(reinterpret_cast<int64_t*>(dest), reinterpret_cast<const int32_t*>(src), count);
            else
                static_assert(!AlwaysTrue<T>, "datatype casting not supported");
        }
        else
            static_assert(!AlwaysTrue<T>, "datatype casting not supported");
    }
    template<typename T, typename U>
    forceinline void TruncCopy(T* const dest, const U* src, const size_t count) const noexcept
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
            else if constexpr (SizeU == 8)
                TruncCopy81(reinterpret_cast<uint8_t*>(dest), reinterpret_cast<const uint64_t*>(src), count);
            else
                static_assert(!AlwaysTrue<T>, "datatype casting not supported");
        }
        else if constexpr (SizeT == 2)
        {
            if constexpr (SizeU == 4)
                TruncCopy42(reinterpret_cast<uint16_t*>(dest), reinterpret_cast<const uint32_t*>(src), count);
            else if constexpr (SizeU == 8)
                TruncCopy82(reinterpret_cast<uint16_t*>(dest), reinterpret_cast<const uint64_t*>(src), count);
            else
                static_assert(!AlwaysTrue<T>, "datatype casting not supported");
        }
        else if constexpr (SizeT == 4)
        {
            if constexpr (SizeU == 8)
                TruncCopy84(reinterpret_cast<uint32_t*>(dest), reinterpret_cast<const uint64_t*>(src), count);
            else
                static_assert(!AlwaysTrue<T>, "datatype casting not supported");
        }
        else
            static_assert(!AlwaysTrue<T>, "datatype casting not supported");
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
            else if constexpr (std::is_same_v<U, uint32_t>)
                CvtU32F32(dest, src, count, mulVal);
            else if constexpr (std::is_same_v<U, uint16_t>)
                CvtU16F32(dest, src, count, mulVal);
            else if constexpr (std::is_same_v<U, uint8_t>)
                CvtU8F32(dest, src, count, mulVal);
            else
                static_assert(!AlwaysTrue<T>, "datatype casting not supported");
        }
        else
            static_assert(!AlwaysTrue<T>, "datatype casting not supported");
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
            else if constexpr (std::is_same_v<T, uint16_t>)
                CvtF32U16(dest, src, count, mulVal, saturate);
            else if constexpr (std::is_same_v<T, uint8_t>)
                CvtF32U8(dest, src, count, mulVal, saturate);
            else
                static_assert(!AlwaysTrue<T>, "datatype casting not supported");
        }
        else
            static_assert(!AlwaysTrue<T>, "datatype casting not supported");
    }
};

SYSCOMMONAPI extern const CopyManager CopyEx;


}