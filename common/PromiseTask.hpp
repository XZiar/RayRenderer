#pragma once

#include "PromiseTask.h"
#include <type_traits>
#include <future>


namespace common
{


template<class T>
class COMMONTPL PromiseTaskSTD;


template<class T>
class COMMONTPL PromiseResultSTD : public PromiseResult<T>
{
protected:
	std::future<T> fut;
public:
	PromiseResultSTD(std::future<T>&& fut_) : fut(std::move(fut_))
	{ }
	T virtual wait() override
	{
		return fut.get();
	}
};

template<>
class COMMONTPL PromiseResultSTD<void> : public PromiseResult<void>
{
protected:
	std::future<void> fut;
public:
	PromiseResultSTD(std::future<void>&& fut_) : fut(std::move(fut_))
	{ }
	void virtual wait() override
	{
		fut.get();
		return;
	}
};


template<class T>
class COMMONTPL PromiseTaskSTD : PromiseTask<T>
{
protected:
	std::promise<T> pms;
public:
	PromiseTaskSTD(std::function<T(void)> task_) : PromiseTask(task_)
	{ }
	template<class... ARGS>
	PromiseTaskSTD(std::function<T(ARGS...)> task_, ARGS... args) : PromiseTask(std::bind(task_, args))
	{ }
	void virtual dowork() override
	{
		pms.set_value(task());
	}
	std::unique_ptr<PromiseResult<T>> getResult() override
	{
		PromiseResult<T> *ret = new PromiseResultSTD<T>(pms.get_future());
		return std::unique_ptr<PromiseResult<T>>(ret);
	}
};


template<>
class COMMONTPL PromiseTaskSTD<void> : PromiseTask<void>
{
protected:
	std::promise<void> pms;
public:
	PromiseTaskSTD(std::function<void(void)> task_) : PromiseTask(task_)
	{ }
	template<class... ARGS>
	PromiseTaskSTD(std::function<void(ARGS...)> task_, ARGS... args) : PromiseTask(std::bind(task_, args))
	{ }
	void virtual dowork() override
	{
		task();
		pms.set_value();
	}
	std::unique_ptr<PromiseResult<void>> getResult() override
	{
		PromiseResult<void> *ret = new PromiseResultSTD<void>(pms.get_future());
		return std::unique_ptr<PromiseResult<void>>(ret);
	}
};


}
