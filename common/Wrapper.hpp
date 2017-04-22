#pragma once

#include "CommonRely.hpp"
#include <cstdint>
#include <type_traits>
#include <memory>
#include <string>


namespace common
{
struct NoArg {};

template<class T>
class Wrapper : public std::shared_ptr<T>
{
private:
	using base_type = std::shared_ptr<T>;
public:
	using weak_type = std::weak_ptr<T>;
	constexpr Wrapper() noexcept { }
	template<class = typename std::enable_if<std::is_default_constructible<T>::value>::type>
	Wrapper(NoArg) : base_type(std::make_shared<T>())
	{ }
	template<class U, class = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
	Wrapper(const Wrapper<U>& other) noexcept : base_type(std::static_pointer_cast<T>(other))
	{ }
	Wrapper(const base_type& other) noexcept : base_type(other)
	{ }
	Wrapper(base_type&& other) noexcept : base_type(other)
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
	template<class = typename std::enable_if<std::is_default_constructible<T>::value>::type>
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


}