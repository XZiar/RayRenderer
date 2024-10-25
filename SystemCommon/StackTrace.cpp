#include "SystemCommonPch.h"
#include "StackTrace.h"
#include "ThreadEx.h"
#include "SystemCommon/StringConvert.h"
#include "common/SharedString.hpp"
#if defined(__cpp_lib_stacktrace) && __cpp_lib_stacktrace >= 202011L
# define CM_ST_STD 1
# include <stacktrace>
using Frame = std::stacktrace_entry;
using Trace = std::basic_stacktrace;
# pragma message("Compiling SystemCommon with std stacktrace[" STRINGIZE(__cpp_lib_stacktrace) "]" )
#else
# if COMMON_OS_WIN
#   define BOOST_STACKTRACE_USE_WINDBG_CACHED 1
# elif !COMMON_COMPILER_CLANG // no idea how to cleanly make clang find backtrace
#   define BOOST_STACKTRACE_USE_BACKTRACE 1
# endif
# define CM_ST_STD 0
# include <boost/stacktrace.hpp>
# include <boost/version.hpp>
using Frame = boost::stacktrace::frame;
using Trace = boost::stacktrace::stacktrace;
# pragma message("Compiling SystemCommon with boost stacktrace[" STRINGIZE(BOOST_LIB_VERSION) "]" )
#endif
#include <unordered_map>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <variant>


namespace common
{

using FrameRequest = std::pair<const Frame*, StackTraceItem*>;
using StackRequest = std::pair<const Trace*, std::vector<StackTraceItem>*>;


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
    SharedString<char16_t> CachedFileName(const Frame& frame)
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
            ThreadObject::GetCurrentThreadObject().SetName(u"StackExplainer");
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
                    const auto [st, output] = std::get<2>(Request);
                    for (const auto& frame : *st)
                    {
                        try
                        {
                            output->emplace_back(CachedFileName(frame), 
#if CM_ST_STD
                                str::to_u16string(frame.description()),
#else
                                str::to_u16string(frame.name()),
#endif
                                frame.source_line());
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
    std::vector<StackTraceItem> Explain(const Trace& st) noexcept
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
    StackTraceItem Explain(const Frame& frame) noexcept
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
#if CM_ST_STD
    auto st = Trace::current((skip, UINT32_MAX);
#else
    Trace st(skip, UINT32_MAX);
#endif
    return StackExplainer::GetExplainer().Explain(st);
}

AlignedBuffer GetDelayedStack(size_t skip) noexcept
{
#ifdef _DEBUG
    skip += 3;
#endif
#if CM_ST_STD
    auto st = Trace::current((skip, UINT32_MAX);
#else
    Trace st(skip, UINT32_MAX);
#endif
    AlignedBuffer ret(st.size() * sizeof(Frame), alignof(Frame));
    for (size_t i = 0; i < st.size(); ++i)
        ret.GetRawPtr<Frame>()[i] = st[i];
    return ret;
}


StackTraceItem ExceptionBasicInfo::GetStack(size_t idx) const noexcept
{
    if (idx < StackTrace.size())
        return StackTrace[idx];
    else
    {
        const auto ptr = DelayedStacks.GetRawPtr<Frame>();
        return StackExplainer::GetExplainer().Explain(ptr[idx - StackTrace.size()]);
    }
}
size_t ExceptionBasicInfo::GetStackCount() const noexcept
{
    return StackTrace.size() + DelayedStacks.GetSize() / sizeof(Frame);
}


}
