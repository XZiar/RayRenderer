#pragma once

#include "oglRely.h"
#include "oglUtil.h"
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>


namespace oglu::detail
{


class MTWorker
{
private:
	std::atomic_flag shouldRun;
	std::mutex mtx;
	std::condition_variable cv;
	std::packaged_task<void(void)> task;
	void worker()
	{
		std::unique_lock<std::mutex> lck(mtx);
		while (shouldRun.test_and_set())
		{
			cv.wait(lck);
			if(task.valid())
			{
				task();
				task = std::move(std::packaged_task<void(void)>());
			}
		}
		//exit
	}
public:
	MTWorker()
	{
		shouldRun.test_and_set();
		std::thread(&MTWorker::worker, this).detach();
	}
	~MTWorker()
	{
		shouldRun.clear();
		cv.notify_all();
	}
	std::future<void> doWork(std::function<void(void)> task_)
	{
		mtx.lock();
		task = std::move(std::packaged_task<void(void)>(task_));
		auto fut = task.get_future();
		cv.notify_all();
		mtx.unlock();
		return fut;
	}
};


}