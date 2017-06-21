#pragma once

#include "Exceptions.hpp"
#include <functional>
#include <memory>

namespace common
{

template<class RetType, class MidArgs>
class AsyncTask
{
public:
	using MainTask = std::function<MidArgs(void)>;
	using PostExecute = std::function<RetType(MidArgs)>;
	using InvokeCallBack = std::function<RetType(void)>;
	using OnFinish = std::function<void(InvokeCallBack)>;
	using OnException = std::function<void(BaseException&)>;
private:
	std::unique_ptr<std::function<MidArgs(void)>> mainTask;
	std::unique_ptr<std::function<T(MidArgs&&...)>> postTask;
public:
	AsyncTask(MainTask& taskBG, PostExecute& taskFG) :
		mainTask(std::make_unique(taskBG)), postTask(std::make_unique(taskFG))
	{
	}
	void RunAsync(const OnFinish& onFinish, const OnException& onException)
	{
		std::thread([taskBG{ std::move(mainTask) }, taskFG{ std::move(postTask) }, onFinish]()
		{
			try
			{
				auto midArg = (*taskBG)();
				auto invokeCallback = std::bind([invoker{ std::move(taskFG) }](auto& midArg)
				{
					return invoker(midArg);
				}, midArg);
				onFinish(invokeCallback);
			}
			catch (BaseException& e)
			{
				onException(e);
			}
		}).detach();
	}
	template<class... ARGS>
	void RunSync(ARGS&&... args)
	{

	}
};

}