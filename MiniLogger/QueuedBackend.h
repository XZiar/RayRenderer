#pragma once
#include "MiniLoggerRely.h"
#include "SystemCommon/LoopBase.h"
#if defined(_MSC_VER)
#   define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING 1
#   pragma warning(disable:4996)
#   include "boost/lockfree/queue.hpp"
#   pragma warning(default:4996)
#else
#   include "boost/lockfree/queue.hpp"
#endif


namespace common::mlog
{

class MINILOGAPI LoggerQBackend : private loop::LoopBase, public LoggerBackend
{
private:
    boost::lockfree::queue<LogMessage*> MsgQueue;
    virtual bool SleepCheck() noexcept override; // double check if shoul sleep
    virtual LoopState OnLoop() override;
protected:
    using LoopBase::Stop;
    void EnsureRunning();
public:
    LoggerQBackend(const size_t initSize = 64);
    virtual ~LoggerQBackend() override;
    void virtual Print(LogMessage* msg) override final;
    template<class T, typename... Args>
    static std::unique_ptr<T> InitialQBackend(Args&&... args)
    {
        auto backend = std::make_unique<T>(std::forward<Args>(args)...);
        backend->Start();
        return backend;
    }
};


}
