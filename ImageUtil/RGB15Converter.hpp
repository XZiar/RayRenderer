#pragma once
#include "ImageUtilRely.h"

namespace xziar::img::convert
{


inline void RGB15ToRGBAs(uint32_t * __restrict destPtr, const uint16_t * __restrict srcPtr, uint64_t count)
{
    while (count--)
    {
        const auto bgr15 = *srcPtr++;
        *destPtr++ = ((bgr15 & 0x1f) << 19) | ((bgr15 & 0x3e) << 6) | ((bgr15 & 0x7c) >> 7) | 0xff000000;
    }
}
inline void BGR15ToRGBAs(uint32_t * __restrict destPtr, const uint16_t * __restrict srcPtr, uint64_t count)
{
    while (count--)
    {
        const auto bgr15 = *srcPtr++;
        *destPtr++ = ((bgr15 & 0x1f) << 3) | ((bgr15 & 0x03e0) << 6) | ((bgr15 & 0x7c00) << 9) | 0xff000000;
    }
}
inline void RGB16ToRGBAs(uint32_t * __restrict destPtr, const uint16_t * __restrict srcPtr, uint64_t count)
{
    while (count--)
    {
        const auto bgr16 = *srcPtr++;
        *destPtr++ = ((bgr16 & 0x1f) << 19) | ((bgr16 & 0x03e0) << 6) | ((bgr16 & 0x7c00) >> 7) | (bgr16 & 0x8000 ? 0xff000000 : 0x0);
    }
}
inline void BGR16ToRGBAs(uint32_t * __restrict destPtr, const uint16_t * __restrict srcPtr, uint64_t count)
{
    while (count--)
    {
        const auto bgr16 = *srcPtr++;
        *destPtr++ = ((bgr16 & 0x1f) << 3) | ((bgr16 & 0x03e0) << 6) | ((bgr16 & 0x7c00) << 9) | (bgr16 & 0x8000 ? 0xff000000 : 0x0);
    }
}

inline void RGB15ToRGBs(byte * __restrict destPtr, const uint16_t * __restrict srcPtr, uint64_t count)
{
    while (count--)
    {
        const auto bgr15 = *srcPtr++;
        *destPtr++ = byte((bgr15 & 0x7c00) >> 7);
        *destPtr++ = byte((bgr15 & 0x03e0) >> 2);
        *destPtr++ = byte((bgr15 & 0x1f) << 3);
    }
}
inline void BGR15ToRGBs(byte * __restrict destPtr, const uint16_t * __restrict srcPtr, uint64_t count)
{
    while (count--)
    {
        const auto bgr15 = *srcPtr++;
        *destPtr++ = byte((bgr15 & 0x1f) << 3);
        *destPtr++ = byte((bgr15 & 0x03e0) >> 2);
        *destPtr++ = byte((bgr15 & 0x7c00) >> 7);
    }
}


}
