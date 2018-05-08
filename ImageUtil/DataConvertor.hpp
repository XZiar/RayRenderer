#pragma once

#include "ImageUtilRely.h"


namespace xziar::img::convert
{

inline uint16_t ParseWordLE(const byte *data) { return static_cast<uint16_t>((std::to_integer<uint16_t>(data[1]) << 8) + std::to_integer<uint16_t>(data[0])); }
inline uint16_t ParseWordBE(const byte *data) { return static_cast<uint16_t>(std::to_integer<uint16_t>(data[1]) + std::to_integer<uint16_t>(data[0] << 8)); }
inline void WordToLE(byte *output, const uint16_t data) { output[0] = byte(data & 0xff); output[1] = byte(data >> 8); }
inline void WordToBE(byte *output, const uint16_t data) { output[1] = byte(data & 0xff); output[0] = byte(data >> 8); }
inline uint32_t ParseDWordLE(const byte *data) 
{ 
    return static_cast<uint32_t>(
        (std::to_integer<uint32_t>(data[3]) << 24) + 
        (std::to_integer<uint32_t>(data[2]) << 16) + 
        (std::to_integer<uint32_t>(data[1]) << 8) + 
        (std::to_integer<uint32_t>(data[0])));
}
inline uint32_t ParseDWordBE(const byte *data) 
{
    return static_cast<uint32_t>(
        (std::to_integer<uint32_t>(data[3])) + 
        (std::to_integer<uint32_t>(data[2]) << 8) + 
        (std::to_integer<uint32_t>(data[1]) << 16) + 
        (std::to_integer<uint32_t>(data[0]) << 24));
}
inline void DWordToLE(byte *output, const uint32_t data) { output[0] = byte(data & 0xff); output[1] = byte(data >> 8); output[2] = byte(data >> 16); output[3] = byte(data >> 24); }
inline void DWordToBE(byte *output, const uint32_t data) { output[3] = byte(data & 0xff); output[2] = byte(data >> 8); output[1] = byte(data >> 16); output[0] = byte(data >> 24); }

template<typename T>
inline T EmptyStruct()
{
    T obj;
    memset(&obj, 0, sizeof(T));
    return obj;
}


inline void FixAlpha(size_t count, uint32_t* destPtr)
{
    while (count--)
        (*destPtr++) |= 0xff000000u;
}

inline void CopyRGBAToRGB(byte * __restrict &destPtr, const uint32_t color)
{
    const auto* __restrict colorPtr = reinterpret_cast<const byte*>(&color);
    *destPtr++ = colorPtr[0];
    *destPtr++ = colorPtr[1];
    *destPtr++ = colorPtr[2];
}


#pragma region GRAY->GRAYA
inline void GraysToGrayAs(byte * __restrict destPtr, const byte * __restrict srcPtr, uint64_t count)
{
#if defined(COMPILER_MSVC)
#   pragma warning(suppress: 4309)
    constexpr int16_t AlphaMaskVal = static_cast<int16_t>(0xff00);
#else
    constexpr int16_t AlphaMaskVal = static_cast<int16_t>(0xff00);
#endif
#if COMMON_SIMD_LV >= 200
    const auto alphaMask = _mm256_set1_epi16(AlphaMaskVal);
    const auto shuffleMask1 = _mm256_setr_epi8(0, -1, 1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1, 0, -1, 1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1);
    const auto shuffleMask2 = _mm256_setr_epi8(8, -1, 9, -1, 10, -1, 11, -1, 12, -1, 13, -1, 14, -1, 15, -1, 8, -1, 9, -1, 10, -1, 11, -1, 12, -1, 13, -1, 14, -1, 15, -1);
    while (count > 64)
    {
        const auto in1 = _mm256_loadu_si256((const __m256i*)srcPtr); srcPtr += 32;//0~31
        const auto in2 = _mm256_loadu_si256((const __m256i*)srcPtr); srcPtr += 32;//32~63
        count -= 64;
        const auto tmp1 = _mm256_or_si256(_mm256_shuffle_epi8(in1, shuffleMask1), alphaMask);//0~7,16~23
        const auto tmp2 = _mm256_or_si256(_mm256_shuffle_epi8(in1, shuffleMask2), alphaMask);//8~15,24~31
        const auto tmp3 = _mm256_or_si256(_mm256_shuffle_epi8(in2, shuffleMask1), alphaMask);//0~7,16~23
        const auto tmp4 = _mm256_or_si256(_mm256_shuffle_epi8(in2, shuffleMask2), alphaMask);//8~15,24~31

        _mm256_storeu_si256((__m256i*)destPtr, _mm256_permute2x128_si256(tmp1, tmp2, 0x20)); destPtr += 32;//0~7,8~15
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_permute2x128_si256(tmp1, tmp2, 0x31)); destPtr += 32;//16~23,24~31
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_permute2x128_si256(tmp3, tmp4, 0x20)); destPtr += 32;//0~7,8~15
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_permute2x128_si256(tmp3, tmp4, 0x31)); destPtr += 32;//16~23,24~31
    }
#elif COMMON_SIMD_LV >= 31
    const auto alphaMask = _mm_set1_epi16(AlphaMaskVal);
    const auto shuffleMask1 = _mm_setr_epi8(0, -1, 1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1);
    const auto shuffleMask2 = _mm_setr_epi8(8, -1, 9, -1, 10, -1, 11, -1, 12, -1, 13, -1, 14, -1, 15, -1);
    while (count > 64)
    {
        const auto in1 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//0~15
        const auto in2 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//16~31
        const auto in3 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//32~47
        const auto in4 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//48~63
        count -= 64;
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in1, shuffleMask1), alphaMask)); destPtr += 16;//0~7
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in1, shuffleMask2), alphaMask)); destPtr += 16;//8~15
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in2, shuffleMask1), alphaMask)); destPtr += 16;//16~23
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in2, shuffleMask2), alphaMask)); destPtr += 16;//24~31
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in3, shuffleMask1), alphaMask)); destPtr += 16;//32~39
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in3, shuffleMask2), alphaMask)); destPtr += 16;//40~47
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in4, shuffleMask1), alphaMask)); destPtr += 16;//48~55
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in4, shuffleMask2), alphaMask)); destPtr += 16;//56~63
    }
#endif
    while (count)
    {
    #define LOOP_GRAY_GRAYA *(uint16_t*)destPtr = uint16_t(*srcPtr++) | 0xff00u; destPtr += 2; count--;
        switch (count)
        {
        default:LOOP_GRAY_GRAYA
        case 7: LOOP_GRAY_GRAYA
        case 6: LOOP_GRAY_GRAYA
        case 5: LOOP_GRAY_GRAYA
        case 4: LOOP_GRAY_GRAYA
        case 3: LOOP_GRAY_GRAYA
        case 2: LOOP_GRAY_GRAYA
        case 1: LOOP_GRAY_GRAYA
        }
    #undef LOOP_GRAY_GRAYA
    }
}
#pragma endregion GRAY->GRAYA


#pragma region GRAY->RGBA
constexpr auto MAKE_GRAY2RGBA()
{
    std::array<uint32_t, 256> ret{ 0 };
    for (uint32_t i = 0; i < 256u; ++i)
        ret[i] = (i * 0x00010101u) | 0xff000000u;
    return ret;
}
inline const auto GrayToRGBAMAP = MAKE_GRAY2RGBA();
#define LOOP_GRAY_RGBA *(uint32_t*)destPtr = GrayToRGBAMAP[*(uint8_t*)srcPtr++]; destPtr += 4; count--;
inline void GraysToRGBAs(byte * __restrict destPtr, const byte * __restrict srcPtr, uint64_t count)
{
#if COMMON_SIMD_LV >= 200
    const auto alphaMask = _mm256_set1_epi32(0xff000000);
    const auto shuffleMask1 = _mm256_setr_epi8(0, 0, 0, -1, 1, 1, 1, -1, 2, 2, 2, -1, 3, 3, 3, -1, 0, 0, 0, -1, 1, 1, 1, -1, 2, 2, 2, -1, 3, 3, 3, -1);
    const auto shuffleMask2 = _mm256_setr_epi8(4, 4, 4, -1, 5, 5, 5, -1, 6, 6, 6, -1, 7, 7, 7, -1, 4, 4, 4, -1, 5, 5, 5, -1, 6, 6, 6, -1, 7, 7, 7, -1);
    const auto shuffleMask3 = _mm256_setr_epi8(8, 8, 8, -1, 9, 9, 9, -1, 10, 10, 10, -1, 11, 11, 11, -1, 8, 8, 8, -1, 9, 9, 9, -1, 10, 10, 10, -1, 11, 11, 11, -1);
    const auto shuffleMask4 = _mm256_setr_epi8(12, 12, 12, -1, 13, 13, 13, -1, 14, 14, 14, -1, 15, 15, 15, -1, 12, 12, 12, -1, 13, 13, 13, -1, 14, 14, 14, -1, 15, 15, 15, -1);
    while (count > 32)
    {
        const auto in = _mm256_loadu_si256((const __m256i*)srcPtr);//0~32
        srcPtr += 32; count -= 32;
        const auto tmp1 = _mm256_or_si256(_mm256_shuffle_epi8(in, shuffleMask1), alphaMask);//0~3,16~19
        const auto tmp2 = _mm256_or_si256(_mm256_shuffle_epi8(in, shuffleMask2), alphaMask);//4~7,20~23
        const auto tmp3 = _mm256_or_si256(_mm256_shuffle_epi8(in, shuffleMask3), alphaMask);//8~11,24~27
        const auto tmp4 = _mm256_or_si256(_mm256_shuffle_epi8(in, shuffleMask4), alphaMask);//12~15,28~31

        _mm256_storeu_si256((__m256i*)destPtr, _mm256_permute2x128_si256(tmp1, tmp2, 0x20)); destPtr += 32;//0~3,4~7
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_permute2x128_si256(tmp3, tmp4, 0x20)); destPtr += 32;//8~11,12~15
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_permute2x128_si256(tmp1, tmp2, 0x31)); destPtr += 32;//16~19,20~23
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_permute2x128_si256(tmp3, tmp4, 0x31)); destPtr += 32;//24~27,28~31
    }
#elif COMMON_SIMD_LV >= 31
    const auto alphaMask = _mm_set1_epi32(0xff000000);
    const auto shuffleMask1 = _mm_setr_epi8(0, 0, 0, -1, 1, 1, 1, -1, 2, 2, 2, -1, 3, 3, 3, -1);
    const auto shuffleMask2 = _mm_setr_epi8(4, 4, 4, -1, 5, 5, 5, -1, 6, 6, 6, -1, 7, 7, 7, -1);
    const auto shuffleMask3 = _mm_setr_epi8(8, 8, 8, -1, 9, 9, 9, -1, 10, 10, 10, -1, 11, 11, 11, -1);
    const auto shuffleMask4 = _mm_setr_epi8(12, 12, 12, -1, 13, 13, 13, -1, 14, 14, 14, -1, 15, 15, 15, -1);
    while (count > 32)
    {
        const auto in1 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//0~16
        const auto in2 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//17~32
        count -= 32;
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in1, shuffleMask1), alphaMask)); destPtr += 16;//0~3
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in1, shuffleMask2), alphaMask)); destPtr += 16;//4~7
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in1, shuffleMask3), alphaMask)); destPtr += 16;//8~11
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in1, shuffleMask4), alphaMask)); destPtr += 16;//12~15
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in2, shuffleMask1), alphaMask)); destPtr += 16;//16~19
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in2, shuffleMask2), alphaMask)); destPtr += 16;//20~23
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in2, shuffleMask3), alphaMask)); destPtr += 16;//24~27
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(_mm_shuffle_epi8(in2, shuffleMask4), alphaMask)); destPtr += 16;//28~31
    }
#endif
    while (count)
    {
        switch (count)
        {
        default:LOOP_GRAY_RGBA
        case 7: LOOP_GRAY_RGBA
        case 6: LOOP_GRAY_RGBA
        case 5: LOOP_GRAY_RGBA
        case 4: LOOP_GRAY_RGBA
        case 3: LOOP_GRAY_RGBA
        case 2: LOOP_GRAY_RGBA
        case 1: LOOP_GRAY_RGBA
        }
    }
}
inline const auto& GraysToBGRAs = GraysToRGBAs;
#undef LOOP_GRAY_RGBA
#pragma endregion GRAY->RGBA


#pragma region GRAYA->GRAY
inline void GrayAsToGrays(byte * __restrict destPtr, const byte * __restrict srcPtr, uint64_t count)
{
#if COMMON_SIMD_LV >= 200
    const auto shuffleMask1 = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto shuffleMask2 = _mm256_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 4, 6, 8, 10, 12, 14);
    while (count > 64)
    {
        const auto in1 = _mm256_loadu_si256((const __m256i*)srcPtr); srcPtr += 32;//0~7,8~15
        const auto in2 = _mm256_loadu_si256((const __m256i*)srcPtr); srcPtr += 32;//16~23,24~31
        const auto tmp1 = _mm256_shuffle_epi8(_mm256_permute2x128_si256(in1, in2, 0x20), shuffleMask1);//0~7,00,16~23,00
        const auto tmp2 = _mm256_shuffle_epi8(_mm256_permute2x128_si256(in1, in2, 0x31), shuffleMask2);//00,8~15,00,24~31
        const auto in3 = _mm256_loadu_si256((const __m256i*)srcPtr); srcPtr += 32;//32~39,40~47
        const auto in4 = _mm256_loadu_si256((const __m256i*)srcPtr); srcPtr += 32;//48~55,56~63
        const auto tmp3 = _mm256_shuffle_epi8(_mm256_permute2x128_si256(in3, in4, 0x20), shuffleMask1);//32~39,00,48~55,00
        const auto tmp4 = _mm256_shuffle_epi8(_mm256_permute2x128_si256(in3, in4, 0x31), shuffleMask2);//00,40~47,00,56~63
        count -= 64;
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_or_si256(tmp1, tmp2)); destPtr += 32;//0~15,16~31
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_or_si256(tmp3, tmp4)); destPtr += 32;//32~47,48~63
    }
#elif COMMON_SIMD_LV >= 31
    const auto shuffleMask1 = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto shuffleMask2 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 4, 6, 8, 10, 12, 14);
    while (count > 64)
    {
        const auto in1 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)srcPtr), shuffleMask1); srcPtr += 16;//0~7
        const auto in2 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)srcPtr), shuffleMask2); srcPtr += 16;//8~15
        const auto in3 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)srcPtr), shuffleMask1); srcPtr += 16;//16~23
        const auto in4 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)srcPtr), shuffleMask2); srcPtr += 16;//24~31
        const auto in5 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)srcPtr), shuffleMask1); srcPtr += 16;//32~39
        const auto in6 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)srcPtr), shuffleMask2); srcPtr += 16;//40~47
        const auto in7 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)srcPtr), shuffleMask1); srcPtr += 16;//48~55
        const auto in8 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)srcPtr), shuffleMask2); srcPtr += 16;//56~63
        count -= 64;
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(in1, in2)); destPtr += 16;//0~15
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(in3, in4)); destPtr += 16;//16~31
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(in5, in6)); destPtr += 16;//32~47
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(in7, in8)); destPtr += 16;//48~63
    }
#endif
    while (count)
    {
    #define LOOP_GRAYA_GRAY *destPtr++ = byte(*(const uint16_t*)srcPtr & 0xff); srcPtr += 2; count--;
        switch (count)
        {
        default:LOOP_GRAYA_GRAY
        case 7: LOOP_GRAYA_GRAY
        case 6: LOOP_GRAYA_GRAY
        case 5: LOOP_GRAYA_GRAY
        case 4: LOOP_GRAYA_GRAY
        case 3: LOOP_GRAYA_GRAY
        case 2: LOOP_GRAYA_GRAY
        case 1: LOOP_GRAYA_GRAY
        }
    #undef LOOP_GRAYA_GRAY
    }
}
#pragma endregion GRAYA->GRAY


#pragma region GRAYA->RGBA
#pragma warning(disable: 4309)
constexpr auto MAKE_GRAYA2RGBA()
{
    constexpr uint32_t size = 256 * 256;
    std::array<uint32_t, size> ret{ 0 };
    for (uint32_t i = 0; i < size; ++i)
        ret[i] = ((i & 0xffu) * 0x00010101u) | ((i & 0xff00u) << 16);
    return ret;
}
inline const auto GrayAToRGBAMAP = MAKE_GRAYA2RGBA();
#define LOOP_GRAYA_RGBA *(uint32_t*)destPtr = GrayAToRGBAMAP[*(uint16_t*)srcPtr]; destPtr += 4; srcPtr+=2; count--;
inline void GrayAsToRGBAs(byte * __restrict destPtr, const byte * __restrict srcPtr, uint64_t count)
{
#if COMMON_SIMD_LV >= 200
    const auto shuffleMask1 = _mm256_setr_epi8(0, 0, 0, 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 6, 7, 0, 0, 0, 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 6, 7);
    const auto shuffleMask2 = _mm256_setr_epi8(8, 8, 8, 9, 10, 10, 10, 11, 12, 12, 12, 13, 14, 14, 14, 15, 8, 8, 8, 9, 10, 10, 10, 11, 12, 12, 12, 13, 14, 14, 14, 15);
    while (count > 64)
    {
        const auto in1 = _mm256_loadu_si256((const __m256i*)srcPtr); srcPtr += 32;//0~15
        const auto in2 = _mm256_loadu_si256((const __m256i*)srcPtr); srcPtr += 32;//16~31
        count -= 64;
        const auto tmp1 = _mm256_shuffle_epi8(in1, shuffleMask1);//0~3,8~11
        const auto tmp2 = _mm256_shuffle_epi8(in1, shuffleMask2);//4~7,12~15
        const auto tmp3 = _mm256_shuffle_epi8(in2, shuffleMask1);//16~19,24~27
        const auto tmp4 = _mm256_shuffle_epi8(in2, shuffleMask2);//20~23,28~31

        _mm256_storeu_si256((__m256i*)destPtr, _mm256_permute2x128_si256(tmp1, tmp2, 0x20)); destPtr += 32;//0~3,4~7
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_permute2x128_si256(tmp1, tmp2, 0x31)); destPtr += 32;//8~11,12~15
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_permute2x128_si256(tmp3, tmp4, 0x20)); destPtr += 32;//16~19,20~23
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_permute2x128_si256(tmp3, tmp4, 0x31)); destPtr += 32;//24~27,28~31
    }
#elif COMMON_SIMD_LV >= 31
    const auto shuffleMask1 = _mm_setr_epi8(0, 0, 0, 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 6, 7);
    const auto shuffleMask2 = _mm_setr_epi8(8, 8, 8, 9, 10, 10, 10, 11, 12, 12, 12, 13, 14, 14, 14, 15);
    while (count > 64)
    {
        const auto in1 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//0~7
        const auto in2 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//8~15
        const auto in3 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//16~23
        const auto in4 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//24~31
        count -= 64;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in1, shuffleMask1)); destPtr += 16;//0~3
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in1, shuffleMask2)); destPtr += 16;//4~7
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in2, shuffleMask1)); destPtr += 16;//8~11
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in2, shuffleMask2)); destPtr += 16;//12~15
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in3, shuffleMask1)); destPtr += 16;//16~19
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in3, shuffleMask2)); destPtr += 16;//20~23
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in4, shuffleMask1)); destPtr += 16;//24~27
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in4, shuffleMask2)); destPtr += 16;//28~31
    }
#endif
    while (count)
    {
        switch (count)
        {
        default:LOOP_GRAYA_RGBA
        case 7: LOOP_GRAYA_RGBA
        case 6: LOOP_GRAYA_RGBA
        case 5: LOOP_GRAYA_RGBA
        case 4: LOOP_GRAYA_RGBA
        case 3: LOOP_GRAYA_RGBA
        case 2: LOOP_GRAYA_RGBA
        case 1: LOOP_GRAYA_RGBA
        }
    }
}
inline const auto& GrayAsToBGRAs = GrayAsToRGBAs;
#undef LOOP_GRAYA_RGBA
#pragma warning(default: 4309)
#pragma endregion GRAYA->RGBA


#pragma region GRAY->RGB
#define LOOP_GRAY_RGB destPtr[0] = destPtr[1] = destPtr[2] = *srcPtr++; destPtr += 3; count--;
inline void GraysToRGBs(byte * __restrict destPtr, const byte * __restrict srcPtr, uint64_t count)
{
#if COMMON_SIMD_LV >= 31
    const auto shuffleMask1 = _mm_setr_epi8(0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5);
    const auto shuffleMask2 = _mm_setr_epi8(5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10);
    const auto shuffleMask3 = _mm_setr_epi8(10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15);
    while (count > 64)
    {
        const auto in1 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//0~15
        const auto in2 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//16~31
        const auto in3 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//32~47
        const auto in4 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//48~63
        count -= 64;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in1, shuffleMask1)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in1, shuffleMask2)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in1, shuffleMask3)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in2, shuffleMask1)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in2, shuffleMask2)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in2, shuffleMask3)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in3, shuffleMask1)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in3, shuffleMask2)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in3, shuffleMask3)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in4, shuffleMask1)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in4, shuffleMask2)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in4, shuffleMask3)); destPtr += 16;
    }
    while (count > 16)
    {
        const auto in = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//0~15
        count -= 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in, shuffleMask1)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in, shuffleMask2)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in, shuffleMask3)); destPtr += 16;
    }
#endif
    while (count)
    {
        switch (count)
        {
        default:LOOP_GRAY_RGB
        case 7: LOOP_GRAY_RGB
        case 6: LOOP_GRAY_RGB
        case 5: LOOP_GRAY_RGB
        case 4: LOOP_GRAY_RGB
        case 3: LOOP_GRAY_RGB
        case 2: LOOP_GRAY_RGB
        case 1: LOOP_GRAY_RGB
        }
    }
}
inline const auto& GraysToBGRs = GraysToRGBs;
#undef LOOP_GRAY_RGB
#pragma endregion GRAY->RGB


#pragma region GRAYA->RGB
#define LOOP_GRAYA_RGB destPtr[0] = destPtr[1] = destPtr[2] = *srcPtr; destPtr += 3; srcPtr += 2; count--;
inline void GrayAsToRGBs(byte * __restrict destPtr, const byte * __restrict srcPtr, uint64_t count)
{
#if COMMON_SIMD_LV >= 31
    const auto shuffleMask1 = _mm_setr_epi8(0, 0, 0, 2, 2, 2, 4, 4, 4, 6, 6, 6, 8, 8, 8, 10);
    const auto shuffleMask2 = _mm_setr_epi8(10, 10, 12, 12, 12, 14, 14, 14, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto shuffleMask3 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 2, 2, 2, 4, 4);
    const auto shuffleMask4 = _mm_setr_epi8(4, 6, 6, 6, 8, 8, 8, 10, 10, 10, 12, 12, 12, 14, 14, 14);
    while (count > 32)
    {
        const auto in1 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//0~7
        const auto in2 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//8~15
        const auto in3 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//16~23
        const auto in4 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//24~31
        count -= 32;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in1, shuffleMask1)); destPtr += 16;
        const auto tmp1 = _mm_or_si128(_mm_shuffle_epi8(in1, shuffleMask2), _mm_shuffle_epi8(in2, shuffleMask3));
        _mm_storeu_si128((__m128i*)destPtr, tmp1); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in2, shuffleMask4)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in3, shuffleMask1)); destPtr += 16;
        const auto tmp2 = _mm_or_si128(_mm_shuffle_epi8(in3, shuffleMask2), _mm_shuffle_epi8(in4, shuffleMask3));
        _mm_storeu_si128((__m128i*)destPtr, tmp2); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in4, shuffleMask4)); destPtr += 16;
    }
    while (count > 16)
    {
        const auto in1 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//0~7
        const auto in2 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;//8~15
        count -= 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in1, shuffleMask1)); destPtr += 16;
        const auto tmp1 = _mm_or_si128(_mm_shuffle_epi8(in1, shuffleMask2), _mm_shuffle_epi8(in2, shuffleMask3));
        _mm_storeu_si128((__m128i*)destPtr, tmp1); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(in2, shuffleMask4)); destPtr += 16;
    }
#endif
    while (count)
    {
        switch (count)
        {
        default:LOOP_GRAYA_RGB
        case 7: LOOP_GRAYA_RGB
        case 6: LOOP_GRAYA_RGB
        case 5: LOOP_GRAYA_RGB
        case 4: LOOP_GRAYA_RGB
        case 3: LOOP_GRAYA_RGB
        case 2: LOOP_GRAYA_RGB
        case 1: LOOP_GRAYA_RGB
        }
    }
}
inline const auto& GrayAsToBGRs = GrayAsToRGBs;
#undef LOOP_GRAYA_RGB
#pragma endregion GRAYA->RGB


#pragma region RGBA->RGB
#define LOOP_RGBA_RGB \
        *destPtr++ = *srcPtr++; \
        *destPtr++ = *srcPtr++; \
        *destPtr++ = *srcPtr++; \
        srcPtr++; count--;
inline void RGBAsToRGBs(byte * __restrict destPtr, const byte * __restrict srcPtr, uint64_t count)
{
    while (count)
    {
        switch (count)
        {
        default:LOOP_RGBA_RGB
        case 7: LOOP_RGBA_RGB
        case 6: LOOP_RGBA_RGB
        case 5: LOOP_RGBA_RGB
        case 4: LOOP_RGBA_RGB
        case 3: LOOP_RGBA_RGB
        case 2: LOOP_RGBA_RGB
        case 1: LOOP_RGBA_RGB
        }
    }
}
inline const auto& BGRAsToBGRs = RGBAsToRGBs;
#undef LOOP_RGBA_RGB
#pragma endregion RGBA->RGB


#pragma region BGRA->RGB
#define LOOP_BGRA_RGB \
        *destPtr++ = srcPtr[2]; \
        *destPtr++ = srcPtr[1]; \
        *destPtr++ = srcPtr[0]; \
        srcPtr += 4; count--;
inline void BGRAsToRGBs(byte * __restrict destPtr, const byte * __restrict srcPtr, uint64_t count)
{
    while (count)
    {
        switch (count)
        {
        default:LOOP_BGRA_RGB
        case 7: LOOP_BGRA_RGB
        case 6: LOOP_BGRA_RGB
        case 5: LOOP_BGRA_RGB
        case 4: LOOP_BGRA_RGB
        case 3: LOOP_BGRA_RGB
        case 2: LOOP_BGRA_RGB
        case 1: LOOP_BGRA_RGB
        }
    }
}
inline const auto& RGBAsToBGRs = BGRAsToRGBs;
#undef LOOP_BGRA_RGB
#pragma endregion BGRA->RGB


#pragma region BGR->RGB(in place)
#define LOOP_BGR_RGB \
        { \
            const auto tmp = destPtr[0]; \
            destPtr[0] = destPtr[2]; \
            destPtr[2] = tmp; \
            destPtr += 3; count--; \
        }
inline void BGRsToRGBs(byte * __restrict destPtr, uint64_t count)
{
    while (count)
    {
        switch (count)
        {
        default:LOOP_BGR_RGB
        case 7: LOOP_BGR_RGB
        case 6: LOOP_BGR_RGB
        case 5: LOOP_BGR_RGB
        case 4: LOOP_BGR_RGB
        case 3: LOOP_BGR_RGB
        case 2: LOOP_BGR_RGB
        case 1: LOOP_BGR_RGB
        }
    }
}
#undef LOOP_BGR_RGB
#pragma endregion BGR->RGB(in place)


#pragma region BGR->RGB(copy)
#define LOOP_BGR_RGB \
        { \
            destPtr[0] = srcPtr[2]; \
            destPtr[1] = srcPtr[1]; \
            destPtr[2] = srcPtr[0]; \
            destPtr += 3, srcPtr += 3; count--; \
        }
inline void BGRsToRGBs(byte * __restrict destPtr, const byte * __restrict srcPtr, uint64_t count)
{
    while (count)
    {
        switch (count)
        {
        default:LOOP_BGR_RGB
        case 7: LOOP_BGR_RGB
        case 6: LOOP_BGR_RGB
        case 5: LOOP_BGR_RGB
        case 4: LOOP_BGR_RGB
        case 3: LOOP_BGR_RGB
        case 2: LOOP_BGR_RGB
        case 1: LOOP_BGR_RGB
        }
    }
}
#undef LOOP_BGR_RGB
#pragma endregion BGR->RGB(copy)


#pragma region RGB->RGBA
#define LOOP_RGB_RGBA \
        *destPtr++ = *srcPtr++; \
        *destPtr++ = *srcPtr++; \
        *destPtr++ = *srcPtr++; \
        *destPtr++ = byte(0xff); count--;
inline void RGBsToRGBAs(byte * __restrict destPtr, const byte * __restrict srcPtr, uint64_t count)
{
#if COMMON_SIMD_LV >= 41
    const auto shuffleMask = _mm_setr_epi8(0x0, 0x1, 0x2, -1, 0x3, 0x4, 0x5, -1, 0x6, 0x7, 0x8, -1, 0x9, 0xa, 0xb, -1);
    const auto shuffleMask2 = _mm_setr_epi8(0x4, 0x5, 0x6, -1, 0x7, 0x8, 0x9, -1, 0xa, 0xb, 0xc, -1, 0xd, 0xe, 0xf, -1);
    const auto alphaMask = _mm_set1_epi32(0xff000000);
    while (count >= 16)
    {
        const auto raw1 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;
        const auto raw2 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;
        const auto raw3 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;
        //rgb0rgb0rgb0
        const auto shuffled1 = _mm_shuffle_epi8(raw1, shuffleMask);
        const auto shuffled2 = _mm_shuffle_epi8(_mm_alignr_epi8(raw2, raw1, 12), shuffleMask);
        const auto shuffled3 = _mm_shuffle_epi8(_mm_alignr_epi8(raw3, raw2, 8), shuffleMask);
        const auto shuffled4 = _mm_shuffle_epi8(raw3, shuffleMask2);
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(shuffled1, alphaMask)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(shuffled2, alphaMask)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(shuffled3, alphaMask)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(shuffled4, alphaMask)); destPtr += 16;
        count -= 16;
    }
    while (count >= 4)
    {
        const auto raw = _mm_loadu_si128((const __m128i*)srcPtr);//rgbrgbrgbxxxx
        const auto shuffled = _mm_shuffle_epi8(raw, shuffleMask);//rgb0rgb0rgb0
        const auto alphaed = _mm_or_si128(shuffled, alphaMask);
        _mm_storeu_si128((__m128i*)destPtr, alphaed);
        destPtr += 16, srcPtr += 12, count -= 4;
    }
#endif
    while (count)
    {
        switch (count)
        {
        default:LOOP_RGB_RGBA
        case 3: LOOP_RGB_RGBA
        case 2: LOOP_RGB_RGBA
        case 1: LOOP_RGB_RGBA
        }
    }
}
inline const auto& BGRsToBGRAs = RGBsToRGBAs;
#undef LOOP_RGB_RGBA
#pragma endregion RGB->RGBA


#pragma region BGR->RGBA
#define LOOP_BGR_RGBA \
        *destPtr++ = srcPtr[2]; \
        *destPtr++ = srcPtr[1]; \
        *destPtr++ = *srcPtr; \
        *destPtr++ = byte(0xff); \
        srcPtr += 3; count--;
inline void BGRsToRGBAs(byte * __restrict destPtr, const byte * __restrict srcPtr, uint64_t count)
{
#if COMMON_SIMD_LV >= 41
    const auto shuffleMask = _mm_setr_epi8(0x2, 0x1, 0x0, -1, 0x5, 0x4, 0x3, -1, 0x8, 0x7, 0x6, -1, 0xb, 0xa, 0x9, -1);
    const auto shuffleMask2 = _mm_setr_epi8(0x6, 0x5, 0x4, -1, 0x9, 0x8, 0x7, -1, 0xc, 0xb, 0xa, -1, 0xf, 0xe, 0xd, -1);
    const auto alphaMask = _mm_set1_epi32(0xff000000);
    while (count >= 16)
    {
        const auto raw1 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;
        const auto raw2 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;
        const auto raw3 = _mm_loadu_si128((const __m128i*)srcPtr); srcPtr += 16;
        //rgb0rgb0rgb0
        const auto shuffled1 = _mm_shuffle_epi8(raw1, shuffleMask);
        const auto shuffled2 = _mm_shuffle_epi8(_mm_alignr_epi8(raw2, raw1, 12), shuffleMask);
        const auto shuffled3 = _mm_shuffle_epi8(_mm_alignr_epi8(raw3, raw2, 8), shuffleMask);
        const auto shuffled4 = _mm_shuffle_epi8(raw3, shuffleMask2);
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(shuffled1, alphaMask)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(shuffled2, alphaMask)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(shuffled3, alphaMask)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_or_si128(shuffled4, alphaMask)); destPtr += 16;
        count -= 16;
    }
    while (count >= 4)
    {
        const auto raw = _mm_loadu_si128((const __m128i*)srcPtr);//bgrbgrbgrxxxx
        const auto shuffled = _mm_shuffle_epi8(raw, shuffleMask);//rgb0rgb0rgb0
        const auto alphaed = _mm_or_si128(shuffled, alphaMask);
        _mm_storeu_si128((__m128i*)destPtr, alphaed);
        destPtr += 16, srcPtr += 12, count -= 4;
    }
#endif
    while (count)
    {
        switch (count)
        {
        default:LOOP_BGR_RGBA
        case 3: LOOP_BGR_RGBA
        case 2: LOOP_BGR_RGBA
        case 1: LOOP_BGR_RGBA
        }
    }
}
inline const auto& RGBsToBGRAs = BGRsToRGBAs;
#undef LOOP_BGR_RGBA
#pragma endregion BGR->RGBA


#pragma region BGRA->RGBA(in place)
#define LOOP_BGRA_RGBA \
        { \
            const auto tmp = destPtr[0]; \
            destPtr[0] = destPtr[2]; \
            destPtr[2] = tmp; \
            destPtr += 4; count--; \
        }
inline void BGRAsToRGBAs(byte * __restrict destPtr, uint64_t count)
{
#if COMMON_SIMD_LV >= 31
    const auto shuffle128 = _mm_setr_epi8(0x2, 0x1, 0x0, 0x3, 0x6, 0x5, 0x4, 0x7, 0xa, 0x9, 0x8, 0xb, 0xe, 0xd, 0xc, 0xf);
#   if COMMON_SIMD_LV >= 100 || defined(__AVX2__)
    const auto shuffle256 = _mm256_set_m128i(shuffle128, shuffle128);
    while (count >= 32)
    {
        //bgrabgrabgrabgra -> rgbargbargbargba
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_shuffle_epi8(_mm256_loadu_si256((const __m256i*)destPtr), shuffle256)); destPtr += 32;
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_shuffle_epi8(_mm256_loadu_si256((const __m256i*)destPtr), shuffle256)); destPtr += 32;
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_shuffle_epi8(_mm256_loadu_si256((const __m256i*)destPtr), shuffle256)); destPtr += 32;
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_shuffle_epi8(_mm256_loadu_si256((const __m256i*)destPtr), shuffle256)); destPtr += 32;
        count -= 32;
        continue;
    }
    while (count >= 8)
    {
        const auto raw = _mm256_loadu_si256((const __m256i*)destPtr);//bgrabgrabgra
        const auto shuffled = _mm256_shuffle_epi8(raw, shuffle256);//rgbargbargba
        _mm256_storeu_si256((__m256i*)destPtr, shuffled);
        destPtr += 32, count -= 8;
        continue;
    }
#   else
    while (count >= 16)
    {
        //bgrabgrabgrabgra -> rgbargbargbargba
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)destPtr), shuffle128)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)destPtr), shuffle128)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)destPtr), shuffle128)); destPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)destPtr), shuffle128)); destPtr += 16;
        count -= 16;
        continue;
    }
#   endif
    while (count >= 4)
    {
        const auto raw = _mm_loadu_si128((const __m128i*)destPtr);//bgrabgrabgra
        const auto shuffled = _mm_shuffle_epi8(raw, shuffle128);//rgbargbargba
        _mm_storeu_si128((__m128i*)destPtr, shuffled);
        destPtr += 16, count -= 4;
        continue;
    }
#endif
    while (count)
    {
        switch (count)
        {
        default:LOOP_BGRA_RGBA
        case 3: LOOP_BGRA_RGBA
        case 2: LOOP_BGRA_RGBA
        case 1: LOOP_BGRA_RGBA
        }
    }
}
#undef LOOP_BGRA_RGBA
#pragma endregion BGRA->RGBA(in place)


#pragma region BGRA->RGBA(copy)
#define LOOP_BGRA_RGBA \
        { \
            destPtr[0] = srcPtr[2]; \
            destPtr[1] = srcPtr[1]; \
            destPtr[2] = srcPtr[0]; \
            destPtr[3] = srcPtr[3]; \
            destPtr += 4, srcPtr += 4; count--; \
        }
inline void BGRAsToRGBAs(byte * __restrict destPtr, const byte * __restrict srcPtr, uint64_t count)
{
#if COMMON_SIMD_LV >= 31
    const auto shuffle128 = _mm_setr_epi8(0x2, 0x1, 0x0, 0x3, 0x6, 0x5, 0x4, 0x7, 0xa, 0x9, 0x8, 0xb, 0xe, 0xd, 0xc, 0xf);
#   if COMMON_SIMD_LV >= 100 || defined(__AVX2__)
    const auto shuffle256 = _mm256_set_m128i(shuffle128, shuffle128);
    while (count >= 32)
    {
        //bgrabgrabgrabgra -> rgbargbargbargba
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_shuffle_epi8(_mm256_loadu_si256((const __m256i*)srcPtr), shuffle256)); destPtr += 32, srcPtr += 32;
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_shuffle_epi8(_mm256_loadu_si256((const __m256i*)srcPtr), shuffle256)); destPtr += 32, srcPtr += 32;
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_shuffle_epi8(_mm256_loadu_si256((const __m256i*)srcPtr), shuffle256)); destPtr += 32, srcPtr += 32;
        _mm256_storeu_si256((__m256i*)destPtr, _mm256_shuffle_epi8(_mm256_loadu_si256((const __m256i*)srcPtr), shuffle256)); destPtr += 32, srcPtr += 32;
        count -= 32;
        continue;
    }
    while (count >= 8)
    {
        const auto raw = _mm256_loadu_si256((const __m256i*)srcPtr);//bgrabgrabgra
        const auto shuffled = _mm256_shuffle_epi8(raw, shuffle256);//rgbargbargba
        _mm256_storeu_si256((__m256i*)destPtr, shuffled);
        destPtr += 32, srcPtr += 32, count -= 8;
        continue;
    }
#   else
    while (count >= 16)
    {
        //bgrabgrabgrabgra -> rgbargbargbargba
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)srcPtr), shuffle128)); destPtr += 16, srcPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)srcPtr), shuffle128)); destPtr += 16, srcPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)srcPtr), shuffle128)); destPtr += 16, srcPtr += 16;
        _mm_storeu_si128((__m128i*)destPtr, _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)srcPtr), shuffle128)); destPtr += 16, srcPtr += 16;
        count -= 16;
        continue;
    }
#   endif
    while (count >= 4)
    {
        const auto raw = _mm_loadu_si128((const __m128i*)srcPtr);//bgrabgrabgra
        const auto shuffled = _mm_shuffle_epi8(raw, shuffle128);//rgbargbargba
        _mm_storeu_si128((__m128i*)destPtr, shuffled);
        destPtr += 16, srcPtr += 16, count -= 4;
        continue;
    }
#endif
    while (count)
    {
        switch (count)
        {
        default:LOOP_BGRA_RGBA
        case 3: LOOP_BGRA_RGBA
        case 2: LOOP_BGRA_RGBA
        case 1: LOOP_BGRA_RGBA
        }
    }
}
#undef LOOP_BGRA_RGBA
#pragma endregion BGRA->RGBA(copy)


#pragma region SWAP two buffer
#define SWAP_BLOCK(COUNTER) \
        while (COUNTER--) \
        { \
            const auto tmp = *ptrA; \
            *ptrA++ = *ptrB; \
            *ptrB++ = tmp; \
        } 
inline bool Swap2Buffer(byte * __restrict ptrA, byte * __restrict ptrB, uint64_t bytes)
{
    if (ptrA == ptrB)
        return false;
    if (ptrA < ptrB && ptrA + bytes > ptrB)
        return false;
    if (ptrA > ptrB && ptrA < ptrB + bytes)
        return false;
#if COMMON_SIMD_LV >= 100 || defined(__AVX2__)
    if ((intptr_t)ptrA & 0x1f)
    {
        const uint64_t offset = 32 - ((intptr_t)ptrA & 0x1f);
        uint8_t count = (uint8_t)std::min(bytes, offset);
        bytes -= count;
        SWAP_BLOCK(count)
    }
    //ptrA now 32-byte aligned(start from a cache line)
    while (bytes >= 128)
    {
        const auto tmp1 = _mm256_load_si256((const __m256i*)ptrA);
        _mm256_store_si256((__m256i*)ptrA, _mm256_loadu_si256((const __m256i*)ptrB)); ptrA += 32;
        _mm256_storeu_si256((__m256i*)ptrB, tmp1); ptrB += 32;
        const auto tmp2 = _mm256_load_si256((const __m256i*)ptrA);
        _mm256_store_si256((__m256i*)ptrA, _mm256_loadu_si256((const __m256i*)ptrB)); ptrA += 32;
        _mm256_storeu_si256((__m256i*)ptrB, tmp2); ptrB += 32;
        const auto tmp3 = _mm256_load_si256((const __m256i*)ptrA);
        _mm256_store_si256((__m256i*)ptrA, _mm256_loadu_si256((const __m256i*)ptrB)); ptrA += 32;
        _mm256_storeu_si256((__m256i*)ptrB, tmp3); ptrB += 32;
        const auto tmp4 = _mm256_load_si256((const __m256i*)ptrA);
        _mm256_store_si256((__m256i*)ptrA, _mm256_loadu_si256((const __m256i*)ptrB)); ptrA += 32;
        _mm256_storeu_si256((__m256i*)ptrB, tmp4); ptrB += 32;
        bytes -= 128;
        _mm_prefetch((const char*)ptrA, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrA + 64, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrB, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrB + 64, _MM_HINT_NTA);
    }
    while (bytes >= 32)
    {
        const auto tmp = _mm256_load_si256((const __m256i*)ptrA);
        _mm256_store_si256((__m256i*)ptrA, _mm256_loadu_si256((const __m256i*)ptrB)); ptrA += 32;
        _mm256_storeu_si256((__m256i*)ptrB, tmp); ptrB += 32;
        bytes -= 32;
    }
    SWAP_BLOCK(bytes)
#elif COMMON_SIMD_LV >= 20
    if ((intptr_t)ptrA & 0x1f)
    {
        const uint64_t offset = 32 - ((intptr_t)ptrA & 0x1f);
        uint8_t count = (uint8_t)std::min(bytes, offset);
        bytes -= count;
        SWAP_BLOCK(count)
    }
    //ptrA now 32-byte aligned(start from a cache line)
    while (bytes >= 64)
    {
        const auto tmp1 = _mm_load_si128((const __m256i*)ptrA);
        _mm_store_si128((__m256i*)ptrA, _mm_loadu_si128((const __m256i*)ptrB)); ptrA += 16;
        _mm_storeu_si128((__m256i*)ptrB, tmp1); ptrB += 16;
        const auto tmp2 = _mm_load_si128((const __m256i*)ptrA);
        _mm_store_si128((__m256i*)ptrA, _mm_loadu_si128((const __m256i*)ptrB)); ptrA += 16;
        _mm_storeu_si128((__m256i*)ptrB, tmp2); ptrB += 16;
        const auto tmp3 = _mm_load_si128((const __m256i*)ptrA);
        _mm_store_si128((__m256i*)ptrA, _mm_loadu_si128((const __m256i*)ptrB)); ptrA += 16;
        _mm_storeu_si128((__m256i*)ptrB, tmp3); ptrB += 16;
        const auto tmp4 = _mm_load_si128((const __m256i*)ptrA);
        _mm_store_si128((__m256i*)ptrA, _mm_loadu_si128((const __m256i*)ptrB)); ptrA += 16;
        _mm_storeu_si128((__m256i*)ptrB, tmp4); ptrB += 16;
        bytes -= 64;
        _mm_prefetch((const char*)ptrA, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrB, _MM_HINT_NTA);
    }
    while (bytes >= 16)
    {
        const auto tmp = _mm_load_si128((const __m128i*)ptrA);
        _mm_store_si128((__m128i*)ptrA, _mm_loadu_si128((const __m128i*)ptrB)); ptrA += 16;
        _mm_storeu_si128((__m128i*)ptrB, tmp); ptrB += 16;
        bytes -= 16;
    }
    SWAP_BLOCK(bytes)
#else
#   define SWAP_UINT64 \
        { \
            const uint64_t tmp = *ptrA8; \
            *ptrA8++ = *ptrB8; \
            *ptrB8++ = tmp; \
            count8--; \
        }
    if ((intptr_t)ptrA & 0x7)
    {
        const uint64_t offset = 8 - ((intptr_t)ptrA & 0x7);
        uint8_t count = static_cast<uint8_t>(std::min(bytes, offset));
        bytes -= count;
        SWAP_BLOCK(count)
    }
    auto count8 = bytes / 8; bytes &= 0x7;
    uint64_t * __restrict ptrA8 = (uint64_t*)ptrA; uint64_t * __restrict ptrB8 = (uint64_t*)ptrB;
    while (count8)
    {
        switch (count8)
        {
        default:SWAP_UINT64
        case 7: SWAP_UINT64
        case 6: SWAP_UINT64
        case 5: SWAP_UINT64
        case 4: SWAP_UINT64
        case 3: SWAP_UINT64
        case 2: SWAP_UINT64
        case 1: SWAP_UINT64
        }
    }
    SWAP_BLOCK(bytes)
#endif
    return true;
}
#undef SWAP_UINT64
#undef SWAP_BLOCK
#pragma endregion SWAP two buffer


#pragma region REVERSE one buffer(per 4byte)
#define REV_BLOCK(COUNTER) \
        while (COUNTER--) \
        { \
            const auto tmp = *ptrA; \
            *ptrA++ = *ptrB; \
            *ptrB-- = tmp; \
        } 
inline bool ReverseBuffer4(byte * __restrict ptr, uint64_t count)
{
    uint32_t * __restrict ptrA = reinterpret_cast<uint32_t*>(ptr);
    uint32_t * __restrict ptrB = reinterpret_cast<uint32_t*>(ptr) + (count - 1);
    count = count / 2;
    if (count == 0)
        return false;
#if defined(IMGU_USE_SIMD)
    if ((intptr_t)ptrA & 0x1f)
    {
        const uint64_t offset = (32 - ((intptr_t)ptrA & 0x1f)) / 4;
        uint8_t diff = (uint8_t)std::min(count, offset);
        count -= diff;
        REV_BLOCK(diff)
    }
    //ptrA now 32-byte aligned(start from a cache line)
#endif
#if COMMON_SIMD_LV >= 200
    const auto indexer = _mm256_setr_epi32(7, 6, 5, 4, 3, 2, 1, 0);
    uint32_t * __restrict ptrC = ptrB - 7;
    while (count > 32)
    {
        const auto tmp1 = _mm256_permutevar8x32_epi32(_mm256_load_si256((const __m256i*)ptrA), indexer),
            tmp8 = _mm256_permutevar8x32_epi32(_mm256_load_si256((const __m256i*)ptrC), indexer);
        _mm256_store_si256((__m256i*)ptrA, tmp8); ptrA += 8;
        _mm256_storeu_si256((__m256i*)ptrC, tmp1); ptrC -= 8;
        const auto tmp2 = _mm256_permutevar8x32_epi32(_mm256_load_si256((const __m256i*)ptrA), indexer),
            tmp7 = _mm256_permutevar8x32_epi32(_mm256_load_si256((const __m256i*)ptrC), indexer);
        _mm256_store_si256((__m256i*)ptrA, tmp7); ptrA += 8;
        _mm256_storeu_si256((__m256i*)ptrC, tmp2); ptrC -= 8;
        const auto tmp3 = _mm256_permutevar8x32_epi32(_mm256_load_si256((const __m256i*)ptrA), indexer),
            tmp6 = _mm256_permutevar8x32_epi32(_mm256_load_si256((const __m256i*)ptrC), indexer);
        _mm256_store_si256((__m256i*)ptrA, tmp6); ptrA += 8;
        _mm256_storeu_si256((__m256i*)ptrC, tmp3); ptrC -= 8;
        const auto tmp4 = _mm256_permutevar8x32_epi32(_mm256_load_si256((const __m256i*)ptrA), indexer),
            tmp5 = _mm256_permutevar8x32_epi32(_mm256_load_si256((const __m256i*)ptrC), indexer);
        _mm256_store_si256((__m256i*)ptrA, tmp5); ptrA += 8;
        _mm256_storeu_si256((__m256i*)ptrC, tmp4); ptrC -= 8;
        count -= 32;
        _mm_prefetch((const char*)ptrA, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrA + 64, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC - 64, _MM_HINT_NTA);
    }
    while (count > 8)
    {
        const auto tmpA = _mm256_permutevar8x32_epi32(_mm256_load_si256((const __m256i*)ptrA), indexer),
            tmpC = _mm256_permutevar8x32_epi32(_mm256_load_si256((const __m256i*)ptrC), indexer);
        _mm256_store_si256((__m256i*)ptrA, tmpC); ptrA += 8;
        _mm256_storeu_si256((__m256i*)ptrC, tmpA); ptrC -= 8;
        count -= 8;
    }
    ptrB = ptrC + 7;
    REV_BLOCK(count)
#elif COMMON_SIMD_LV >= 100
    const auto indexer = _mm256_setr_epi32(3, 2, 1, 0, 3, 2, 1, 0);
    uint32_t * __restrict ptrC = ptrB - 7;
    while (count > 32)
    {
        auto tmp1 = _mm256_permutevar_ps(_mm256_load_ps((const float*)ptrA), indexer),
            tmp8 = _mm256_permutevar_ps(_mm256_load_ps((const float*)ptrC), indexer);
        tmp1 = _mm256_permute2f128_ps(tmp1, tmp1, 0x01), tmp8 = _mm256_permute2f128_ps(tmp8, tmp8, 0x01);
        _mm256_store_ps((float*)ptrA, tmp8); ptrA += 8;
        _mm256_storeu_ps((float*)ptrC, tmp1); ptrC -= 8;
        auto tmp2 = _mm256_permutevar_ps(_mm256_load_ps((const float*)ptrA), indexer),
            tmp7 = _mm256_permutevar_ps(_mm256_load_ps((const float*)ptrC), indexer);
        tmp2 = _mm256_permute2f128_ps(tmp2, tmp2, 0x01), tmp7 = _mm256_permute2f128_ps(tmp7, tmp7, 0x01);
        _mm256_store_ps((float*)ptrA, tmp7); ptrA += 8;
        _mm256_storeu_ps((float*)ptrC, tmp2); ptrC -= 8;
        auto tmp3 = _mm256_permutevar_ps(_mm256_load_ps((const float*)ptrA), indexer),
            tmp6 = _mm256_permutevar_ps(_mm256_load_ps((const float*)ptrC), indexer);
        tmp3 = _mm256_permute2f128_ps(tmp3, tmp3, 0x01), tmp6 = _mm256_permute2f128_ps(tmp6, tmp6, 0x01);
        _mm256_store_ps((float*)ptrA, tmp6); ptrA += 8;
        _mm256_storeu_ps((float*)ptrC, tmp3); ptrC -= 8;
        auto tmp4 = _mm256_permutevar_ps(_mm256_load_ps((const float*)ptrA), indexer),
            tmp5 = _mm256_permutevar_ps(_mm256_load_ps((const float*)ptrC), indexer);
        tmp4 = _mm256_permute2f128_ps(tmp4, tmp4, 0x01), tmp5 = _mm256_permute2f128_ps(tmp5, tmp5, 0x01);
        _mm256_store_ps((float*)ptrA, tmp5); ptrA += 8;
        _mm256_storeu_ps((float*)ptrC, tmp4); ptrC -= 8;
        count -= 32;
        _mm_prefetch((const char*)ptrA, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrA + 64, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC - 64, _MM_HINT_NTA);
    }
    while (count > 8)
    {
        auto tmpA = _mm256_permutevar_ps(_mm256_load_ps((const float*)ptrA), indexer),
            tmpC = _mm256_permutevar_ps(_mm256_load_ps((const float*)ptrC), indexer);
        tmpA = _mm256_permute2f128_ps(tmpA, tmpA, 0x01), tmpC = _mm256_permute2f128_ps(tmpC, tmpC, 0x01);
        _mm256_store_ps((float*)ptrA, tmpC); ptrA += 8;
        _mm256_storeu_ps((float*)ptrC, tmpA); ptrC -= 8;
        count -= 4;
    }
    ptrB = ptrC + 7;
    REV_BLOCK(count)
#elif COMMON_SIMD_LV >= 20
    uint32_t * __restrict ptrC = ptrB - 3;
    while (count > 16)
    {
        const auto tmp1 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*)ptrA), 0b00011011),
            tmp8 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*)ptrC), 0b00011011);
        _mm_store_si128((__m128i*)ptrA, tmp8); ptrA += 4;
        _mm_storeu_si128((__m128i*)ptrC, tmp1); ptrC -= 4;
        const auto tmp2 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*)ptrA), 0b00011011),
            tmp7 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*)ptrC), 0b00011011);
        _mm_store_si128((__m128i*)ptrA, tmp7); ptrA += 4;
        _mm_storeu_si128((__m128i*)ptrC, tmp2); ptrC -= 4;
        const auto tmp3 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*)ptrA), 0b00011011),
            tmp6 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*)ptrC), 0b00011011);
        _mm_store_si128((__m128i*)ptrA, tmp6); ptrA += 4;
        _mm_storeu_si128((__m128i*)ptrC, tmp3); ptrC -= 4;
        const auto tmp4 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*)ptrA), 0b00011011),
            tmp5 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*)ptrC), 0b00011011);
        _mm_store_si128((__m128i*)ptrA, tmp5); ptrA += 4;
        _mm_storeu_si128((__m128i*)ptrC, tmp4); ptrC -= 4;
        count -= 16;
        _mm_prefetch((const char*)ptrA, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC, _MM_HINT_NTA);
    }
    while (count > 4)
    {
        const auto tmpA = _mm_shuffle_epi32(_mm_load_si128((const __m128i*)ptrA), 0b00011011),
            tmpC = _mm_shuffle_epi32(_mm_load_si128((const __m128i*)ptrC), 0b00011011);
        _mm_store_si128((__m128i*)ptrA, tmpC); ptrA += 4;
        _mm_storeu_si128((__m128i*)ptrC, tmpA); ptrC -= 4;
        count -= 4;
    }
    ptrB = ptrC + 3;
    REV_BLOCK(count)
#else
    REV_BLOCK(count)
#endif
    return true;
}
#undef REV_BLOCK
#pragma endregion REVERSE one buffer(per 4byte)


#pragma region REVERSE one buffer(per 3byte)
#define LOOP_REV_BLOCK3 \
        { \
            const auto tmp0 = ptrA[0], tmp1 = ptrA[1], tmp2 = ptrA[2]; \
            ptrA[0] = ptrB[0], ptrA[1] = ptrB[1], ptrA[2] = ptrB[2]; \
            ptrB[0] = tmp0, ptrB[1] = tmp1, ptrB[2] = tmp2; \
            ptrA +=3, ptrB -=3, count --; \
        } 
inline bool ReverseBuffer3(byte * __restrict ptr, uint64_t count)
{
    uint8_t * __restrict ptrA = reinterpret_cast<uint8_t*>(ptr);
    uint8_t * __restrict ptrB = reinterpret_cast<uint8_t*>(ptr) + (count - 1) * 3;
    count = count / 2;
    if (count == 0)
        return false;
    while (count)
    {
        switch (count)
        {
        default:LOOP_REV_BLOCK3
        case 7: LOOP_REV_BLOCK3
        case 6: LOOP_REV_BLOCK3
        case 5: LOOP_REV_BLOCK3
        case 4: LOOP_REV_BLOCK3
        case 3: LOOP_REV_BLOCK3
        case 2: LOOP_REV_BLOCK3
        case 1: LOOP_REV_BLOCK3
        }
    }
    return true;
}
#undef LOOP_REV_BLOCK3
#pragma endregion REVERSE one buffer(per 3byte)




}