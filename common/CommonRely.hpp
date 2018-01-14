#pragma once

#ifdef COMMON_EXPORT
#   define COMMONAPI _declspec(dllexport) 
#   define COMMONTPL _declspec(dllexport) 
#else
#   define COMMONAPI _declspec(dllimport) 
#   define COMMONTPL
#endif

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

#if defined(__GNUC__)
#   include <malloc.h>
#   define malloc_align(size, align) memalign((align), (size))
#   define free_align(ptr) free(ptr)
#elif defined(_MSC_VER)
#   define malloc_align(size, align) _aligned_malloc((size), (align))
#   define free_align(ptr) _aligned_free(ptr)
#endif


/**
** @brief calculate simple hash for string, used for switch-string
** @param str std-string_view/string for the text
** @return uint64_t the hash
**/
template<class T, class = typename std::enable_if<std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>>::type>
inline uint64_t hash_(const T& str)
{
	uint64_t hash = 0;
	for (size_t a = 0, len = str.length(); a < len; ++a)
		hash = hash * 33 + str[a];
	return hash;
}
/**
** @brief calculate simple hash for string, used for switch-string
** @param str c-string for the text
** @return uint64_t the hash
**/
constexpr inline uint64_t hash_(const char *str)
{
	uint64_t hash = 0;
	for (; *str != '\0'; ++str)
		hash = hash * 33 + *str;
	return hash;
}
/**
** @brief calculate simple hash for string, used for switch-string
** @return uint64_t the hash
**/
constexpr inline uint64_t operator "" _hash(const char *str, size_t)
{
	return hash_(str);
}


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


class FixedArray
{
private:
	uint8_t *Data = nullptr;
public:
	static constexpr size_t MIN_ALIGNMENT = alignof(max_align_t);
	const size_t Size;
	const size_t Alignment;
	FixedArray(const size_t size, const size_t alignment = 32) : Size(size), Alignment(max(alignment, MIN_ALIGNMENT)), Data((uint8_t*)malloc_align(Size, Alignment)) {}
	~FixedArray() { if (Data) free_align(Data); }
	FixedArray(const FixedArray& other) : Size(other.Size), Alignment(other.Alignment), Data((uint8_t*)malloc_align(Size, Alignment)) 
	{
		memmove(Data, other.Data, Size);
	}
	FixedArray(FixedArray&& other) : Size(other.Size), Alignment(other.Alignment), Data(other.Data) 
	{
		other.Data = nullptr;
	}
};


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
