#include "SystemCommonPch.h"
#include "StackTrace.h"
#include "ThreadEx.h"
#include "SystemCommon/StringConvert.h"
#include "common/SharedString.hpp"
#if COMMON_OS_WIN
#   define BOOST_STACKTRACE_USE_WINDBG_CACHED 1
#elif !COMMON_COMPILER_CLANG // no idea how to cleanly make clang find backtrace
#   define BOOST_STACKTRACE_USE_BACKTRACE 1
#endif
#include <boost/stacktrace.hpp>
#include <unordered_map>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <variant>


namespace common
{

using FrameRequest = std::pair<const boost::stacktrace::frame*, StackTraceItem*>;
using StackRequest = std::pair<const boost::stacktrace::stacktrace*, std::vector<StackTraceItem>*>;


class StackExplainer
{
private:
    std::thread WorkThread;
    std::mutex CallerMutex;
    std::condition_variable CallerCV;
    std::mutex WorkerMutex;
    std::condition_variable WorkerCV;
    std::unordered_map<std::string, SharedString<char16_t>> FileCache;
    std::variant<std::monostate, FrameRequest, StackRequest> Request;
    uintptr_t ExitCallback = 0;
    bool ShouldRun;
    SharedString<char16_t> CachedFileName(const boost::stacktrace::frame& frame)
    {
        auto fileName = frame.source_file();
        if (const auto it = FileCache.find(fileName); it != FileCache.end())
            return it->second;
        else
        {
            const auto file16 = str::to_u16string(fileName);
            return FileCache.emplace(std::move(fileName), file16).first->second;
        }
    }
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
                ExitCallback = ExitCleaner::RegisterCleaner([&]() noexcept { this->Stop(); });
                CallerCV.notify_one();
                WorkerCV.wait(lock);
                while (ShouldRun)
                {
                    switch (Request.index())
                    {
                    case 1:
                    {
                        const auto [frame, output] = std::get<1>(Request);
                        *output = { CachedFileName(*frame), str::to_u16string(frame->name()), frame->source_line() };
                    } break;
                    case 2:
                    {
                        const auto [stks, output] = std::get<2>(Request);
                        for (auto frame = stks->begin(); frame != stks->end(); ++frame)
                        {
                            try
                            {
                                output->emplace_back(CachedFileName(*frame), str::to_u16string(frame->name()), frame->source_line());
                            }
                            catch (...)
                            {
                            }
                        }
                    } break;
                    default: break;
                    }
                    Request = std::monostate{};
                    CallerCV.notify_one();
                    WorkerCV.wait(lock);
                }
                CallerCV.notify_one();
            });
        CallerCV.wait(workerLock);
    }
    ~StackExplainer()
    {
        Stop();
    }
    void Stop()
    {
        std::unique_lock<std::mutex> callerLock(CallerMutex);
        std::unique_lock<std::mutex> workerLock(WorkerMutex);
        ShouldRun = false;
        if (WorkThread.joinable())
        {
            WorkerCV.notify_one();
            CallerCV.wait(workerLock);
            WorkThread.join();
        }
        ExitCleaner::UnRegisterCleaner(ExitCallback);
    }
    std::vector<StackTraceItem> Explain(const boost::stacktrace::stacktrace& st) noexcept
    {
        std::vector<StackTraceItem> ret;
        std::unique_lock<std::mutex> callerLock(CallerMutex);
        std::unique_lock<std::mutex> workerLock(WorkerMutex);
        if (ShouldRun)
        {
            Request = StackRequest{ &st, &ret };
            WorkerCV.notify_one();
            CallerCV.wait(workerLock);
        }
        return ret;
    }
    StackTraceItem Explain(const boost::stacktrace::frame& frame) noexcept
    {
        StackTraceItem ret;
        std::unique_lock<std::mutex> callerLock(CallerMutex);
        std::unique_lock<std::mutex> workerLock(WorkerMutex);
        if (ShouldRun)
        {
            Request = FrameRequest{ &frame, &ret };
            WorkerCV.notify_one();
            CallerCV.wait(workerLock);
        }
        return ret;
    }
    static StackExplainer& GetExplainer() noexcept
    {
        static StackExplainer Explainer;
        return Explainer;
    }
};


std::vector<StackTraceItem> GetStack(size_t skip) noexcept
{
#ifdef _DEBUG
    skip += 3;
#endif
    boost::stacktrace::stacktrace st(skip, UINT32_MAX);
    return StackExplainer::GetExplainer().Explain(st);
}

AlignedBuffer GetDelayedStack(size_t skip) noexcept
{
#ifdef _DEBUG
    skip += 3;
#endif
    boost::stacktrace::stacktrace st(skip, UINT32_MAX);
    const auto& stacks = st.as_vector();
    AlignedBuffer ret(stacks.size() * sizeof(boost::stacktrace::frame), alignof(boost::stacktrace::frame));
    memcpy_s(ret.GetRawPtr(), ret.GetSize(), stacks.data(), stacks.size() * sizeof(boost::stacktrace::frame));
    return ret;
}


StackTraceItem ExceptionBasicInfo::GetStack(size_t idx) const noexcept
{
    if (idx < StackTrace.size())
        return StackTrace[idx];
    else
    {
        const auto ptr = DelayedStacks.GetRawPtr<boost::stacktrace::frame>();
        return StackExplainer::GetExplainer().Explain(ptr[idx - StackTrace.size()]);
    }
}
size_t ExceptionBasicInfo::GetStackCount() const noexcept
{
    return StackTrace.size() + DelayedStacks.GetSize() / sizeof(boost::stacktrace::frame);
}


}
