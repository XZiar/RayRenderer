#pragma once

#include <cstdint>
#include <type_traits>
#include <cmath>
#include <vector>

#ifdef __AVX2__
#   define __AVX__ 1
#endif

#ifdef __AVX__
#   define __SSE4_2__ 1
#endif

#ifdef __SSE4_2__
#   define __SSE2__ 1
#endif


#if defined(__GNUC__)
#   include <x86intrin.h>
#   define _mm256_set_m128(/* __m128 */ hi, /* __m128 */ lo)  _mm256_insertf128_ps(_mm256_castps128_ps256(lo), (hi), 0x1)
#   define _mm256_set_m128d(/* __m128d */ hi, /* __m128d */ lo)  _mm256_insertf128_pd(_mm256_castpd128_pd256(lo), (hi), 0x1)
#   define _mm256_set_m128i(/* __m128i */ hi, /* __m128i */ lo)  _mm256_insertf128_si256(_mm256_castsi128_si256(lo), (hi), 0x1)
#   include <malloc.h>
#   define malloc_align(size, align) memalign((align), (size))
#   define free_align(ptr) free(ptr)
#   define VECCALL 
#else
#   include <intrin.h>
#   define malloc_align(size, align) _aligned_malloc((size), (align))
#   define free_align(ptr) _aligned_free(ptr)
#   if (_M_IX86_FP == 2)
#      define __SSE2__ 1
#   endif
#endif
#if !defined(__GNUC__) && defined(__SSE2__)
#   define VECCALL __vectorcall
#else
#   define VECCALL 
#endif

#ifdef __AVX2__
#   define MINIBLAS_INTRIN AVX2
#elif defined(__AVX__)
#   define MINIBLAS_INTRIN AVX
#elif defined(__SSE4_2__)
#   define MINIBLAS_INTRIN SSE4.2
#elif defined(__SSE2__)
#   define MINIBLAS_INTRIN SSE2
#else
#   define MINIBLAS_INTRIN NONE
#endif

#define TO_STR(a) #a
#define GET_STR(a) TO_STR(a)
#pragma message("Compiling miniBLAS with " GET_STR(MINIBLAS_INTRIN) )

namespace miniBLAS
{
inline constexpr char* miniBLAS_intrin()
{
	return GET_STR(MINIBLAS_INTRIN);
#undef GET_STR
#undef TO_STR
}
/*make struct's heap allocation align to N bytes boundary*/
template<uint32_t N = 32>
struct AlignBase
{
	void* operator new(size_t size)
	{
		return malloc_align(size, N);
	};
	void operator delete(void *p)
	{
		free_align(p);
	}
	void* operator new[](size_t size)
	{
		return malloc_align(size, N);
	};
	void operator delete[](void *p)
	{
		free_align(p);
	}
};

template <class T, class = typename std::enable_if<std::is_base_of<AlignBase<>,T>::value>::type>
struct AlignAllocator
{
	typedef T value_type;
	AlignAllocator() noexcept { }
	template<class U> AlignAllocator(const AlignAllocator<U>&) noexcept { }

	template<class U> bool operator==(const AlignAllocator<U>&) const noexcept
	{
		return true;
	}
	template<class U> bool operator!=(const AlignAllocator<U>&) const noexcept
	{
		return false;
	}
	T* allocate(const size_t n) const
	{
		T *ptr = (T*)T::operator new(n * sizeof(T));
		return ptr;
	}
	void deallocate(T* const p, size_t) const noexcept
	{
		T::operator delete(p);
	}
};

}


namespace std
{

template<class T>
using vector_ = vector<T, miniBLAS::AlignAllocator<T>>;


}