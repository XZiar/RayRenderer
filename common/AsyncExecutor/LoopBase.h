#pragma once

#include "AsyncExecutorRely.h"


namespace common
{
namespace asyexe
{

class LoopBase;

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

class ASYEXEAPI LoopExecutor : public NonCopyable
{
    friend class LoopBase;
private:
    struct ControlBlock;
    bool Start();
    bool Stop();
protected:
    ControlBlock* const Control;
    std::atomic_bool RequestStop{ false }, IsRunning{ false };
    LoopBase& Loop;

    LoopExecutor(LoopBase& loop);
    void RunLoop() noexcept;

    virtual void Sleep() = 0; // executor request sleep
    virtual void Wakeup() = 0; // other requst executor to wake up
    virtual bool OnStart() = 0; // when executor is requested to start
    virtual bool OnStop() = 0; // when executor is requested to stop
public:
    virtual ~LoopExecutor();
};

class ASYEXEAPI InplaceExecutor : public LoopExecutor
{
    friend class LoopBase;
protected:
    virtual void Sleep() override;
    virtual void Wakeup() override;
    virtual bool OnStart() override;
    virtual bool OnStop() override;
public:
    using LoopExecutor::LoopExecutor;
    bool RunInplace();
};

class ASYEXEAPI LoopBase : public NonCopyable, public NonMovable
{
    friend void LoopExecutor::RunLoop() noexcept;
private:
    std::unique_ptr<LoopExecutor> Host;
    void MainLoop() noexcept;
protected:
    enum class LoopState : uint8_t { Continue, Finish, Sleep };
    bool ShouldStop() const { return Host->RequestStop; }
    void Wakeup() const;
    virtual LoopState OnLoop() = 0;
    virtual bool OnStart() noexcept { return true; }
    virtual void OnStop() noexcept {}
    virtual bool OnError(std::exception_ptr) noexcept { return false; }
    LoopBase(std::unique_ptr<LoopExecutor>&& host);
    bool Start();
    bool Stop();
public:
    virtual ~LoopBase();

    static std::unique_ptr<LoopExecutor> GetThreadedExecutor(LoopBase& loop);
    static std::unique_ptr<LoopExecutor> GetInplaceExecutor(LoopBase& loop);
};

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

}
}