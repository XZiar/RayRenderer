#pragma once

#include "TexUtilRely.h"
#include "common/SIMD.hpp"
#define STB_DXT_IMPLEMENTATION
#include "3rdParty/stblib/stb_dxt.h"
#include "OpenGLUtil/oglException.h"

//#define COMMON_SIMD_LV 200
namespace oglu::texutil::detail
{

//SIMD optimized according to stblib's implementation
static forceinline void CompressBC5Block(uint8_t * __restrict dest, uint8_t * __restrict buffer)
{
#if COMMON_SIMD_LV < 31
    stb_compress_bc5_block(dest, buffer);
#else
    const auto backup = _MM_GET_ROUNDING_MODE();
    _MM_SET_ROUNDING_MODE(_MM_ROUND_TOWARD_ZERO);
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
    const auto num7F = _mm256_set1_ps(7);

    const auto SHUF_MSK_R = _mm256_setr_epi8(0, -1, -1, -1, 2, -1, -1, -1, 4, -1, -1, -1, 6, -1, -1, -1, 8, -1, -1, -1, 10, -1, -1, -1, 12, -1, -1, -1, 14, -1, -1, -1);
    const auto Rdist = Rmax - Rmin;
    const auto Rbias = (Rdist < 8 ? (Rdist - 1) : (Rdist / 2 + 2)) - Rmin * 7;
    const auto RbiasF = _mm256_set1_ps(static_cast<int16_t>(Rbias));
    const auto R12 = _mm256_fmadd_ps(_mm256_cvtepi32_ps(_mm256_shuffle_epi8(_mm256_broadcastsi128_si256(l12), SHUF_MSK_R)), num7F, RbiasF),
        R34 = _mm256_fmadd_ps(_mm256_cvtepi32_ps(_mm256_shuffle_epi8(_mm256_broadcastsi128_si256(l34), SHUF_MSK_R)), num7F, RbiasF);
    
    const auto Rdist1F = _mm256_broadcastss_ps(_mm_rcp_ss(_mm_set1_ps(static_cast<int16_t>(Rdist))));
    const auto Ri12 = _mm256_cvtps_epi32(_mm256_mul_ps(R12, Rdist1F)), Ri34 = _mm256_cvtps_epi32(_mm256_mul_ps(R34, Rdist1F));

    const auto SHUF_MSK_G = _mm256_setr_epi8(1, -1, -1, -1, 3, -1, -1, -1, 5, -1, -1, -1, 7, -1, -1, -1, 9, -1, -1, -1, 11, -1, -1, -1, 13, -1, -1, -1, 15, -1, -1, -1);
    const auto Gdist = Gmax - Gmin;
    const auto Gbias = (Gdist < 8 ? (Gdist - 1) : (Gdist / 2 + 2)) - Gmin * 7;
    const auto GbiasF = _mm256_set1_ps(static_cast<int16_t>(Gbias));
    const auto G12 = _mm256_fmadd_ps(_mm256_cvtepi32_ps(_mm256_shuffle_epi8(_mm256_broadcastsi128_si256(l12), SHUF_MSK_G)), num7F, GbiasF),
        G34 = _mm256_fmadd_ps(_mm256_cvtepi32_ps(_mm256_shuffle_epi8(_mm256_broadcastsi128_si256(l34), SHUF_MSK_G)), num7F, GbiasF);

    const auto Gdist1F = _mm256_broadcastss_ps(_mm_rcp_ss(_mm_set1_ps(static_cast<int16_t>(Gdist))));
    const auto Gi12 = _mm256_cvtps_epi32(_mm256_mul_ps(G12, Gdist1F)), Gi34 = _mm256_cvtps_epi32(_mm256_mul_ps(G34, Gdist1F));

    const auto SHUF_MSK_12 = _mm256_setr_epi8(0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1),
        SHUF_MSK_34 = _mm256_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12);
    const auto Rtmp = _mm256_or_si256(_mm256_shuffle_epi8(Ri12, SHUF_MSK_12), _mm256_shuffle_epi8(Ri34, SHUF_MSK_34)),
        Gtmp = _mm256_or_si256(_mm256_shuffle_epi8(Gi12, SHUF_MSK_12), _mm256_shuffle_epi8(Gi34, SHUF_MSK_34));
    auto RGidx = _mm256_or_si256(_mm256_permute2x128_si256(Rtmp, Gtmp, 0x20), _mm256_permute2x128_si256(Rtmp, Gtmp, 0x31));

    const auto num0 = _mm256_setzero_si256(), num7 = _mm256_set1_epi8(7);
    RGidx = _mm256_and_si256(_mm256_sub_epi8(num0, RGidx), num7);
    const auto num2 = _mm256_set1_epi8(2), num1 = _mm256_set1_epi8(1);
    RGidx = _mm256_xor_si256(_mm256_and_si256(_mm256_cmpgt_epi8(num2, RGidx), num1), RGidx);
    _mm256_store_si256(reinterpret_cast<__m256i*>(buffer), RGidx);
#   else
    const auto num7F = _mm_set1_ps(7);
    const auto SHUF_MSK_R1 = _mm_setr_epi8(0, -1, -1, -1, 2, -1, -1, -1, 4, -1, -1, -1, 6, -1, -1, -1),
        SHUF_MSK_R2 = _mm_setr_epi8(8, -1, -1, -1, 10, -1, -1, -1, 12, -1, -1, -1, 14, -1, -1, -1);
    const auto R1 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(l12, SHUF_MSK_R1)), num7F),
        R2 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(l12, SHUF_MSK_R2)), num7F),
        R3 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(l34, SHUF_MSK_R1)), num7F),
        R4 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(l34, SHUF_MSK_R2)), num7F);

    const auto Rdist = Rmax - Rmin;
    const auto Rbias = (Rdist < 8 ? (Rdist - 1) : (Rdist / 2 + 2)) - Rmin * 7;
    const auto RbiasF = _mm_set1_ps(static_cast<int16_t>(Rbias)), Rdist1F = _mm_rcp_ps(_mm_set1_ps(static_cast<int16_t>(Rdist)));
    const auto Ri1 = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(R1, RbiasF), Rdist1F)),
        Ri2 = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(R2, RbiasF), Rdist1F)),
        Ri3 = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(R3, RbiasF), Rdist1F)),
        Ri4 = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(R4, RbiasF), Rdist1F));

    const auto SHUF_MSK_G1 = _mm_setr_epi8(1, -1, -1, -1, 3, -1, -1, -1, 5, -1, -1, -1, 7, -1, -1, -1),
        SHUF_MSK_G2 = _mm_setr_epi8(9, -1, -1, -1, 11, -1, -1, -1, 13, -1, -1, -1, 15, -1, -1, -1);
    const auto G1 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(l12, SHUF_MSK_G1)), num7F),
        G2 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(l12, SHUF_MSK_G2)), num7F),
        G3 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(l34, SHUF_MSK_G1)), num7F),
        G4 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(l34, SHUF_MSK_G2)), num7F);

    const auto Gdist = Gmax - Gmin;
    const auto Gbias = (Gdist < 8 ? (Gdist - 1) : (Gdist / 2 + 2)) - Gmin * 7;
    const auto GbiasF = _mm_set1_ps(static_cast<int16_t>(Gbias)), Gdist1F = _mm_rcp_ps(_mm_set1_ps(static_cast<int16_t>(Gdist)));
    const auto Gi1 = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(G1, GbiasF), Gdist1F)),
        Gi2 = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(G2, GbiasF), Gdist1F)),
        Gi3 = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(G3, GbiasF), Gdist1F)),
        Gi4 = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(G4, GbiasF), Gdist1F));

    const auto SHUF_MSK_1 = _mm_setr_epi8(0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
        SHUF_MSK_2 = _mm_setr_epi8(-1, -1, -1, -1, 0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1),
        SHUF_MSK_3 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12, -1, -1, -1, -1),
        SHUF_MSK_4 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12);
    auto Ridx = _mm_or_si128(_mm_or_si128(_mm_shuffle_epi8(Ri1, SHUF_MSK_1), _mm_shuffle_epi8(Ri2, SHUF_MSK_2)),
        _mm_or_si128(_mm_shuffle_epi8(Ri3, SHUF_MSK_3), _mm_shuffle_epi8(Ri4, SHUF_MSK_4)));
    auto Gidx = _mm_or_si128(_mm_or_si128(_mm_shuffle_epi8(Gi1, SHUF_MSK_1), _mm_shuffle_epi8(Gi2, SHUF_MSK_2)),
        _mm_or_si128(_mm_shuffle_epi8(Gi3, SHUF_MSK_3), _mm_shuffle_epi8(Gi4, SHUF_MSK_4)));

    const auto num0 = _mm_setzero_si128(), num7 = _mm_set1_epi8(7);
    Ridx = _mm_and_si128(_mm_sub_epi8(num0, Ridx), num7), Gidx = _mm_and_si128(_mm_sub_epi8(num0, Gidx), num7);
    const auto num2 = _mm_set1_epi8(2), num1 = _mm_set1_epi8(1);
    Ridx = _mm_xor_si128(_mm_and_si128(_mm_cmpgt_epi8(num2, Ridx), num1), Ridx), Gidx = _mm_xor_si128(_mm_and_si128(_mm_cmpgt_epi8(num2, Gidx), num1), Gidx);
    _mm_store_si128(reinterpret_cast<__m128i*>(buffer), Ridx), _mm_store_si128(reinterpret_cast<__m128i*>(buffer) + 1, Gidx);
#   endif
    dest[2] = static_cast<uint8_t>((buffer[0] >> 0) | (buffer[1] << 3) | (buffer[2] << 6));
    dest[3] = static_cast<uint8_t>((buffer[2] >> 2) | (buffer[3] << 1) | (buffer[4] << 4) | (buffer[5] << 7));
    dest[4] = static_cast<uint8_t>((buffer[5] >> 1) | (buffer[6] << 2) | (buffer[7] << 5));
    dest[5] = static_cast<uint8_t>((buffer[8] >> 0) | (buffer[9] << 3) | (buffer[10] << 6));
    dest[6] = static_cast<uint8_t>((buffer[10] >> 2) | (buffer[11] << 1) | (buffer[12] << 4) | (buffer[13] << 7));
    dest[7] = static_cast<uint8_t>((buffer[13] >> 1) | (buffer[14] << 2) | (buffer[15] << 5));
    dest += 8, buffer += 16;
    dest[2] = static_cast<uint8_t>((buffer[0] >> 0) | (buffer[1] << 3) | (buffer[2] << 6));
    dest[3] = static_cast<uint8_t>((buffer[2] >> 2) | (buffer[3] << 1) | (buffer[4] << 4) | (buffer[5] << 7));
    dest[4] = static_cast<uint8_t>((buffer[5] >> 1) | (buffer[6] << 2) | (buffer[7] << 5));
    dest[5] = static_cast<uint8_t>((buffer[8] >> 0) | (buffer[9] << 3) | (buffer[10] << 6));
    dest[6] = static_cast<uint8_t>((buffer[10] >> 2) | (buffer[11] << 1) | (buffer[12] << 4) | (buffer[13] << 7));
    dest[7] = static_cast<uint8_t>((buffer[13] >> 1) | (buffer[14] << 2) | (buffer[15] << 5));
    _MM_SET_ROUNDING_MODE(backup);
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
        common::AlignedBuffer<32> buffer(bytePerBlock * (img.Width * img.Height / 4 / 4));
        const auto blockStride = img.ElementSize * 4;
        const auto rowStride = img.Width * img.ElementSize;
        const uint8_t * __restrict row = img.GetRawPtr<uint8_t>();
        uint8_t * __restrict output = buffer.GetRawPtr<uint8_t>();

        for (uint32_t y = img.Height; y > 0; y -= 4)
        {
            for (uint32_t x = img.Width; x > 0; x -= 4)
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
    if (HAS_FIELD(img.DataType, ImageDataType::FLOAT_MASK))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"float data type not supported in BC5");

    BCBlock block;
    switch (img.DataType)
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
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"error datatype for Image");
    }
}


}
