#pragma once
#include "MiniLoggerRely.h"
#include "common/ThreadEx.h"
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING 1
#pragma warning(disable:4996)
#include "boost/lockfree/queue.hpp"
#pragma warning(default:4996)
#include <mutex>
#include <condition_variable>


namespace common::mlog
{

class MINILOGAPI LoggerQBackend : public LoggerBackend
{
protected:
    boost::lockfree::queue<LogMessage*> MsgQueue;
    ThreadObject CurThread;
    std::mutex RunningMtx;
    std::condition_variable CondWait;
    std::atomic_bool ShouldRun = false;
    std::atomic_bool IsWaiting = false;
    void LoggerWorker();
    void virtual OnPrint(const LogMessage& msg) = 0;
    void virtual OnStart() {}
    void virtual OnStop() {}
    void Start();
public:
    LoggerQBackend(const size_t initSize = 64);
    ~LoggerQBackend();
    void virtual Print(LogMessage* msg) override;
    void Flush();
    template<class T, typename... Args>
    static std::shared_ptr<LoggerQBackend> InitialQBackend(Args&&... args)
    {
        auto backend = std::make_shared<T>(std::forward<Args>(args)...);
        backend->Start();
        return std::dynamic_pointer_cast<LoggerQBackend>(backend);
    }
};


}
