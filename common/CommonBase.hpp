#pragma once

#include <cstdint>
#include <type_traits>
#include <memory>
#include <string>
#include <codecvt>

#ifdef COMMON_EXPORT
#   define COMMONAPI _declspec(dllexport) 
#   define COMMONTPL _declspec(dllexport) 
#else
#   define COMMONAPI _declspec(dllimport) 
#   define COMMONTPL
#endif

namespace common
{

struct COMMONAPI NonCopyable
{
	NonCopyable() = default;
	NonCopyable(const NonCopyable & other) = delete;
	NonCopyable(NonCopyable && other) = default;
	NonCopyable& operator= (const NonCopyable & other) = delete;
	NonCopyable& operator= (NonCopyable && other) = default;
};

struct COMMONAPI NonMovable
{
	NonMovable() = default;
	NonMovable(const NonMovable & other) = default;
	NonMovable(NonMovable && other) = delete;
	NonMovable& operator= (const NonMovable & other) = default;
	NonMovable& operator= (NonMovable && other) = delete;
};




template<class T>
class COMMONTPL Wrapper : public std::shared_ptr<T>
{
public:
	using weak_type = std::weak_ptr<T>;
	template<class... ARGS>
	Wrapper(ARGS... args) : std::shared_ptr<T>(new T(args...))
	{ }
	Wrapper(const std::shared_ptr<T>& other) noexcept : std::shared_ptr<T>(other)
	{ }
	Wrapper(std::shared_ptr<T>&& other) noexcept : std::shared_ptr<T>(other)
	{ }
	template<class = typename std::enable_if<std::is_base_of<std::enable_shared_from_this<T>, T>::value>::type>
	Wrapper(T *src) noexcept : std::shared_ptr<T>(src->shared_from_this())
	{ }
	explicit Wrapper(T *src) noexcept : std::shared_ptr<T>(src)
	{ }
	constexpr Wrapper() noexcept { }
	template<class... ARGS>
	void reset(ARGS... args)
	{
		std::shared_ptr<T>::reset(new T(args...));
	}
	void reset(const std::shared_ptr<T>& other) noexcept
	{
		std::shared_ptr<T>::reset(other);
	}
	void reset(std::shared_ptr<T>&& other) noexcept
	{
		std::shared_ptr<T>::reset(other);
	}
	void reset()
	{
		std::shared_ptr<T>::reset(new T());
	}
	void release()
	{
		std::shared_ptr<T>::reset((T*)nullptr);
	}
	weak_type weakRef() const noexcept
	{
		return weak_type(*this);
	}
	template<class U, class = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
	operator const Wrapper<U>&() const noexcept
	{
		return *(Wrapper<U>*)this;
	}
	template<class U, class = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
	operator Wrapper<U>&() noexcept
	{
		return *(Wrapper<U>*)this;
	}
};


template<class U, class T = U::element_type>
struct WeakPtrComparerator
{
	bool operator()(const std::weak_ptr<T>& pl, const std::weak_ptr<T>& pr) const
	{
		return pl.lock() < pr.lock();
	}
};


template<class T = char>
class COMMONTPL OPResult
{
private:
	bool isSuc;
public:
	std::wstring msg;
	T data;
	OPResult(const bool isSuc_, const std::wstring& msg_) :isSuc(isSuc_), msg(msg_) { }
	OPResult(const bool isSuc_, const std::wstring& msg_, const T& dat_) :isSuc(isSuc_), msg(msg_), data(dat_) { }
	OPResult(const bool isSuc_, const std::string& msg_ = "")
		:OPResult(isSuc_, std::wstring(msg_.begin(), msg_.end())) { }
	OPResult(const bool isSuc_, const std::string& msg_, const T& dat_) 
		:OPResult(isSuc_, std::wstring(msg_.begin(), msg_.end()), dat_) { }
	operator bool() { return isSuc; }
};


/** @brief calculate simple hash for string, used for switch-string
** @param str std-string for the text
** @return uint64_t the hash
**/
inline uint64_t hash_(const std::string& str)
{
	uint64_t hash = 0;
	for (size_t a = 0; a < str.length(); ++a)
		hash = hash * 33 + str[a];
	return hash;
}
/** @brief calculate simple hash for string, used for switch-string
** @param str std-string for the text
** @return uint64_t the hash
**/
constexpr uint64_t hash_(const char *str, const uint64_t last = 0)
{
	return *str == '\0' ? last : hash_(str + 1, *str + last * 33);
}
/** @brief calculate simple hash for string, used for switch-string
** @return uint64_t the hash
**/
constexpr uint64_t operator "" _hash(const char *str, size_t)
{
	return hash_(str);
}

}