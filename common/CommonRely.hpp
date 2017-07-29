#pragma once

#ifdef COMMON_EXPORT
#   define COMMONAPI _declspec(dllexport) 
#   define COMMONTPL _declspec(dllexport) 
#else
#   define COMMONAPI _declspec(dllimport) 
#   define COMMONTPL
#endif

#include <cstdint>
#include <new>

#if defined(__GNUC__)
#   include <malloc.h>
#   define malloc_align(size, align) memalign((align), (size))
#   define free_align(ptr) free(ptr)
#elif defined(_MSC_VER)
#   define malloc_align(size, align) _aligned_malloc((size), (align))
#   define free_align(ptr) _aligned_free(ptr)
#endif

namespace common
{

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


struct NonCopyable
{
	NonCopyable() noexcept = default;
	~NonCopyable() noexcept = default;
	NonCopyable(const NonCopyable&) noexcept = delete;
	NonCopyable& operator= (const NonCopyable&) noexcept = delete;
	NonCopyable(NonCopyable&&) noexcept = default;
	NonCopyable& operator= (NonCopyable&&) noexcept = default;
};

struct NonMovable
{
	NonMovable() noexcept = default;
	~NonMovable() noexcept = default;
	NonMovable(NonMovable&&) noexcept = delete;
	NonMovable& operator= (NonMovable&&) noexcept = delete;
	NonMovable(const NonMovable&) noexcept = default;
	NonMovable& operator= (const NonMovable&) noexcept = default;
};

template<class T>
struct AlignAllocator;

template<class T>
struct AlignBase
{
	friend AlignAllocator<T>;
private:
	static constexpr size_t __cdecl calcAlign() noexcept
	{
		return max((size_t)alignof(T), (size_t)32);
	}
	static constexpr bool __cdecl judgeStrict() noexcept
	{
		return alignof(T) > alignof(max_align_t);
	}
public:
	static void* __cdecl operator new(size_t size)
	{
		if (judgeStrict())
			return malloc_align(size, calcAlign());
		else
			return malloc(size);
	};
	static void __cdecl operator delete(void *p)
	{
		if (judgeStrict())
			return free_align(p);
		else
			return free(p);
	}
	static void* __cdecl operator new[](size_t size)
	{
		if (judgeStrict())
			return malloc_align(size, calcAlign());
		else
			return malloc(size);
	};
	static void __cdecl operator delete[](void *p)
	{
		if (judgeStrict())
			return free_align(p);
		else
			return free(p);
	}
};


}
