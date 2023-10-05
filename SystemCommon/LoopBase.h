#pragma once

#include "SystemCommonRely.h"
#include "ThreadEx.h"
#include "Exceptions.h"
#include <atomic>
#include <any>
#include <memory>

namespace common::loop
{

class LoopBase;

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class LoopExecutor
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
    virtual void DoSleep(void* runningLock, const uint32_t sleepTime) noexcept = 0; // executor request sleep
    virtual void DoWakeup() noexcept = 0; // other requst executor to wake up
    virtual void DoStart() = 0; // when executor is requested to start
    virtual void WaitUtilStop() = 0; // when executor is requested to stop
protected:
    COMMON_NO_COPY(LoopExecutor)
    COMMON_NO_MOVE(LoopExecutor)
    SYSCOMMONAPI LoopExecutor(LoopBase& loop);
    SYSCOMMONAPI bool TurnToRun() noexcept;
    SYSCOMMONAPI void RunLoop() noexcept;
public:
    SYSCOMMONAPI virtual ~LoopExecutor();
};

class InplaceExecutor : public LoopExecutor
{
    friend class LoopBase;
protected:
    void DoSleep(void* runningLock, const uint32_t sleepTime) noexcept override;
    void DoWakeup() noexcept override;
    void DoStart() override;
    void WaitUtilStop() override;
public:
    using LoopExecutor::LoopExecutor;
    SYSCOMMONAPI bool RunInplace();
};

class SYSCOMMONAPI LoopBase
{
    friend void LoopExecutor::RunLoop() noexcept;
public:
    struct LoopAction
    {
        friend void LoopExecutor::RunLoop() noexcept;
    private:
        static constexpr uint32_t ValSleep    = 0x80000000;
        static constexpr uint32_t ValContinue = 0x00000000;
        static constexpr uint32_t ValFinish   = 0xffffffff;

        uint32_t Val;
        constexpr LoopAction(const uint32_t val) noexcept : Val(val) { }
    public:
        static constexpr uint32_t MaxSleepTime = 0x7fffffff;
        constexpr bool operator==(const LoopAction other) const noexcept
        {
            return other.Val == Val;
        }
        constexpr uint32_t GetSleepTime() const noexcept { return Val; }

        static constexpr LoopAction Sleep()     noexcept { return ValSleep; }
        static constexpr LoopAction Continue()  noexcept { return ValContinue; }
        static constexpr LoopAction Finish()    noexcept { return ValFinish; }
        static constexpr LoopAction SleepFor(const uint32_t ms) noexcept { return std::min(ms, MaxSleepTime); }
    };
private:
    std::unique_ptr<LoopExecutor> Host;
protected:
    COMMON_NO_COPY(LoopBase)
    COMMON_NO_MOVE(LoopBase)
    [[nodiscard]] bool IsRunning() const;
    void Wakeup() const;
    virtual LoopAction OnLoop() = 0;
    [[nodiscard]] virtual bool SleepCheck() noexcept { return true; }; // double check if should sleep
    virtual bool OnStart(const ThreadObject&, std::any&) noexcept { return true; }
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
    [[nodiscard]] LoopExecutor& GetHost() const noexcept
    {
        return *Host;
    }
public:
    virtual ~LoopBase();

    [[nodiscard]] static std::unique_ptr<LoopExecutor> GetThreadedExecutor(LoopBase& loop);
    [[nodiscard]] static std::unique_ptr<LoopExecutor> GetInplaceExecutor(LoopBase& loop);
};

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

}