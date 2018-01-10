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
	std::atomic_bool isFence;
	std::function<void(void)> task = nullptr;
	std::promise<void> pms;
	std::promise<GLsync> syncpms;
	const wstring name;
	SimpleTimer callerTimer;
	void worker()
	{
		SimpleTimer timer;
		const wstring prefix = L"Worker " + name;
		std::unique_lock<std::mutex> lck(mtx);
        if (!wglMakeCurrent(hdc, hrc))
        {
            oglLog().error(L"{} with HDC[{}] HRC[{}], error: {}\n", prefix, (void*)hdc, (void*)hrc, GetLastError());
        }
		oglLog().info(L"{} use HDC[{}] HRC[{}], GL version {}\n", prefix, (void*)hdc, (void*)hrc, oglUtil::getVersion());
		oglUtil::setDebug(0x2f, 0x2f, MsgLevel::Notfication);
		while (shouldRun.test_and_set())
		{
			cv.wait(lck);
			if (task != nullptr)
			{
				timer.Start();
				task();
				task = nullptr;
				if (isFence)
				{
					const auto sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, NULL);
					syncpms.set_value(sync);
				}
				else
				{
					glFinish();
					pms.set_value();
				}
				timer.Stop();
				oglLog().debug(L"{} cost {} us\n", prefix, timer.ElapseUs());
			}
		}
		//exit
		if (!wglMakeCurrent(hdc, nullptr))
		{
			oglLog().error(L"{} terminate with HDC[{}] HRC[{}], error: {}\n", prefix, (void*)hdc, (void*)hrc, GetLastError());
		}
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
		isFence = false;
		task = std::move(work);
		pms = std::promise<void>();
		auto fut = pms.get_future();
		cv.notify_all();
		mtx.unlock();
		callerTimer.Stop();
		oglLog().debug(L"CALL {} cost {} us\n", name, callerTimer.ElapseUs());
		return fut;
	}
	GLsync doWork2(std::function<void(void)> work)
	{
		callerTimer.Start();
		mtx.lock();
		isFence = true;
		task = std::move(work);
		syncpms = std::promise<GLsync>();
		auto fut = syncpms.get_future();
		cv.notify_all();
		mtx.unlock();
		callerTimer.Stop();
		oglLog().debug(L"CALL {} cost {} us\n", name, callerTimer.ElapseUs());
		return fut.get();
	}
};


}