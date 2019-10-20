#pragma once

#include "SystemCommonRely.h"
#include <atomic>
#include <any>
#include <memory>

namespace common::loop
{

class LoopBase;

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class SYSCOMMONAPI LoopExecutor : public NonCopyable
{
    friend class LoopBase;
protected:
    struct ControlBlock;
    ControlBlock* const Control;
private:
    LoopBase& Loop;
    std::any Cookie = {};
    enum class ExeStates : uint8_t { Stopped = 0, Starting = 1, Running = 2, Stopping = 3 };
    std::atomic<ExeStates> ExeState{ ExeStates::Stopped };
    //std::atomic_bool RequestStop{ false }, IsRunning{ false };
    enum class SleepStates : uint8_t { Running = 0, Pending = 1, Forbit = 2, Sleep = 3 };
    std::atomic<SleepStates> SleepState{ SleepStates::Running };
    std::atomic_bool WakeupLock{ false };
    bool Start(std::any cookie = {});
    bool Stop();
    void Wakeup();
    bool RequestStop();
    virtual void DoSleep(void* runningLock) noexcept = 0; // executor request sleep
    virtual void DoWakeup() noexcept = 0; // other requst executor to wake up
    virtual void DoStart() = 0; // when executor is requested to start
    virtual void WaitUtilStop() = 0; // when executor is requested to stop
protected:
    LoopExecutor(LoopBase& loop);
    bool TurnToRun() noexcept;
    void RunLoop() noexcept;
public:
    virtual ~LoopExecutor();
};

class SYSCOMMONAPI InplaceExecutor : public LoopExecutor
{
    friend class LoopBase;
protected:
    virtual void DoSleep(void* runningLock) noexcept override;
    virtual void DoWakeup() noexcept override;
    virtual void DoStart() override;
    virtual void WaitUtilStop() override;
public:    
    using LoopExecutor::LoopExecutor;
    bool RunInplace();
};

class SYSCOMMONAPI LoopBase : public NonCopyable, public NonMovable
{
    friend void LoopExecutor::RunLoop() noexcept;
private:
    std::unique_ptr<LoopExecutor> Host;
protected:
    enum class LoopState : uint8_t { Continue, Finish, Sleep };
    [[nodiscard]] bool IsRunning() const;
    void Wakeup() const;
    virtual LoopState OnLoop() = 0;
    [[nodiscard]] virtual bool SleepCheck() noexcept { return true; }; // double check if should sleep
    virtual bool OnStart(std::any) noexcept { return true; }
    virtual void OnStop() noexcept {}
    virtual bool OnError(std::exception_ptr) noexcept { return false; }
    template<typename T>
    LoopBase(const T& HostGenerator)
    {
        Host = HostGenerator(*this);
        if (!Host)
            COMMON_THROW(BaseException, u"Host invalid");
    }
    bool Start(std::any cookie = {});
    bool Stop();
    bool RequestStop();
    LoopExecutor& GetHost();
public:
    virtual ~LoopBase();

    [[nodiscard]] static std::unique_ptr<LoopExecutor> GetThreadedExecutor(LoopBase& loop);
    [[nodiscard]] static std::unique_ptr<LoopExecutor> GetInplaceExecutor(LoopBase& loop);
};

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

}