#include "MiniLoggerRely.h"
#include "miniLogger.h"
#include "QueuedBackend.h"
#include <map>

namespace common::mlog
{

class GlobalBackend final : public LoggerQBackend
{
private:
    std::atomic_uint32_t Idx = 0;
    std::mutex ModifyMtx;
    std::map<uint32_t, MLoggerCallback> Callbacks;
protected:
    void virtual OnStart() override
    {
        common::SetThreadName(u"Debugger-GlobalBackend");
    }
public:
    ~GlobalBackend() override { }
    void virtual OnPrint(const LogMessage& msg) override
    {
        std::unique_lock<std::mutex> lock(ModifyMtx);
        if (Callbacks.empty())
            return;
        for (const auto& item : Callbacks)
            item.second(msg);
    }
    uint32_t AddCallback(const MLoggerCallback& cb)
    {
        std::unique_lock<std::mutex> lock(ModifyMtx);
        const auto id = Idx++;
        Callbacks.emplace(id, cb);
        Start();
        return id;
    }
    void DelCallback(const uint32_t cbidx)
    {
        std::unique_lock<std::mutex> lock(ModifyMtx);
        Callbacks.erase(cbidx);
    }
};

namespace detail
{

const std::unique_ptr<LoggerBackend> MiniLoggerBase::GlobalOutputer = std::unique_ptr<LoggerBackend>(new GlobalBackend());

}

uint32_t CDECLCALL AddGlobalCallback(const MLoggerCallback& cb)
{
    return reinterpret_cast<GlobalBackend*>(detail::MiniLoggerBase::GlobalOutputer.get())->AddCallback(cb);
}
void CDECLCALL DelGlobalCallback(const uint32_t id)
{
    reinterpret_cast<GlobalBackend*>(detail::MiniLoggerBase::GlobalOutputer.get())->DelCallback(id);
}


}
