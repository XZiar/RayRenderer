#pragma once

#include <memory>
#include <functional>

namespace common
{


namespace detail
{


template<class T>
class COMMONTPL PromiseResult_ : public NonCopyable, public std::enable_shared_from_this<PromiseResult_<T>>
{
protected:
	PromiseResult_()
	{ }
public:
	T virtual wait() = 0;
};


}

template<class T>
using PromiseResult = std::shared_ptr<detail::PromiseResult_<T>>;

template<class T>
class COMMONTPL PromiseTask : public NonCopyable
{
protected:
	std::function<T(void)> task;
public:
	PromiseTask(std::function<T(void)> task_) : task(task_)
	{ }
	template<class... ARGS>
	PromiseTask(std::function<T(ARGS...)> task_, ARGS... args) : task(std::bind(task_, args))
	{ }
	void virtual dowork() = 0;
	PromiseResult<T> virtual getResult() = 0;
};


}