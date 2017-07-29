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
	using inner_type = T;
	using weak_type = std::weak_ptr<T>;

	constexpr Wrapper() noexcept { }
	

	template<class = typename std::enable_if<std::is_default_constructible<T>::value>::type>
	Wrapper(NoArg) : base_type(std::make_shared<T>())
	{ }
	template<class U, class = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
	Wrapper(const Wrapper<U>& other) noexcept : base_type(std::static_pointer_cast<T>(other))
	{ }
	template<class U, class = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
	Wrapper(Wrapper<U>&& other) noexcept : base_type(static_cast<Wrapper<T>&&>(other))
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
	template<typename Arg, typename... Args, typename = typename std::enable_if<
		sizeof...(Args) != 0 || !std::is_base_of<Wrapper<T>, std::remove_reference<Arg>::type>::value>::type>
	explicit Wrapper(Arg&& arg, Args&&... args) : base_type(std::make_shared<T>(std::forward<Arg>(arg), std::forward<Args>(args)...))
	{ }
	template<class... Args>
	void reset(Args&&... args)
	{
		*this = Wrapper<T>(std::forward<Args>(args)...);
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

	Wrapper(const Wrapper<T>& other) noexcept = default;
	Wrapper(Wrapper<T>&& other) noexcept = default;
	Wrapper& operator=(const Wrapper<T>& other) noexcept = default;
	Wrapper& operator=(Wrapper<T>&& other) noexcept = default;
};

template<class U, class T = U::element_type>
struct WeakPtrComparerator
{
	bool operator()(const std::weak_ptr<T>& pl, const std::weak_ptr<T>& pr) const
	{
		return pl.lock() < pr.lock();
	}
};


}