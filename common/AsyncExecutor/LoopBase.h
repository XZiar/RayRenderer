#pragma once

#include "AsyncExecutorRely.h"


namespace common
{
namespace asyexe
{

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif
class ASYEXEAPI LoopBase : public NonCopyable, public NonMovable
{
private:
    struct LoopHost;
    LoopHost* const Host;
    std::atomic_bool RequestStop{ false }, IsRunning{ false };
    void MainLoop() noexcept;
protected:
    enum class LoopState : uint8_t { Continue, Finish, Sleep };
    bool ShouldStop() const { return RequestStop; }
    void Wakeup() const;
    virtual LoopState OnLoop() = 0;
    virtual bool OnStart() noexcept { return true; }
    virtual void OnStop() noexcept {}
    virtual bool OnError(std::exception_ptr) { return false; }
    LoopBase();
    bool Start();
    bool Stop();
public:
    virtual ~LoopBase();
};
#if COMPILER_MSVC
#   pragma warning(pop)
#endif

}
}