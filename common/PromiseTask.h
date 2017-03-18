#pragma once

#include <cstdint>
#include <memory>
#include <functional>

namespace common
{


template<class T>
class COMMONTPL PromiseResult : public NonCopyable
{
protected:
	PromiseResult()
	{ }
public:
	T virtual wait() = 0;
};


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
	std::unique_ptr<PromiseResult<T>> virtual getResult() = 0;
};


//template<>
//class COMMONTPL PromiseTask<void> : public NonCopyable
//{
//protected:
//	std::function<void(void)> task;
//public:
//	PromiseTask(std::function<void(void)> task_) : task(task_)
//	{ }
//	template<class... ARGS>
//	PromiseTask(std::function<void(ARGS...)> task_, ARGS... args) : task(std::bind(task_, args))
//	{ }
//	void virtual dowork() = 0;
//	std::unique_ptr<PromiseResult<void>> virtual getResult() = 0;
//};


}