#pragma once

#include "oglRely.h"
#include "oglUtil.h"
#include "oglInternal.h"
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
	const wstring name;
	SimpleTimer callerTimer;
	void worker()
	{
		SimpleTimer timer;
		const wstring prefix = L"Worker " + name;
		std::unique_lock<std::mutex> lck(mtx);
		wglMakeCurrent(hdc, hrc);
		oglLog().info(L"{} use HDC[{}] HRC[{}], GL version {}\n", prefix, (void*)hdc, (void*)hrc, oglUtil::getVersion());
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
				oglLog().debug(L"{} cost {} us\n", prefix, timer.ElapseUs());
				pms.set_value();
			}
		}
		//exit
		wglDeleteContext(hrc);
	}
public:
	MTWorker(wstring name_) : name(name_)
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
		oglLog().debug(L"CALL {} cost {} us\n", name, callerTimer.ElapseUs());
		return fut;
	}
};


}