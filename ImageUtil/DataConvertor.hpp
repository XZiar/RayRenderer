#pragma once

#include "ImageUtilRely.h"

namespace xziar::img::convert
{

inline uint16_t ParseWordLE(const uint8_t *data) { return static_cast<uint16_t>((*(data + 1) << 8) + *data); }
inline uint16_t ParseWordBE(const uint8_t *data) { return static_cast<uint16_t>(*(data + 1) + ((*data) << 8)); }
inline void WordToLE(uint8_t *output, const uint16_t data) { output[0] = static_cast<uint8_t>(data & 0xff); output[1] = static_cast<uint8_t>(data >> 8); }

#pragma region RGBA->RGB
#define LOOP_RGBA_RGB \
		*destPtr++ = *srcPtr++; \
		*destPtr++ = *srcPtr++; \
		*destPtr++ = *srcPtr++; \
		srcPtr++; count--;
inline void RGBAsToRGBs(uint8_t * __restrict destPtr, uint8_t * __restrict srcPtr, uint64_t count)
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
#undef LOOP_RGBA_RGB
#pragma endregion RGBA->RGB


#pragma region BGRA->RGB
#define LOOP_BGRA_RGB \
		*destPtr++ = srcPtr[2]; \
		*destPtr++ = srcPtr[1]; \
		*destPtr++ = srcPtr[0]; \
		srcPtr += 4; count--;
inline void BGRAsToRGBs(uint8_t * __restrict destPtr, uint8_t * __restrict srcPtr, uint64_t count)
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
#undef LOOP_BGRA_RGB
#pragma endregion BGRA->RGB


#pragma region BGR->RGB
#define LOOP_BGR_RGB \
		{ \
			const auto tmp = destPtr[0]; \
			destPtr[0] = destPtr[2]; \
			destPtr[2] = tmp; \
			destPtr += 3; count--; \
		}
inline void BGRsToRGBs(uint8_t * __restrict destPtr, uint64_t count)
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
#pragma endregion BGR->RGB


#pragma region RGB->RGBA
#define LOOP_RGB_RGBA \
		*destPtr++ = *srcPtr++; \
		*destPtr++ = *srcPtr++; \
		*destPtr++ = *srcPtr++; \
		*destPtr++ = 0xff; count--;
inline void RGBsToRGBAs(uint8_t * __restrict destPtr, uint8_t * __restrict srcPtr, uint64_t count)
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
#undef LOOP_RGB_RGBA
#pragma endregion RGB->RGBA


#pragma region BGR->RGBA
#define LOOP_BGR_RGBA \
		*destPtr++ = srcPtr[2]; \
		*destPtr++ = srcPtr[1]; \
		*destPtr++ = *srcPtr; \
		*destPtr++ = 0xff; \
		srcPtr += 3; count--;
inline void BGRsToRGBAs(uint8_t * __restrict destPtr, uint8_t * __restrict srcPtr, uint64_t count)
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
#undef LOOP_BGR_RGBA
#pragma endregion BGR->RGBA


#pragma region BGRA->RGBA
#define LOOP_BGRA_RGBA \
		{ \
			const auto tmp = destPtr[0]; \
			destPtr[0] = destPtr[2]; \
			destPtr[2] = tmp; \
			destPtr += 4; count--; \
		}
inline void BGRAsToRGBAs(uint8_t * __restrict destPtr, uint64_t count)
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
#pragma endregion BGR->RGBA


#pragma region SWAP
#define SWAP_BLOCK(COUNTER) \
		while (COUNTER--) \
		{ \
			const uint8_t tmp = *ptrA; \
			*ptrA++ = *ptrB; \
			*ptrB++ = tmp; \
		} 
inline bool Swap2Buffer(uint8_t * __restrict ptrA, uint8_t * __restrict ptrB, uint64_t bytes)
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
		uint8_t count = std::min(bytes, offset);
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

}