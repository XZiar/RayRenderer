#pragma once

#include "oglRely.h"
#include "oglUtil.h"
#include <GL/wglew.h>
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
	HDC hdc;
	HGLRC hrc;
	std::atomic_flag shouldRun;
	std::mutex mtx;
	std::condition_variable cv;
	std::function<void(void)> task = nullptr;
	std::promise<void> pms;
	SimpleTimer callerTimer;
	void worker()
	{
		SimpleTimer timer;
		std::unique_lock<std::mutex> lck(mtx);
		printf("Worker Thread use HDC[%p] HRC[%p]\n", hdc, hrc);
		wglMakeCurrent(hdc, hrc);
		printf("Worker GL Version:%s\n", oglUtil::getVersion().c_str());
		oglUtil::setDebug(0x2f, 0x2f, MsgLevel::Notfication);
		auto err = oglUtil::getError();
		while (shouldRun.test_and_set())
		{
			cv.wait(lck);
			if (task != nullptr)
			{
				timer.Start();
				task();
				glFinish();
				task = nullptr;
				timer.Stop();
				printf("@@worker cost %lld us\n", timer.ElapseUs());
				pms.set_value();
			}
		}
		//exit
		wglDeleteContext(hrc);
	}
public:
	MTWorker()
	{
	}
	~MTWorker()
	{
		shouldRun.clear();
		cv.notify_all();
	}
	void start(HDC hdc_, HGLRC hrc_)
	{
		hdc = hdc_, hrc = hrc_;
		shouldRun.test_and_set();
		std::thread(&MTWorker::worker, this).detach();
	}
	std::future<void> doWork(std::function<void(void)> work)
	{
		callerTimer.Start();
		mtx.lock();
		task = std::move(work);
		pms = std::promise<void>();
		auto fut = pms.get_future();
		cv.notify_all();
		mtx.unlock();
		callerTimer.Stop();
		printf("@@call worker cost %lld us\n", callerTimer.ElapseUs());
		return fut;
	}
};


}