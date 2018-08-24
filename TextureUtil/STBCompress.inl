#pragma once

#include "TexUtilRely.h"
#include <boost/detail/endian.hpp>
#if BOOST_BYTE_ORDER != 1234
#   pragma message("unsupported byte order (non little endian), fallback to SIMD_LV = 0")
#   undef COMMON_SIMD_LV
#   define COMMON_SIMD_LV 0
#endif
#include "common/SIMD.hpp"
#define STB_DXT_IMPLEMENTATION
#include "3rdParty/stblib/stb_dxt.h"
#include "OpenGLUtil/oglException.h"


namespace oglu::texutil::detail
{

//SIMD optimized according to stblib's implementation
static forceinline void CompressBC5Block(uint8_t * __restrict dest, uint8_t * __restrict buffer)
{
#if COMMON_SIMD_LV < 31
    return stb_compress_bc5_block(dest, buffer);
#else
    //12345678,abcdefgh
    const auto l12 = _mm_load_si128(reinterpret_cast<const __m128i*>(buffer)), l34 = _mm_load_si128(reinterpret_cast<const __m128i*>(buffer) + 1);
    const auto max1 = _mm_max_epu8(l12, l34), min1 = _mm_min_epu8(l12, l34);//12345678
    const auto max1s = _mm_shuffle_epi32(max1, 0b01001110), min1s = _mm_shuffle_epi32(min1, 0b01001110);//56781234
    const auto max2 = _mm_max_epu8(max1, max1s), min2 = _mm_min_epu8(min1, min1s);//1234
    const auto max2s = _mm_shufflelo_epi16(max2, 0b01001110), min2s = _mm_shufflelo_epi16(min2, 0b01001110);//3412
    const auto max3 = _mm_max_epu8(max2, max2s), min3 = _mm_min_epu8(min2, min2s);//1212
    const auto max3s = _mm_shufflelo_epi16(max3, 0b00010001), min3s = _mm_shufflelo_epi16(min3, 0b00010001);//2121
    const auto max4 = _mm_max_epu8(max3, max3s), min4 = _mm_min_epu8(min3, min3s);//1111
    const uint32_t ret = _mm_cvtsi128_si32(_mm_unpacklo_epi16(max4, min4));//Rmax,Gmax,Rmin,Gmin
    const uint8_t Rmax = static_cast<uint8_t>(ret), Gmax = static_cast<uint8_t>(ret >> 8),
        Rmin = static_cast<uint8_t>(ret >> 16), Gmin = static_cast<uint8_t>(ret >> 24);

    dest[0] = Rmax, dest[1] = Rmin;
    dest[8] = Gmax, dest[9] = Gmin;
#   if COMMON_SIMD_LV >= 200
    const auto blk = _mm256_set_m128i(l34, l12), num7_16 = _mm256_set1_epi16(7);
    __m256i Ri1234, Gi1234;
    if (const auto Rdist = Rmax - Rmin; Rdist > 0)
    {
        auto R1234 = _mm256_sub_epi16(_mm256_and_si256(blk, _mm256_set1_epi16(0xff)), _mm256_set1_epi16(Rmin));
        const auto Rbias = Rdist < 8 ? (Rdist - 1) : (Rdist / 2 + 2);
        R1234 = _mm256_add_epi16(_mm256_mullo_epi16(R1234, num7_16), _mm256_set1_epi16(static_cast<int16_t>(Rbias)));
        const auto Rdist1 = _mm256_set1_epi16(static_cast<int16_t>(UINT16_MAX / Rdist));
        Ri1234 = _mm256_mulhi_epi16(R1234, Rdist1);
    }
    else
        Ri1234 = _mm256_setzero_si256();

    if (const auto Gdist = Gmax - Gmin; Gdist > 0)
    {
        const auto SHUF_MSK_G = _mm256_setr_epi8(1, -1, 3, -1, 5, -1, 7, -1, 9, -1, 11, -1, 13, -1, 15, -1, 1, -1, 3, -1, 5, -1, 7, -1, 9, -1, 11, -1, 13, -1, 15, -1);
        auto G1234 = _mm256_sub_epi16(_mm256_shuffle_epi8(blk, SHUF_MSK_G), _mm256_set1_epi16(Gmin));
        const auto Gbias = Gdist < 8 ? (Gdist - 1) : (Gdist / 2 + 2);
        G1234 = _mm256_add_epi16(_mm256_mullo_epi16(G1234, num7_16), _mm256_set1_epi16(static_cast<int16_t>(Gbias)));
        const auto Gdist1 = _mm256_set1_epi16(static_cast<int16_t>(UINT16_MAX / Gdist));
        Gi1234 = _mm256_mulhi_epi16(G1234, Gdist1);
    }
    else
        Gi1234 = _mm256_setzero_si256();

    const auto SHUF_MSK = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 4, 6, 8, 10, 12, 14);
    const auto Rtmp = _mm256_shuffle_epi8(Ri1234, SHUF_MSK), Gtmp = _mm256_shuffle_epi8(Gi1234, SHUF_MSK);
    auto RGidx = _mm256_or_si256(_mm256_permute2x128_si256(Rtmp, Gtmp, 0x20), _mm256_permute2x128_si256(Rtmp, Gtmp, 0x31));

    const auto num0 = _mm256_setzero_si256(), num7 = _mm256_set1_epi8(7);
    RGidx = _mm256_and_si256(_mm256_sub_epi8(num0, RGidx), num7);
    const auto num2 = _mm256_set1_epi8(2), num1 = _mm256_set1_epi8(1);
    RGidx = _mm256_xor_si256(_mm256_and_si256(_mm256_cmpgt_epi8(num2, RGidx), num1), RGidx);
    _mm256_store_si256(reinterpret_cast<__m256i*>(buffer), RGidx);
#   else
    __m128i Ri12, Ri34, Gi12, Gi34, num7_16 = _mm_set1_epi16(7);
    if (const auto Rdist = Rmax - Rmin; Rdist > 0)
    {
        const auto BLEND_MSK_R = _mm_set1_epi16(0xff), Rmin8 = _mm_set1_epi16(Rmin);
        auto R12 = _mm_sub_epi16(_mm_and_si128(l12, BLEND_MSK_R), Rmin8);
        auto R34 = _mm_sub_epi16(_mm_and_si128(l34, BLEND_MSK_R), Rmin8);
        const auto Rbias = Rdist < 8 ? (Rdist - 1) : (Rdist / 2 + 2);
        const auto Rbias8 = _mm_set1_epi16(static_cast<int16_t>(Rbias));
        R12 = _mm_add_epi16(_mm_mullo_epi16(R12, num7_16), Rbias8), R34 = _mm_add_epi16(_mm_mullo_epi16(R34, num7_16), Rbias8);
        const auto Rdist1 = _mm_set1_epi16(static_cast<int16_t>(UINT16_MAX / Rdist));
        Ri12 = _mm_mulhi_epi16(R12, Rdist1), Ri34 = _mm_mulhi_epi16(R34, Rdist1);
    }
    else
        Ri12 = Ri34 = _mm_setzero_si128();

    if (const auto Gdist = Gmax - Gmin; Gdist > 0)
    {
        const auto SHUF_MSK_G = _mm_setr_epi8(1, -1, 3, -1, 5, -1, 7, -1, 9, -1, 11, -1, 13, -1, 15, -1), Gmin8 = _mm_set1_epi16(Gmin);
        auto G12 = _mm_sub_epi16(_mm_shuffle_epi8(l12, SHUF_MSK_G), Gmin8);
        auto G34 = _mm_sub_epi16(_mm_shuffle_epi8(l34, SHUF_MSK_G), Gmin8);
        const auto Gbias = Gdist < 8 ? (Gdist - 1) : (Gdist / 2 + 2);
        const auto Gbias8 = _mm_set1_epi16(static_cast<int16_t>(Gbias));
        G12 = _mm_add_epi16(_mm_mullo_epi16(G12, num7_16), Gbias8), G34 = _mm_add_epi16(_mm_mullo_epi16(G34, num7_16), Gbias8);
        const auto Gdist1 = _mm_set1_epi16(static_cast<int16_t>(UINT16_MAX / Gdist));
        Gi12 = _mm_mulhi_epi16(G12, Gdist1), Gi34 = _mm_mulhi_epi16(G34, Gdist1);
    }
    else
        Gi12 = Gi34 = _mm_setzero_si128();

    const auto SHUF_MSK_12 = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto SHUF_MSK_34 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 4, 6, 8, 10, 12, 14);
    auto Ridx = _mm_or_si128(_mm_shuffle_epi8(Ri12, SHUF_MSK_12), _mm_shuffle_epi8(Ri34, SHUF_MSK_34));
    auto Gidx = _mm_or_si128(_mm_shuffle_epi8(Gi12, SHUF_MSK_12), _mm_shuffle_epi8(Gi34, SHUF_MSK_34));

    const auto num0 = _mm_setzero_si128(), num7 = _mm_set1_epi8(7);
    Ridx = _mm_and_si128(_mm_sub_epi8(num0, Ridx), num7), Gidx = _mm_and_si128(_mm_sub_epi8(num0, Gidx), num7);
    const auto num2 = _mm_set1_epi8(2), num1 = _mm_set1_epi8(1);
    Ridx = _mm_xor_si128(_mm_and_si128(_mm_cmpgt_epi8(num2, Ridx), num1), Ridx), Gidx = _mm_xor_si128(_mm_and_si128(_mm_cmpgt_epi8(num2, Gidx), num1), Gidx);
    _mm_store_si128(reinterpret_cast<__m128i*>(buffer), Ridx), _mm_store_si128(reinterpret_cast<__m128i*>(buffer) + 1, Gidx);
#   endif
    *reinterpret_cast<uint16_t*>(&dest[2]) = static_cast<uint16_t>((buffer[0] >> 0) | (buffer[1] << 3) | (buffer[2] << 6) | (buffer[3] << 9) | (buffer[4] << 12) | (buffer[5] << 15));
    *reinterpret_cast<uint16_t*>(&dest[4]) = static_cast<uint16_t>((buffer[5] >> 1) | (buffer[6] << 2) | (buffer[7] << 5) | (buffer[8] << 8) | (buffer[9] << 11) | (buffer[10] << 14));
    *reinterpret_cast<uint16_t*>(&dest[6]) = static_cast<uint16_t>((buffer[10] >> 2) | (buffer[11] << 1) | (buffer[12] << 4) | (buffer[13] << 7) | (buffer[14] << 10) | (buffer[15] << 13));
    dest += 8, buffer += 16; 
    *reinterpret_cast<uint16_t*>(&dest[2]) = static_cast<uint16_t>((buffer[0] >> 0) | (buffer[1] << 3) | (buffer[2] << 6) | (buffer[3] << 9) | (buffer[4] << 12) | (buffer[5] << 15));
    *reinterpret_cast<uint16_t*>(&dest[4]) = static_cast<uint16_t>((buffer[5] >> 1) | (buffer[6] << 2) | (buffer[7] << 5) | (buffer[8] << 8) | (buffer[9] << 11) | (buffer[10] << 14));
    *reinterpret_cast<uint16_t*>(&dest[6]) = static_cast<uint16_t>((buffer[10] >> 2) | (buffer[11] << 1) | (buffer[12] << 4) | (buffer[13] << 7) | (buffer[14] << 10) | (buffer[15] << 13));
#endif
}


struct alignas(32) BCBlock
{
private:
    uint8_t data[64];
public:
    static forceinline void RGBA2RGBA(const uint8_t * __restrict row1, const size_t stride, uint8_t * __restrict output)
    {
#if COMMON_SIMD_LV >= 100
        _mm256_store_ps(reinterpret_cast<float*>(output +  0), _mm256_loadu2_m128(reinterpret_cast<const float*>(row1 + 0), reinterpret_cast<const float*>(row1 + stride)));
        _mm256_store_ps(reinterpret_cast<float*>(output + 32), _mm256_loadu2_m128(reinterpret_cast<const float*>(row1 + stride * 2), reinterpret_cast<const float*>(row1 + stride * 3)));
#elif COMMON_SIMD_LV >= 10
        _mm_store_ps(reinterpret_cast<float*>(output +  0), _mm_loadu_ps(reinterpret_cast<const float*>(row1 + 0)));
        _mm_store_ps(reinterpret_cast<float*>(output + 16), _mm_loadu_ps(reinterpret_cast<const float*>(row1 + stride)));
        _mm_store_ps(reinterpret_cast<float*>(output + 32), _mm_loadu_ps(reinterpret_cast<const float*>(row1 + stride * 2)));
        _mm_store_ps(reinterpret_cast<float*>(output + 48), _mm_loadu_ps(reinterpret_cast<const float*>(row1 + stride * 3)));
#else
        memcpy_s(output +  0, 16, row1, 16);
        memcpy_s(output + 16, 16, row1 + stride, 16);
        memcpy_s(output + 32, 16, row1 + stride * 2, 16);
        memcpy_s(output + 48, 16, row1 + stride * 3, 16);
#endif
    }

    static forceinline void RGBA2RG(const uint8_t * __restrict row1, const size_t stride, uint8_t * __restrict output)
    {
#if COMMON_SIMD_LV >= 200
        const auto SHUF_MSK = _mm256_setr_epi8(0, 1, 4, 5, 8, 9, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 4, 5, 8, 9, 12, 13);
        const auto src0 = _mm256_loadu2_m128i(reinterpret_cast<const __m128i*>(row1 + 0), reinterpret_cast<const __m128i*>(row1 + stride));
        const auto src1 = _mm256_loadu2_m128i(reinterpret_cast<const __m128i*>(row1 + stride * 2), reinterpret_cast<const __m128i*>(row1 + stride * 3));
        const auto middle0 = _mm256_shuffle_epi8(src0, SHUF_MSK);//a0,0b
        const auto middle1 = _mm256_shuffle_epi8(src1, SHUF_MSK);//c0,0d
        const auto abab = _mm256_permute4x64_epi64(middle0, 0b11001100);
        const auto cdcd = _mm256_permute4x64_epi64(middle1, 0b11001100);
        _mm256_store_si256(reinterpret_cast<__m256i*>(output), _mm256_blend_epi32(abab, cdcd, 0b11110000));
#elif COMMON_SIMD_LV >= 20
        const auto SHUF_MSK = _mm_setr_epi8(0, 1, 4, 5, 8, 9, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1);
        const auto src0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + 0));
        const auto src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + stride));
        const auto src2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + stride * 2));
        const auto src3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + stride * 3));
        const auto middle0 = _mm_shuffle_epi8(src0, SHUF_MSK);//a0
        const auto middle1 = _mm_shuffle_epi8(src1, SHUF_MSK);//b0
        const auto middle2 = _mm_shuffle_epi8(src2, SHUF_MSK);//c0
        const auto middle3 = _mm_shuffle_epi8(src3, SHUF_MSK);//d0
        _mm_store_si128(reinterpret_cast<__m128i*>(output + 0), _mm_unpacklo_epi64(middle0, middle1));
        _mm_store_si128(reinterpret_cast<__m128i*>(output + 16), _mm_unpacklo_epi64(middle2, middle3));
#else
        uint16_t * __restrict dest = reinterpret_cast<uint16_t*>(output);
        {
            const uint32_t * __restrict src = reinterpret_cast<const uint32_t*>(row1);
            *dest++ = static_cast<uint16_t>(src[0]);
            *dest++ = static_cast<uint16_t>(src[1]);
            *dest++ = static_cast<uint16_t>(src[2]);
            *dest++ = static_cast<uint16_t>(src[3]);
        }
        {
            const uint32_t * __restrict src = reinterpret_cast<const uint32_t*>(row1 + stride);
            *dest++ = static_cast<uint16_t>(src[0]);
            *dest++ = static_cast<uint16_t>(src[1]);
            *dest++ = static_cast<uint16_t>(src[2]);
            *dest++ = static_cast<uint16_t>(src[3]);
        }
        {
            const uint32_t * __restrict src = reinterpret_cast<const uint32_t*>(row1 + stride * 2);
            *dest++ = static_cast<uint16_t>(src[0]);
            *dest++ = static_cast<uint16_t>(src[1]);
            *dest++ = static_cast<uint16_t>(src[2]);
            *dest++ = static_cast<uint16_t>(src[3]);
        }
        {
            const uint32_t * __restrict src = reinterpret_cast<const uint32_t*>(row1 + stride * 3);
            *dest++ = static_cast<uint16_t>(src[0]);
            *dest++ = static_cast<uint16_t>(src[1]);
            *dest++ = static_cast<uint16_t>(src[2]);
            *dest++ = static_cast<uint16_t>(src[3]);
        }
#endif
    }

    static forceinline void RGB2RG(const uint8_t * __restrict row1, const size_t stride, uint8_t * __restrict output)
    {
#if COMMON_SIMD_LV >= 200
        const auto SHUF_MSK = _mm256_setr_epi8(0, 1, 3, 4, 6, 7, 9, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 3, 4, 6, 7, 9, 10);
        const auto src0 = _mm256_loadu2_m128i(reinterpret_cast<const __m128i*>(row1 + 0), reinterpret_cast<const __m128i*>(row1 + stride));
        const auto src1 = _mm256_loadu2_m128i(reinterpret_cast<const __m128i*>(row1 + stride * 2), reinterpret_cast<const __m128i*>(row1 + stride * 3));
        const auto middle0 = _mm256_shuffle_epi8(src0, SHUF_MSK);//a0,0b
        const auto middle1 = _mm256_shuffle_epi8(src1, SHUF_MSK);//c0,0d
        const auto abab = _mm256_permute4x64_epi64(middle0, 0b11001100);
        const auto cdcd = _mm256_permute4x64_epi64(middle1, 0b11001100);
        _mm256_store_si256(reinterpret_cast<__m256i*>(output), _mm256_blend_epi32(abab, cdcd, 0b11110000));
#elif COMMON_SIMD_LV >= 20
        const auto SHUF_MSK = _mm_setr_epi8(0, 1, 3, 4, 6, 7, 9, 10, -1, -1, -1, -1, -1, -1, -1, -1);
        const auto src0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + 0));
        const auto src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + stride));
        const auto src2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + stride * 2));
        const auto src3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + stride * 3));
        const auto middle0 = _mm_shuffle_epi8(src0, SHUF_MSK);//a0
        const auto middle1 = _mm_shuffle_epi8(src1, SHUF_MSK);//b0
        const auto middle2 = _mm_shuffle_epi8(src2, SHUF_MSK);//c0
        const auto middle3 = _mm_shuffle_epi8(src3, SHUF_MSK);//d0
        _mm_store_si128(reinterpret_cast<__m128i*>(output + 0), _mm_unpacklo_epi64(middle0, middle1));
        _mm_store_si128(reinterpret_cast<__m128i*>(output + 16), _mm_unpacklo_epi64(middle2, middle3));
#else
        *output++ = row1[0]; *output++ = row1[1];
        *output++ = row1[3]; *output++ = row1[4];
        *output++ = row1[6]; *output++ = row1[7];
        *output++ = row1[9]; *output++ = row1[10];
        row1 += stride;
        *output++ = row1[0]; *output++ = row1[1];
        *output++ = row1[3]; *output++ = row1[4];
        *output++ = row1[6]; *output++ = row1[7];
        *output++ = row1[9]; *output++ = row1[10];
        row1 += stride;
        *output++ = row1[0]; *output++ = row1[1];
        *output++ = row1[3]; *output++ = row1[4];
        *output++ = row1[6]; *output++ = row1[7];
        *output++ = row1[9]; *output++ = row1[10];
        row1 += stride;
        *output++ = row1[0]; *output++ = row1[1];
        *output++ = row1[3]; *output++ = row1[4];
        *output++ = row1[6]; *output++ = row1[7];
        *output++ = row1[9]; *output++ = row1[10];
#endif
    }

    static forceinline void RG2RG(const uint8_t * __restrict row1, const size_t stride, uint8_t * __restrict output)
    {
#if COMMON_SIMD_LV >= 20
        const auto src0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + 0));
        const auto src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + stride));
        const auto src2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + stride * 2));
        const auto src3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + stride * 3));
        _mm_store_si128(reinterpret_cast<__m128i*>(output + 0), _mm_unpacklo_epi64(src0, src1));
        _mm_store_si128(reinterpret_cast<__m128i*>(output + 16), _mm_unpacklo_epi64(src2, src3));
#else
        memcpy_s(output + 0, 8, row1, 8);
        memcpy_s(output + 16, 8, row1 + stride, 8);
        memcpy_s(output + 32, 8, row1 + stride * 2, 8);
        memcpy_s(output + 48, 8, row1 + stride * 3, 8);
#endif
    }

    static forceinline void R2RG(const uint8_t * __restrict row1, const size_t stride, uint8_t * __restrict output)
    {
#if COMMON_SIMD_LV >= 41
        const auto src0 = _mm_cvtepu8_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + 0)));
        const auto src1 = _mm_cvtepu8_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + stride)));
        const auto src2 = _mm_cvtepu8_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + stride * 2)));
        const auto src3 = _mm_cvtepu8_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(row1 + stride * 3)));
        _mm_store_si128(reinterpret_cast<__m128i*>(output + 0), _mm_unpacklo_epi64(src0, src1));
        _mm_store_si128(reinterpret_cast<__m128i*>(output + 16), _mm_unpacklo_epi64(src2, src3));
#else
        uint16_t * __restrict dest = reinterpret_cast<uint16_t*>(output);
        *dest++ = row1[0]; *dest++ = row1[1];
        *dest++ = row1[3]; *dest++ = row1[4];
        row1 += stride;
        *dest++ = row1[0]; *dest++ = row1[1];
        *dest++ = row1[3]; *dest++ = row1[4];
        row1 += stride;
        *dest++ = row1[0]; *dest++ = row1[1];
        *dest++ = row1[3]; *dest++ = row1[4];
        row1 += stride;
        *dest++ = row1[0]; *dest++ = row1[1];
        *dest++ = row1[3]; *dest++ = row1[4];
#endif
    }

    template<typename Prepare, typename Process>
    common::AlignedBuffer<32> EachBlock(const Image& img, const size_t bytePerBlock, Prepare&& prepare, Process&& process)
    {
        common::AlignedBuffer<32> buffer(bytePerBlock * (img.GetWidth() * img.GetHeight() / 4 / 4));
        const auto blockStride = img.GetElementSize() * 4;
        const auto rowStride = img.GetWidth() * img.GetElementSize();
        const uint8_t * __restrict row = img.GetRawPtr<uint8_t>();
        uint8_t * __restrict output = buffer.GetRawPtr<uint8_t>();

        for (uint32_t y = img.GetHeight(); y > 0; y -= 4)
        {
            for (uint32_t x = img.GetWidth(); x > 0; x -= 4)
            {
                prepare(row, rowStride, data);
                process(output, data);
                row += blockStride;
                output += bytePerBlock;
            }
            row += rowStride * 3;
        }
        return buffer;
    }
};


static common::AlignedBuffer<32> CompressBC5(const Image& img)
{
    if (HAS_FIELD(img.GetDataType(), ImageDataType::FLOAT_MASK))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"float data type not supported in BC5");

    BCBlock block;
    switch (img.GetDataType())
    {
    case ImageDataType::RGBA:
        //return block.EachBlock(img, 16, BCBlock::RGBA2RG, stb_compress_bc5_block);
        return block.EachBlock(img, 16, BCBlock::RGBA2RG, CompressBC5Block);
    case ImageDataType::RGB:
        //return block.EachBlock(img, 16, BCBlock::RGB2RG, stb_compress_bc5_block);
        return block.EachBlock(img, 16, BCBlock::RGB2RG, CompressBC5Block);
    case ImageDataType::RA:
        //return block.EachBlock(img, 16, BCBlock::RG2RG, stb_compress_bc5_block);
        return block.EachBlock(img, 16, BCBlock::RG2RG, CompressBC5Block);
    case ImageDataType::GRAY:
        //return block.EachBlock(img, 16, BCBlock::R2RG, stb_compress_bc5_block);
        return block.EachBlock(img, 16, BCBlock::R2RG, CompressBC5Block);
    default:
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"error datatype for Image");
    }
}


}
