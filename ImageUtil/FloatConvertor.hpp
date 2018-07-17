#pragma once

#include "ImageUtilRely.h"

namespace xziar::img::convert
{



inline void Float1sToU8s(byte * __restrict destPtr, const float * __restrict srcPtr, uint64_t count, const float floatRange)
{
    const auto muler = 255 / floatRange;
#if COMMON_SIMD_LV >= 200
    const auto SHUF_MSK1 = _mm256_setr_epi8(0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto SHUF_MSK2 = _mm256_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12);
    const auto muler8 = _mm256_set1_ps(muler);
    while (count > 64)
    {
        const auto src0 = _mm256_mul_ps(_mm256_loadu_ps(srcPtr), muler8);
        const auto src1 = _mm256_mul_ps(_mm256_loadu_ps(srcPtr + 8), muler8);
        const auto src2 = _mm256_mul_ps(_mm256_loadu_ps(srcPtr + 16), muler8);
        const auto src3 = _mm256_mul_ps(_mm256_loadu_ps(srcPtr + 24), muler8);
        const auto src4 = _mm256_mul_ps(_mm256_loadu_ps(srcPtr + 32), muler8);
        const auto src5 = _mm256_mul_ps(_mm256_loadu_ps(srcPtr + 40), muler8);
        const auto src6 = _mm256_mul_ps(_mm256_loadu_ps(srcPtr + 48), muler8);
        const auto src7 = _mm256_mul_ps(_mm256_loadu_ps(srcPtr + 56), muler8);
        const auto i8pix0 = _mm256_shuffle_epi8(_mm256_cvtps_epi32(src0), SHUF_MSK1);//a000,0b00
        const auto i8pix1 = _mm256_shuffle_epi8(_mm256_cvtps_epi32(src1), SHUF_MSK2);//00c0,000d
        const auto i8pix2 = _mm256_shuffle_epi8(_mm256_cvtps_epi32(src2), SHUF_MSK1);//e000,0f00
        const auto i8pix3 = _mm256_shuffle_epi8(_mm256_cvtps_epi32(src3), SHUF_MSK2);//00g0,000h
        const auto i8pix4 = _mm256_shuffle_epi8(_mm256_cvtps_epi32(src4), SHUF_MSK1);//a000,0b00
        const auto i8pix5 = _mm256_shuffle_epi8(_mm256_cvtps_epi32(src5), SHUF_MSK2);//00c0,000d
        const auto i8pix6 = _mm256_shuffle_epi8(_mm256_cvtps_epi32(src6), SHUF_MSK1);//e000,0f00
        const auto i8pix7 = _mm256_shuffle_epi8(_mm256_cvtps_epi32(src7), SHUF_MSK2);//00g0,000h
        const auto acbd0 = _mm256_blend_epi32(i8pix0, i8pix1, 0b11001100);//a0c0,0b0d
        const auto egfh0 = _mm256_blend_epi32(i8pix2, i8pix3, 0b11001100);//e0g0,0f0h
        const auto acbd1 = _mm256_blend_epi32(i8pix4, i8pix5, 0b11001100);//a0c0,0b0d
        const auto egfh1 = _mm256_blend_epi32(i8pix6, i8pix7, 0b11001100);//e0g0,0f0h
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
    const auto muler4 = _mm_set1_ps(muler);
    while (count > 32)
    {
        const auto src0 = _mm_mul_ps(_mm_loadu_ps(srcPtr), muler4);
        const auto src1 = _mm_mul_ps(_mm_loadu_ps(srcPtr + 4), muler4);
        const auto src2 = _mm_mul_ps(_mm_loadu_ps(srcPtr + 8), muler4);
        const auto src3 = _mm_mul_ps(_mm_loadu_ps(srcPtr + 12), muler4);
        const auto src4 = _mm_mul_ps(_mm_loadu_ps(srcPtr + 16), muler4);
        const auto src5 = _mm_mul_ps(_mm_loadu_ps(srcPtr + 20), muler4);
        const auto src6 = _mm_mul_ps(_mm_loadu_ps(srcPtr + 24), muler4);
        const auto src7 = _mm_mul_ps(_mm_loadu_ps(srcPtr + 28), muler4);
        const auto i8pix0 = _mm_shuffle_epi8(_mm_cvtps_epi32(src0), SHUF_MSK);//a000
        const auto i8pix1 = _mm_shuffle_epi8(_mm_cvtps_epi32(src1), SHUF_MSK);//b000
        const auto i8pix2 = _mm_shuffle_epi8(_mm_cvtps_epi32(src2), SHUF_MSK);//c000
        const auto i8pix3 = _mm_shuffle_epi8(_mm_cvtps_epi32(src3), SHUF_MSK);//d000
        const auto i8pix4 = _mm_shuffle_epi8(_mm_cvtps_epi32(src4), SHUF_MSK);//e000
        const auto i8pix5 = _mm_shuffle_epi8(_mm_cvtps_epi32(src5), SHUF_MSK);//f000
        const auto i8pix6 = _mm_shuffle_epi8(_mm_cvtps_epi32(src6), SHUF_MSK);//g000
        const auto i8pix7 = _mm_shuffle_epi8(_mm_cvtps_epi32(src7), SHUF_MSK);//h000
        const auto ac00 = _mm_unpacklo_epi32(i8pix0, i8pix2);//ac00
        const auto bd00 = _mm_unpacklo_epi32(i8pix1, i8pix3);//bd00
        const auto eg00 = _mm_unpacklo_epi32(i8pix4, i8pix6);//eg00
        const auto fh00 = _mm_unpacklo_epi32(i8pix5, i8pix7);//fh00
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr), _mm_unpacklo_epi32(ac00, bd00));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(destPtr + 16), _mm_unpacklo_epi32(eg00, fh00));
        srcPtr += 32; destPtr += 32; count -= 32;
    }
#endif
    while (count > 0)
    {
        switch (count)
        {
        default: *destPtr++ = byte(uint8_t(*srcPtr++ * muler)); count--;
        case 7:  *destPtr++ = byte(uint8_t(*srcPtr++ * muler)); count--;
        case 6:  *destPtr++ = byte(uint8_t(*srcPtr++ * muler)); count--;
        case 5:  *destPtr++ = byte(uint8_t(*srcPtr++ * muler)); count--;
        case 4:  *destPtr++ = byte(uint8_t(*srcPtr++ * muler)); count--;
        case 3:  *destPtr++ = byte(uint8_t(*srcPtr++ * muler)); count--;
        case 2:  *destPtr++ = byte(uint8_t(*srcPtr++ * muler)); count--;
        case 1:  *destPtr++ = byte(uint8_t(*srcPtr++ * muler)); count--;
        }
    }
}

inline void U8sToFloat1s(float * __restrict destPtr, const byte * __restrict srcPtr, uint64_t count, const float floatRange)
{
    const auto muler = floatRange / 255;
#if COMMON_SIMD_LV >= 200
    const auto SHUF_MSK1 = _mm256_setr_epi8( 0, -1, -1, -1,  1, -1, -1, -1,  2, -1, -1, -1,  3, -1, -1, -1,  0, -1, -1, -1,  1, -1, -1, -1,  2, -1, -1, -1,  3, -1, -1, -1);
    const auto SHUF_MSK2 = _mm256_setr_epi8( 4, -1, -1, -1,  5, -1, -1, -1,  6, -1, -1, -1,  7, -1, -1, -1,  4, -1, -1, -1,  5, -1, -1, -1,  6, -1, -1, -1,  7, -1, -1, -1);
    const auto SHUF_MSK3 = _mm256_setr_epi8( 8, -1, -1, -1,  9, -1, -1, -1, 10, -1, -1, -1, 11, -1, -1, -1,  8, -1, -1, -1,  9, -1, -1, -1, 10, -1, -1, -1, 11, -1, -1, -1);
    const auto SHUF_MSK4 = _mm256_setr_epi8(12, -1, -1, -1, 13, -1, -1, -1, 14, -1, -1, -1, 15, -1, -1, -1, 12, -1, -1, -1, 13, -1, -1, -1, 14, -1, -1, -1, 15, -1, -1, -1);
    const auto muler8 = _mm256_set1_ps(muler);
    while (count > 64)
    {
        const auto src0 = _mm256_loadu_si256((const __m256i*)srcPtr);//00~0f,10~1f
        const auto src1 = _mm256_loadu_si256((const __m256i*)(srcPtr + 32));//20~2f,30~3f
        const auto abcd0 = _mm256_shuffle_epi8(src0, SHUF_MSK1);//00-03,10-13
        const auto abcd1 = _mm256_shuffle_epi8(src0, SHUF_MSK2);//04-07,14-17
        const auto abcd2 = _mm256_shuffle_epi8(src0, SHUF_MSK3);//08-0b,18-1b
        const auto abcd3 = _mm256_shuffle_epi8(src0, SHUF_MSK4);//0c-0f,1c-1f
        const auto abcd4 = _mm256_shuffle_epi8(src1, SHUF_MSK1);//20-23,30-33
        const auto abcd5 = _mm256_shuffle_epi8(src1, SHUF_MSK2);//24-27,34-37
        const auto abcd6 = _mm256_shuffle_epi8(src1, SHUF_MSK3);//28-2b,38-3b
        const auto abcd7 = _mm256_shuffle_epi8(src1, SHUF_MSK4);//2c-2f,3c-3f
        const auto f4pix04 = _mm256_mul_ps(_mm256_cvtepi32_ps(abcd0), muler8);
        const auto f4pix15 = _mm256_mul_ps(_mm256_cvtepi32_ps(abcd1), muler8);
        const auto f4pix26 = _mm256_mul_ps(_mm256_cvtepi32_ps(abcd2), muler8);
        const auto f4pix37 = _mm256_mul_ps(_mm256_cvtepi32_ps(abcd3), muler8);
        const auto f4pix8c = _mm256_mul_ps(_mm256_cvtepi32_ps(abcd4), muler8);
        const auto f4pix9d = _mm256_mul_ps(_mm256_cvtepi32_ps(abcd5), muler8);
        const auto f4pixae = _mm256_mul_ps(_mm256_cvtepi32_ps(abcd6), muler8);
        const auto f4pixbf = _mm256_mul_ps(_mm256_cvtepi32_ps(abcd7), muler8);
        _mm256_storeu_ps(destPtr, _mm256_permute2f128_ps(f4pix04, f4pix15, 0x20));
        _mm256_storeu_ps(destPtr + 8, _mm256_permute2f128_ps(f4pix26, f4pix37, 0x20));
        _mm256_storeu_ps(destPtr + 16, _mm256_permute2f128_ps(f4pix04, f4pix15, 0x31)); 
        _mm256_storeu_ps(destPtr + 24, _mm256_permute2f128_ps(f4pix26, f4pix37, 0x31));
        _mm256_storeu_ps(destPtr + 32, _mm256_permute2f128_ps(f4pix8c, f4pix9d, 0x20));
        _mm256_storeu_ps(destPtr + 40, _mm256_permute2f128_ps(f4pixae, f4pixbf, 0x20));
        _mm256_storeu_ps(destPtr + 48, _mm256_permute2f128_ps(f4pix8c, f4pix9d, 0x31));
        _mm256_storeu_ps(destPtr + 56, _mm256_permute2f128_ps(f4pixae, f4pixbf, 0x31));
        srcPtr += 64; destPtr += 64; count -= 64;
    }
#elif COMMON_SIMD_LV >= 31
    const auto muler4 = _mm_set1_ps(muler);
#endif
    while (count > 0)
    {
        switch (count)
        {
        default: *destPtr++ = (uint32_t)*srcPtr++ * muler; count--;
        case 7:  *destPtr++ = (uint32_t)*srcPtr++ * muler; count--;
        case 6:  *destPtr++ = (uint32_t)*srcPtr++ * muler; count--;
        case 5:  *destPtr++ = (uint32_t)*srcPtr++ * muler; count--;
        case 4:  *destPtr++ = (uint32_t)*srcPtr++ * muler; count--;
        case 3:  *destPtr++ = (uint32_t)*srcPtr++ * muler; count--;
        case 2:  *destPtr++ = (uint32_t)*srcPtr++ * muler; count--;
        case 1:  *destPtr++ = (uint32_t)*srcPtr++ * muler; count--;
        }
}
}



}
