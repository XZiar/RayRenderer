#include "MiniLoggerRely.h"
#include "MiniLogger.h"
#include "QueuedBackend.h"
#include "SystemCommon/ThreadEx.h"
#include "common/Delegate.hpp"

namespace common::mlog
{

class GlobalBackend final : public LoggerQBackend
{
private:
    Delegate<const LogMessage&> DoPrint;
protected:
    virtual bool OnStart(std::any) noexcept override
    {
        common::SetThreadName(u"Debugger-GlobalBackend");
        return true;
    }
public:
    GlobalBackend() {}
    ~GlobalBackend() override { }
    void virtual OnPrint(const LogMessage& msg) override
    {
        DoPrint(msg);
    }
    CallbackToken AddCallback(const MLoggerCallback& cb)
    {
        const auto token = DoPrint += cb;
        EnsureRunning();
        return token;
    }
    bool DelCallback(const CallbackToken& token)
    {
        return DoPrint -= token;
    }
};

static GlobalBackend& GetGlobalOutputer()
{
    static const auto GlobalOutputer = std::make_unique<GlobalBackend>();
    return *GlobalOutputer;
}

CallbackToken AddGlobalCallback(const MLoggerCallback& cb)
{
    return GetGlobalOutputer().AddCallback(cb);
}
void DelGlobalCallback(const CallbackToken& token)
{
    GetGlobalOutputer().DelCallback(token);
}


void detail::MiniLoggerBase::SentToGlobalOutputer(LogMessage* msg)
{
    GetGlobalOutputer().Print(msg);
}


template class MINILOGAPI MiniLogger<true>;
template class MINILOGAPI MiniLogger<false>;

}
