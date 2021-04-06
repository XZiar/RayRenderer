#include "SystemCommonPch.h"
#include "StackTrace.h"
#include "ThreadEx.h"
#include "StringUtil/Convert.h"
#include "common/SharedString.hpp"
#if defined(_WIN32)
#   define BOOST_STACKTRACE_USE_WINDBG_CACHED 1
#elif !COMPILER_CLANG // no idea how to cleanly make clang find backtrace
#   define BOOST_STACKTRACE_USE_BACKTRACE 1
#endif
#include <boost/stacktrace.hpp>
#include <unordered_map>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>


namespace common
{

class StackExplainer
{
private:
    std::thread WorkThread;
    std::mutex CallerMutex;
    std::condition_variable CallerCV;
    std::mutex WorkerMutex;
    std::condition_variable WorkerCV;
    std::unordered_map<std::string, SharedString<char16_t>> FileCache;
    const boost::stacktrace::stacktrace* Stk;
    std::vector<StackTraceItem>* Output;
    bool ShouldRun;
public:
    StackExplainer() noexcept : ShouldRun(true)
    {
        FileCache.insert({});
        std::unique_lock<std::mutex> callerLock(CallerMutex);
        std::unique_lock<std::mutex> workerLock(WorkerMutex);
        WorkThread = std::thread([&]()
            {
                SetThreadName(u"StackExplainer");
                std::unique_lock<std::mutex> lock(WorkerMutex);
                CallerCV.notify_one();
                WorkerCV.wait(lock);
                while (ShouldRun)
                {
                    for (auto frame = Stk->begin(); frame != Stk->end(); ++frame)
                    {
                        try
                        {
                            auto fileName = frame->source_file();
                            SharedString<char16_t> file;
                            if (const auto it = FileCache.find(fileName); it != FileCache.end())
                                file = it->second;
                            else
                            {
                                const auto file16 = str::to_u16string(fileName);
                                file = FileCache.emplace(std::move(fileName), file16).first->second;
                            }
                            Output->emplace_back(std::move(file), str::to_u16string(frame->name()), frame->source_line());
                        }
                        catch (...)
                        {
                        }
                    }
                    CallerCV.notify_one();
                    WorkerCV.wait(lock);
                }
                CallerCV.notify_one();
            });
        CallerCV.wait(workerLock);
    }
    ~StackExplainer()
    {
        std::unique_lock<std::mutex> callerLock(CallerMutex);
        std::unique_lock<std::mutex> workerLock(WorkerMutex);
        if (WorkThread.joinable())
        {
            ShouldRun = false;
            WorkerCV.notify_one();
            CallerCV.wait(workerLock);
            WorkThread.join();
        }
    }
    std::vector<StackTraceItem> Explain(const boost::stacktrace::stacktrace& st) noexcept
    {
        std::vector<StackTraceItem> ret;
        {
            std::unique_lock<std::mutex> callerLock(CallerMutex);
            std::unique_lock<std::mutex> workerLock(WorkerMutex);
            Stk = &st; Output = &ret;
            WorkerCV.notify_one();
            CallerCV.wait(workerLock);
        }
        return ret;
    }
};


std::vector<StackTraceItem> GetStack(size_t skip) noexcept
{
#ifdef _DEBUG
    skip += 3;
#endif
    static StackExplainer Explainer;
    boost::stacktrace::stacktrace st(skip, UINT32_MAX);
    return Explainer.Explain(st);
}


}
