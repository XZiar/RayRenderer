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
#if defined(__SSE4_1__) || defined(__SSE4_2__) || defined(__AVX__) || defined(__AVX2__)
#   pragma warning(disable:4309)
	const auto shuffleMask = _mm_setr_epi8(0x0, 0x1, 0x2, 0xff, 0x3, 0x4, 0x5, 0xff, 0x6, 0x7, 0x8, 0xff, 0x9, 0xa, 0xb, 0xff);
	const auto shuffleMask2 = _mm_setr_epi8(0x4, 0x5, 0x6, 0xff, 0x7, 0x8, 0x9, 0xff, 0xa, 0xb, 0xc, 0xff, 0xd, 0xe, 0xf, 0xff);
#   pragma warning(default:4309)
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
#if defined(__SSE4_1__) || defined(__SSE4_2__) || defined(__AVX__) || defined(__AVX2__)
#   pragma warning(disable:4309)
	const auto shuffleMask = _mm_setr_epi8(0x2, 0x1, 0x0, 0xff, 0x5, 0x4, 0x3, 0xff, 0x8, 0x7, 0x6, 0xff, 0xb, 0xa, 0x9, 0xff);
	const auto shuffleMask2 = _mm_setr_epi8(0x6, 0x5, 0x4, 0xff, 0x9, 0x8, 0x7, 0xff, 0xc, 0xb, 0xa, 0xff, 0xf, 0xe, 0xd, 0xff);
#   pragma warning(default:4309)
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
#if defined(__SSSE3__) || defined(__SSE4_1__) || defined(__SSE4_2__) || defined(__AVX__) || defined(__AVX2__)
#   pragma warning(disable:4309)
	const auto shuffle128 = _mm_setr_epi8(0x2, 0x1, 0x0, 0x3, 0x6, 0x5, 0x4, 0x7, 0xa, 0x9, 0x8, 0xb, 0xe, 0xd, 0xc, 0xf);
#   pragma warning(default:4309)
#   if defined(__AVX__) || defined(__AVX2__)
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
#if defined(__SSSE3__) || defined(__SSE4_1__) || defined(__SSE4_2__) || defined(__AVX__) || defined(__AVX2__)
#   pragma warning(disable:4309)
	const auto shuffle128 = _mm_setr_epi8(0x2, 0x1, 0x0, 0x3, 0x6, 0x5, 0x4, 0x7, 0xa, 0x9, 0x8, 0xb, 0xe, 0xd, 0xc, 0xf);
#   pragma warning(default:4309)
#   if defined(__AVX__) || defined(__AVX2__)
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
#if defined(__AVX__) || defined(__AVX2__)
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
		_mm_prefetch((const char*)ptrA + 32, _MM_HINT_NTA);
		_mm_prefetch((const char*)ptrA + 64, _MM_HINT_NTA);
		_mm_prefetch((const char*)ptrA + 96, _MM_HINT_NTA);
		_mm_prefetch((const char*)ptrB, _MM_HINT_NTA);
		_mm_prefetch((const char*)ptrB + 32, _MM_HINT_NTA);
		_mm_prefetch((const char*)ptrB + 64, _MM_HINT_NTA);
		_mm_prefetch((const char*)ptrB + 96, _MM_HINT_NTA);
	}
	while (bytes >= 32)
	{
		const auto tmp = _mm256_load_si256((const __m256i*)ptrA);
		_mm256_store_si256((__m256i*)ptrA, _mm256_loadu_si256((const __m256i*)ptrB)); ptrA += 32;
		_mm256_storeu_si256((__m256i*)ptrB, tmp); ptrB += 32;
		bytes -= 32;
	}
	SWAP_BLOCK(bytes)
#elif defined(__SSE2__) || defined(__SSE3__) || defined(__SSSE3__) || defined(__SSE4_1__) || defined(__SSE4_2__)
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
		_mm_prefetch((const char*)ptrA + 32, _MM_HINT_NTA);
		_mm_prefetch((const char*)ptrB, _MM_HINT_NTA);
		_mm_prefetch((const char*)ptrB + 32, _MM_HINT_NTA);
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
#if defined(__AVX2__)
    if ((intptr_t)ptrA & 0x1f)
    {
        const uint64_t offset = (32 - ((intptr_t)ptrA & 0x1f)) / 4;
        uint8_t diff = (uint8_t)std::min(count, offset);
        count -= diff;
        REV_BLOCK(diff)
    }
    const auto indexer = _mm256_setr_epi32(7, 6, 5, 4, 3, 2, 1, 0);
    //ptrA now 32-byte aligned(start from a cache line)
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
        _mm_prefetch((const char*)ptrA + 32, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrA + 64, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrA + 96, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC - 32, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC - 64, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC - 96, _MM_HINT_NTA);
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
#elif defined(__AVX__)
    if ((intptr_t)ptrA & 0x1f)
    {
        const uint64_t offset = (32 - ((intptr_t)ptrA & 0x1f)) / 4;
        uint8_t diff = (uint8_t)std::min(count, offset);
        count -= diff;
        REV_BLOCK(diff)
    }
    const auto indexer = _mm256_setr_epi32(3, 2, 1, 0, 3, 2, 1, 0);
    //ptrA now 32-byte aligned(start from a cache line)
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
        _mm_prefetch((const char*)ptrA + 32, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrA + 64, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrA + 96, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC - 32, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC - 64, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC - 96, _MM_HINT_NTA);
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
#elif defined(__SSE2__) || defined(__SSE3__) || defined(__SSSE3__) || defined(__SSE4_1__) || defined(__SSE4_2__)
    if ((intptr_t)ptrA & 0x1f)
    {
        const uint64_t offset = (32 - ((intptr_t)ptrA & 0x1f)) / 4;
        uint8_t diff = (uint8_t)std::min(count, offset);
        count -= diff;
        REV_BLOCK(diff)
    }
    //ptrA now 32-byte aligned(start from a cache line)
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
        _mm_prefetch((const char*)ptrA + 32, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC, _MM_HINT_NTA);
        _mm_prefetch((const char*)ptrC - 32, _MM_HINT_NTA);
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


using BGR16ToRGBAMap = std::vector<uint32_t>;
using RGB16ToRGBAMap = BGR16ToRGBAMap;
static BGR16ToRGBAMap GenerateBGR16ToRGBAMap()
{
    BGR16ToRGBAMap map(1 << 16);
    constexpr uint32_t COUNT = 1 << 5, STEP = 256 / COUNT, HALF_SIZE = 1 << 15;
    constexpr uint32_t RED_STEP = STEP, GREEN_STEP = STEP << 8, BLUE_STEP = STEP << 16;
    uint32_t idx = 0;
    uint32_t color = 0;
    for (uint32_t red = COUNT, colorR = color; red--; colorR += RED_STEP)
    {
        for (uint32_t green = COUNT, colorRG = colorR; green--; colorRG += GREEN_STEP)
        {
            for (uint32_t blue = COUNT, colorRGB = colorRG; blue--; colorRGB += BLUE_STEP)
            {
                map[idx++] = colorRGB;
            }
        }
    }
    for (uint32_t count = 0; count < HALF_SIZE;)//protential 4k-alignment issue
        map[idx++] = map[count++] | 0xff000000;
    return map;
}
static const BGR16ToRGBAMap& GetBGR16ToRGBAMap()
{
    static const auto map = GenerateBGR16ToRGBAMap();
    return map;
}
static RGB16ToRGBAMap GenerateRGB16ToRGBAMap()
{
    RGB16ToRGBAMap map(1 << 16);
    BGRAsToRGBAs(reinterpret_cast<byte*>(map.data()), reinterpret_cast<const byte*>(GetBGR16ToRGBAMap().data()), map.size());
    return map;
}
static const BGR16ToRGBAMap& GetRGB16ToRGBAMap()
{
    static const auto map = GenerateRGB16ToRGBAMap();
    return map;
}


}