#pragma once
#include "CommonRely.hpp"
#include "SIMD.hpp"
#include "Exceptions.hpp"
#include <cstdint>
#include <cstring>

namespace common
{
namespace copy
{


namespace detail
{
template<size_t Src, size_t Dest>
inline void CopyLittleEndian(void* const dest, const void* const src, size_t count);

template<>
inline void CopyLittleEndian<4, 1>(void* const dest, const void* const src, size_t count)
{
    uint8_t * __restrict destPtr = reinterpret_cast<uint8_t*>(dest);
    const uint32_t * __restrict srcPtr = reinterpret_cast<const uint32_t*>(src);
#if COMMON_SIMD_LV >= 200
    const auto SHUF_MSK1 = _mm256_setr_epi8(0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto SHUF_MSK2 = _mm256_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12);
    while (count > 64)
    {
        const auto src0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr));
        const auto src1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 8));
        const auto src2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 16));
        const auto src3 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 24));
        const auto src4 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 32));
        const auto src5 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 40));
        const auto src6 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 48));
        const auto src7 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 56));
        const auto middle0 = _mm256_shuffle_epi8(src0, SHUF_MSK1);//a000,0b00
        const auto middle1 = _mm256_shuffle_epi8(src1, SHUF_MSK2);//00c0,000d
        const auto middle2 = _mm256_shuffle_epi8(src2, SHUF_MSK1);//e000,0f00
        const auto middle3 = _mm256_shuffle_epi8(src3, SHUF_MSK2);//00g0,000h
        const auto middle4 = _mm256_shuffle_epi8(src4, SHUF_MSK1);//a000,0b00
        const auto middle5 = _mm256_shuffle_epi8(src5, SHUF_MSK2);//00c0,000d
        const auto middle6 = _mm256_shuffle_epi8(src6, SHUF_MSK1);//e000,0f00
        const auto middle7 = _mm256_shuffle_epi8(src7, SHUF_MSK2);//00g0,000h
        const auto acbd0 = _mm256_blend_epi32(middle0, middle1, 0b11001100);//a0c0,0b0d
        const auto egfh0 = _mm256_blend_epi32(middle2, middle3, 0b11001100);//e0g0,0f0h
        const auto acbd1 = _mm256_blend_epi32(middle4, middle5, 0b11001100);//a0c0,0b0d
        const auto egfh1 = _mm256_blend_epi32(middle6, middle7, 0b11001100);//e0g0,0f0h
        const auto aceg0 = _mm256_permute2x128_si256(acbd0, egfh0, 0x20);//a0c0,e0g0
        const auto bdfh0 = _mm256_permute2x128_si256(acbd0, egfh0, 0x31);//0b0d,0f0h
        const auto aceg1 = _mm256_permute2x128_si256(acbd1, egfh1, 0x20);//a0c0,e0g0
        const auto bdfh1 = _mm256_permute2x128_si256(acbd1, egfh1, 0x31);//0b0d,0f0h
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(destPtr), _mm256_blend_epi32(aceg0, bdfh0, 0b10101010));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(destPtr + 32), _mm256_blend_epi32(aceg1, bdfh1, 0b10101010));
        srcPtr += 64; destPtr += 64; count -= 64;
    }
#elif COMMON_SIMD_LV >= 31
    const auto SHUF_MSK = _mm_setr_epi8(0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    while (count > 32)
    {
        const auto src0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr));
        const auto src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 4));
        const auto src2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 8));
        const auto src3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 12));
        const auto src4 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 16));
        const auto src5 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 20));
        const auto src6 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 24));
        const auto src7 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 28));
        const auto middle0 = _mm_shuffle_epi8(src0, SHUF_MSK);//a000
        const auto middle1 = _mm_shuffle_epi8(src1, SHUF_MSK);//b000
        const auto middle2 = _mm_shuffle_epi8(src2, SHUF_MSK);//c000
        const auto middle3 = _mm_shuffle_epi8(src3, SHUF_MSK);//d000
        const auto middle4 = _mm_shuffle_epi8(src4, SHUF_MSK);//e000
        const auto middle5 = _mm_shuffle_epi8(src5, SHUF_MSK);//f000
        const auto middle6 = _mm_shuffle_epi8(src6, SHUF_MSK);//g000
        const auto middle7 = _mm_shuffle_epi8(src7, SHUF_MSK);//h000
        const auto ac00 = _mm_unpacklo_epi32(middle0, middle2);//ac00
        const auto bd00 = _mm_unpacklo_epi32(middle1, middle3);//bd00
        const auto eg00 = _mm_unpacklo_epi32(middle4, middle6);//eg00
        const auto fh00 = _mm_unpacklo_epi32(middle5, middle7);//fh00
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr), _mm_unpacklo_epi32(ac00, bd00));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr + 16), _mm_unpacklo_epi32(eg00, fh00));
        srcPtr += 32; destPtr += 32; count -= 32;
    }
#endif
    while (count > 0)
    {
        switch (count)
        {
        default: *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 7:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 6:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 5:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 4:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 3:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 2:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 1:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        }
    }
}

template<>
inline void CopyLittleEndian<4, 2>(void* const dest, const void* const src, size_t count)
{
    uint16_t * __restrict destPtr = reinterpret_cast<uint16_t*>(dest);
    const uint32_t * __restrict srcPtr = reinterpret_cast<const uint32_t*>(src);
#if COMMON_SIMD_LV >= 200
    const auto SHUF_MSK = _mm256_setr_epi8(0, 1, 4, 5, 8, 9, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 4, 5, 8, 9, 12, 13);
    while (count >= 64)
    {
        const auto src0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr));
        const auto src1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 8));
        const auto src2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 16));
        const auto src3 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 24));
        const auto src4 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 32));
        const auto src5 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 40));
        const auto src6 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 48));
        const auto src7 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 56));
        const auto middle0 = _mm256_shuffle_epi8(src0, SHUF_MSK);//a0,0b
        const auto middle1 = _mm256_shuffle_epi8(src1, SHUF_MSK);//c0,0d
        const auto middle2 = _mm256_shuffle_epi8(src2, SHUF_MSK);//e0,0f
        const auto middle3 = _mm256_shuffle_epi8(src3, SHUF_MSK);//g0,0h
        const auto middle4 = _mm256_shuffle_epi8(src4, SHUF_MSK);//a0,0b
        const auto middle5 = _mm256_shuffle_epi8(src5, SHUF_MSK);//c0,0d
        const auto middle6 = _mm256_shuffle_epi8(src6, SHUF_MSK);//e0,0f
        const auto middle7 = _mm256_shuffle_epi8(src7, SHUF_MSK);//g0,0h
        const auto abab0 = _mm256_permute4x64_epi64(middle0, 0b11001100);
        const auto cdcd0 = _mm256_permute4x64_epi64(middle1, 0b11001100);
        const auto efef0 = _mm256_permute4x64_epi64(middle2, 0b11001100);
        const auto ghgh0 = _mm256_permute4x64_epi64(middle3, 0b11001100);
        const auto abab1 = _mm256_permute4x64_epi64(middle4, 0b11001100);
        const auto cdcd1 = _mm256_permute4x64_epi64(middle5, 0b11001100);
        const auto efef1 = _mm256_permute4x64_epi64(middle6, 0b11001100);
        const auto ghgh1 = _mm256_permute4x64_epi64(middle7, 0b11001100);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(destPtr), _mm256_blend_epi32(abab0, cdcd0, 0b11110000));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(destPtr + 16), _mm256_blend_epi32(efef0, ghgh0, 0b11110000));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(destPtr + 32), _mm256_blend_epi32(abab1, cdcd1, 0b11110000));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(destPtr + 48), _mm256_blend_epi32(efef1, ghgh1, 0b11110000));
        srcPtr += 64; destPtr += 64; count -= 64;
    }
#elif COMMON_SIMD_LV >= 31
    const auto SHUF_MSK = _mm_setr_epi8(0, 1, 4, 5, 8, 9, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1);
    while (count >= 32)
    {
        const auto src0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr));
        const auto src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 4));
        const auto src2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 8));
        const auto src3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 12));
        const auto src4 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 16));
        const auto src5 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 20));
        const auto src6 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 24));
        const auto src7 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 28));
        const auto middle0 = _mm_shuffle_epi8(src0, SHUF_MSK);//a0
        const auto middle1 = _mm_shuffle_epi8(src1, SHUF_MSK);//b0
        const auto middle2 = _mm_shuffle_epi8(src2, SHUF_MSK);//c0
        const auto middle3 = _mm_shuffle_epi8(src3, SHUF_MSK);//d0
        const auto middle4 = _mm_shuffle_epi8(src4, SHUF_MSK);//e0
        const auto middle5 = _mm_shuffle_epi8(src5, SHUF_MSK);//f0
        const auto middle6 = _mm_shuffle_epi8(src6, SHUF_MSK);//g0
        const auto middle7 = _mm_shuffle_epi8(src7, SHUF_MSK);//h0
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr), _mm_unpacklo_epi64(middle0, middle1));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr + 8), _mm_unpacklo_epi64(middle2, middle3));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr + 16), _mm_unpacklo_epi64(middle4, middle5));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr + 24), _mm_unpacklo_epi64(middle6, middle7));
        srcPtr += 32; destPtr += 32; count -= 32;
    }
#endif
    while (count > 0)
    {
        switch (count)
        {
        default: *destPtr++ = (uint16_t)*srcPtr++; count--;
        case 7:  *destPtr++ = (uint16_t)*srcPtr++; count--;
        case 6:  *destPtr++ = (uint16_t)*srcPtr++; count--;
        case 5:  *destPtr++ = (uint16_t)*srcPtr++; count--;
        case 4:  *destPtr++ = (uint16_t)*srcPtr++; count--;
        case 3:  *destPtr++ = (uint16_t)*srcPtr++; count--;
        case 2:  *destPtr++ = (uint16_t)*srcPtr++; count--;
        case 1:  *destPtr++ = (uint16_t)*srcPtr++; count--;
        }
    }
}

template<>
inline void CopyLittleEndian<2, 1>(void* const dest, const void* const src, size_t count)
{
    uint8_t * __restrict destPtr = reinterpret_cast<uint8_t*>(dest);
    const uint16_t * __restrict srcPtr = reinterpret_cast<const uint16_t*>(src);
#if COMMON_SIMD_LV >= 200
    const auto SHUF_MSK = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 4, 6, 8, 10, 12, 14);
    while (count >= 64)
    {
        const auto src0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr));
        const auto src1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 16));
        const auto src2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 32));
        const auto src3 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(srcPtr + 48));
        const auto middle0 = _mm256_shuffle_epi8(src0, SHUF_MSK);//a0,0b
        const auto middle1 = _mm256_shuffle_epi8(src1, SHUF_MSK);//c0,0d
        const auto middle2 = _mm256_shuffle_epi8(src2, SHUF_MSK);//e0,0f
        const auto middle3 = _mm256_shuffle_epi8(src3, SHUF_MSK);//g0,0h
        const auto abab = _mm256_permute4x64_epi64(middle0, 0b11001100);
        const auto cdcd = _mm256_permute4x64_epi64(middle1, 0b11001100);
        const auto efef = _mm256_permute4x64_epi64(middle2, 0b11001100);
        const auto ghgh = _mm256_permute4x64_epi64(middle3, 0b11001100);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(destPtr), _mm256_blend_epi32(abab, cdcd, 0b11110000));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(destPtr + 32), _mm256_blend_epi32(efef, ghgh, 0b11110000));
        srcPtr += 64; destPtr += 64; count -= 64;
    }
#elif COMMON_SIMD_LV >= 31
    const auto SHUF_MSK = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1);
    while (count >= 64)
    {
        const auto src0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr));
        const auto src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 8));
        const auto src2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 16));
        const auto src3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 24));
        const auto src4 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 32));
        const auto src5 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 40));
        const auto src6 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 48));
        const auto src7 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcPtr + 56));
        const auto middle0 = _mm_shuffle_epi8(src0, SHUF_MSK);//a0
        const auto middle1 = _mm_shuffle_epi8(src1, SHUF_MSK);//b0
        const auto middle2 = _mm_shuffle_epi8(src2, SHUF_MSK);//c0
        const auto middle3 = _mm_shuffle_epi8(src3, SHUF_MSK);//d0
        const auto middle4 = _mm_shuffle_epi8(src4, SHUF_MSK);//e0
        const auto middle5 = _mm_shuffle_epi8(src5, SHUF_MSK);//f0
        const auto middle6 = _mm_shuffle_epi8(src6, SHUF_MSK);//g0
        const auto middle7 = _mm_shuffle_epi8(src7, SHUF_MSK);//h0
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr), _mm_unpacklo_epi64(middle0, middle1));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr + 16), _mm_unpacklo_epi64(middle2, middle3));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr + 32), _mm_unpacklo_epi64(middle4, middle5));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr + 48), _mm_unpacklo_epi64(middle6, middle7));
        srcPtr += 64; destPtr += 64; count -= 64;
    }
#endif
    while (count > 0)
    {
        switch (count)
        {
        default: *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 7:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 6:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 5:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 4:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 3:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 2:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        case 1:  *destPtr++ = (uint8_t)*srcPtr++; count--;
        }
    }
}
}
template<typename Src, typename Dest>
inline void CopyLittleEndian(Dest* const dest, const size_t destCount, const Src* const src, const size_t srcCount)
{
    if (destCount < srcCount)
        COMMON_THROW(BaseException, u"space avaliable on Dest is smaller than space required by Src.");
    if constexpr(sizeof(Src) == sizeof(Dest))
        memcpy_s(dest, destCount * sizeof(Dest), src, srcCount * sizeof(Src));
    else
        detail::CopyLittleEndian<sizeof(Src), sizeof(Dest)>(dest, src, srcCount);
}


namespace detail
{
inline void BroadcastMany(void* const dest, const uint8_t src, size_t count)
{
    uint8_t * __restrict destPtr = reinterpret_cast<uint8_t*>(dest);
#if COMMON_SIMD_LV >= 100
    const auto dat = _mm256_set1_epi8(src);
    while (count > 32)
    {
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(destPtr), dat);
        destPtr += 32; count -= 32;
    }
#elif COMMON_SIMD_LV >= 20
    const auto dat = _mm_set1_epi8(src);
    while (count > 16)
    {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr), dat);
        destPtr += 16; count -= 16;
    }
#else
    const uint64_t dat = src * 0x0101010101010101u;
    while (count > 8)
    {
        *reinterpret_cast<uint64_t*>(destPtr) = dat;
        destPtr += 8; count -= 8;
    }
#endif
    while (count > 0)
    {
        *destPtr++ = src;
        count--;
    }
}
inline void BroadcastMany(void* const dest, const uint16_t src, size_t count)
{
    uint16_t * __restrict destPtr = reinterpret_cast<uint16_t*>(dest);
#if COMMON_SIMD_LV >= 100
    const auto dat = _mm256_set1_epi16(src);
    while (count > 16)
    {
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(destPtr), dat);
        destPtr += 16; count -= 16;
    }
#elif COMMON_SIMD_LV >= 20
    const auto dat = _mm_set1_epi16(src);
    while (count > 8)
    {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr), dat);
        destPtr += 8; count -= 8;
    }
#else
    const uint64_t dat = src * 0x0001000100010001u;
    while (count > 4)
    {
        *reinterpret_cast<uint64_t*>(destPtr) = dat;
        destPtr += 4; count -= 4;
    }
#endif
    while (count > 0)
    {
        *destPtr++ = src;
        count--;
    }
}
inline void BroadcastMany(void* const dest, const uint32_t src, size_t count)
{
    uint32_t * __restrict destPtr = reinterpret_cast<uint32_t*>(dest);
#if COMMON_SIMD_LV >= 100
    const auto dat = _mm256_set1_epi32(src);
    while (count > 8)
    {
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(destPtr), dat);
        destPtr += 8; count -= 8;
    }
#elif COMMON_SIMD_LV >= 20
    const auto dat = _mm_set1_epi8(src);
    while (count > 4)
    {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr), dat);
        destPtr += 4; count -= 4;
    }
#endif
    while (count > 0)
    {
        *destPtr++ = src;
        count--;
    }
}
}
template<typename Type>
inline void BroadcastMany(Type* const dest, const size_t destCount, const Type& src, const size_t srcCount)
{
    if (destCount < srcCount)
        COMMON_THROW(BaseException, u"space avaliable on Dest is smaller than space required.");
    if constexpr(sizeof(Type) == 1)
        detail::BroadcastMany(dest, *(const uint8_t*)&src, srcCount);
    else if constexpr(sizeof(Type) == 2)
        detail::BroadcastMany(dest, *(const uint16_t*)&src, srcCount);
    else if constexpr(sizeof(Type) == 4)
        detail::BroadcastMany(dest, *(const uint32_t*)&src, srcCount);
    else
    {
        for (size_t i = 0; i < srcCount; ++i)
            *dest++ = src;
    }
}


}
}
