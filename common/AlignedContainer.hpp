#pragma once

#include "CommonRely.hpp"

#include <vector>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace common
{

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
		return true;
	}
	template<class U>
	bool operator!=(const AlignAllocator<U>&) const noexcept
	{
		return false;
	}
	T* allocate(const size_t n) const
	{
		T *ptr = nullptr;
		if (n == 0)
			return ptr;
		if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
			throw std::bad_array_new_length();
		if (AlignBase<T>::judgeStrict())
			ptr = (T*)malloc_align(n * sizeof(T), AlignBase<T>::calcAlign());
		else
			ptr = (T*)malloc(n * sizeof(T));
		if (!ptr)
			throw std::bad_alloc();
		else
			return ptr;
	}
	void deallocate(T* const p, const size_t) const noexcept
	{
		if (AlignBase<T>::judgeStrict())
			free_align(p);
		else
			free(p);
	}
};


template<class T>
class vectorEx : public std::vector<T, common::AlignAllocator<T>>
{
public:
	using std::vector<T, common::AlignAllocator<T>>::vector;
	operator const std::vector<T>&() const
	{
		return *(const std::vector<T>*)this;
	}
};

template<class T>
using dequeEx = std::deque<T, common::AlignAllocator<T>>;

template<class K, class V, class C = std::less<K>>
using mapEx = std::map<K, V, C, common::AlignAllocator<std::pair<const K, V>>>;

template<class T, class C = std::less<T>>
using setEx = std::set<T, C, common::AlignAllocator<T>>;

template<class K, class V, class H = std::hash<K>, class E = std::equal_to<K>>
using hashmapEx = std::unordered_map<K, H, E, V, common::AlignAllocator<std::pair<const K, V>>>;

template<class T, class H = std::hash<T>, class E = std::equal_to<K>>
using hashsetEx = std::unordered_set<T, H, E, common::AlignAllocator<T>>;


namespace ImageType
{
struct RGBA { using ComponentType = uint32_t; };
struct GREY { using ComponentType = uint8_t; };
}

template<class T>
struct Image
{
public:
	uint32_t width, height;
	common::vectorEx<typename T::ComponentType> data;
	size_t size() const { return data.size(); }
};

}
