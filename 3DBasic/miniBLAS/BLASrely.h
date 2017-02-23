#pragma once

#include <cstdint>
#include <type_traits>
#include <cmath>
#include <vector>

#ifdef COMMON_EXPORT
#   define COMMONAPI _declspec(dllexport) 
#   define COMMONTPL _declspec(dllexport) 
#else
#   define COMMONAPI _declspec(dllimport) 
#   define COMMONTPL
#endif

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
#if !defined(__GNUC__) && defined(__SSE2__) && !defined(_MANAGED) && !defined(_M_CEE)
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


template<class T, class = typename std::enable_if<std::is_arithmetic<T>::value>::type>
constexpr const T& max(const T& left, const T& right)
{
	return left < right ? right : left;
}
template<class T, class = typename std::enable_if<std::is_arithmetic<T>::value>::type>
constexpr const T& min(const T& left, const T& right)
{
	return left < right ? left : right;
}


/*make struct's heap allocation align to N bytes boundary*/
template<uint32_t N = 32>
struct COMMONTPL AlignBase
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

template <class T>
struct AlignAllocator
{
	using value_type = T;
	AlignAllocator() noexcept = default;
	template<class U> 
	AlignAllocator(const AlignAllocator<U>&) noexcept { }

	template<class U> 
	bool operator==(const AlignAllocator<U>&) const noexcept
	{
		return std::is_base_of<AlignBase<>, U>::value == std::is_base_of<AlignBase<>, T>::value;
	}
	template<class U> 
	bool operator!=(const AlignAllocator<U>&) const noexcept
	{
		return std::is_base_of<AlignBase<>, U>::value != std::is_base_of<AlignBase<>, T>::value;
	}
	T* allocate(const std::size_t n) const
	{
		T *ptr = nullptr;
		if (n == 0)
			return ptr;
		if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
			throw std::bad_array_new_length();
		ptr = allocate(n, std::is_base_of<AlignBase<>, T>());
		if (!ptr)
			throw std::bad_alloc();
		else
			return ptr;
	}
	T* allocate(const std::size_t n, std::true_type) const
	{
		return (T*)T::operator new(n * sizeof(T));
	}
	T* allocate(const std::size_t n, std::false_type) const
	{
		return (T*)malloc_align(n * sizeof(T), 32);
	}
	void deallocate(T* p, const std::size_t) const noexcept
	{
		deallocate(p, std::is_base_of<AlignBase<>, T>());
	}
	void deallocate(T* p, std::true_type) const noexcept
	{
		T::operator delete(p);
	}
	void deallocate(T* p, std::false_type) const noexcept
	{
		free_align(p);
	}
};

template<class T, bool isAlign = std::is_base_of<miniBLAS::AlignBase<>, T>::value>
class vector;

template<class T>
class vector<T, true> : public std::vector<T, miniBLAS::AlignAllocator<T>>
{
public:
	using std::vector<T, miniBLAS::AlignAllocator<T>>::vector;
};

template<class T>
class vector<T, false> : public std::vector<T, std::allocator<T>>
{
public:
	using std::vector<T, std::allocator<T>>::vector;
};

}


namespace std
{

template<class T>
using vector_ = vector<T, miniBLAS::AlignAllocator<T>>;

}