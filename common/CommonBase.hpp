#pragma once

#include <cstdint>
#include <type_traits>
#include <memory>
#include <string>

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


struct NoArg {};

template<class T>
class COMMONTPL Wrapper : public std::shared_ptr<T>
{
private:
	using base_type = std::shared_ptr<T>;
public:
	using weak_type = std::weak_ptr<T>;
	constexpr Wrapper() noexcept { }
	Wrapper(NoArg) : base_type(std::make_shared<T>())
	{ }
	template<class U, class = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
	Wrapper(const Wrapper<U>& other) noexcept : base_type(std::static_pointer_cast<T>(other))
	{ }
	Wrapper(const base_type& other) noexcept : base_type(other)
	{ }
	Wrapper(base_type&& other) noexcept : base_type(other)
	{ }
	template<class = typename std::enable_if<std::is_base_of<std::enable_shared_from_this<T>, T>::value>::type>
	Wrapper(T *src) noexcept : base_type(src->shared_from_this())
	{ }
	explicit Wrapper(T *src) noexcept : base_type(src)
	{ }
	template<class... ARGS>
	Wrapper(ARGS... args) : base_type(std::make_shared<T>(args...))
	{ }
	template<class... ARGS>
	void reset(ARGS... args)
	{
		*this = Wrapper<T>(args...);
	}
	void reset()
	{
		*this = Wrapper<T>(NoArg());
	}
	void release()
	{
		base_type::reset();
	}
	weak_type weakRef() const noexcept
	{
		return weak_type(*this);
	}
	template<class U>
	Wrapper<U> cast_static() const noexcept
	{
		return Wrapper<U>(std::static_pointer_cast<U>(*this));
	}
	template<class U, class = typename std::enable_if<std::is_convertible<U*, T*>::value || std::is_convertible<T*, U*>::value>::type>
	Wrapper<U> cast_dynamic() const noexcept
	{
		return Wrapper<U>(std::dynamic_pointer_cast<U>(*this));
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