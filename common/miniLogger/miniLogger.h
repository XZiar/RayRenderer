#pragma once
#include "MiniLoggerRely.h"


namespace common::mlog
{


MINILOGAPI uint32_t AddGlobalCallback(const MLoggerCallback& cb);
MINILOGAPI void DelGlobalCallback(const uint32_t id);

namespace detail
{

class MINILOGAPI MiniLoggerBase : public NonCopyable
{
    friend uint32_t common::mlog::AddGlobalCallback(const MLoggerCallback& cb);
    friend void common::mlog::DelGlobalCallback(const uint32_t id);
protected:
    static const std::unique_ptr<LoggerBackend> GlobalOutputer;
    std::atomic<LogLevel> LeastLevel;
    const SharedString<char16_t> Prefix;
    std::set<std::shared_ptr<LoggerBackend>> Outputer;
    forceinline void AddRefCount(LogMessage& msg, const size_t count) { msg.RefCount += (uint32_t)count; }
public:
    MiniLoggerBase(const std::u16string& name, const std::set<std::shared_ptr<LoggerBackend>>& outputer = {}, const LogLevel level = LogLevel::Debug)
        : LeastLevel(level), Prefix(name), Outputer(outputer)
    { }
    void SetLeastLevel(const LogLevel level) { LeastLevel = level; }
    LogLevel GetLeastLevel() { return LeastLevel; }

};

}


template<bool DynamicBackend>
class MINILOGAPI MiniLogger : public detail::MiniLoggerBase
{
protected:
    common::WRSpinLock WRLock;
public:
    using detail::MiniLoggerBase::MiniLoggerBase;
   
    template<typename T, typename... Args>
    void error(T&& formatter, Args&&... args)
    {
        log(LogLevel::Error, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    void warning(T&& formatter, Args&&... args)
    {
        log(LogLevel::Warning, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    void success(T&& formatter, Args&&... args)
    {
        log(LogLevel::Success, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    void verbose(T&& formatter, Args&&... args)
    {
        log(LogLevel::Verbose, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    void info(T&& formatter, Args&&... args)
    {
        log(LogLevel::Info, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    void debug(T&& formatter, Args&&... args)
    {
        log(LogLevel::Debug, std::forward<T>(formatter), std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    void log(const LogLevel level, T&& formatter, Args&&... args)
    {
        if ((uint8_t)level < (uint8_t)LeastLevel.load(std::memory_order_relaxed))
            return;

        LogMessage* msg = nullptr;
        if constexpr (sizeof...(args) == 0)
            msg = LogMessage::MakeMessage(Prefix, std::forward<T>(formatter), level);
        else
            msg = LogMessage::MakeMessage(Prefix, detail::StrFormater::ToU16Str(std::forward<T>(formatter), std::forward<Args>(args)...), level);

        AddRefCount(*msg, 1);
        MiniLoggerBase::GlobalOutputer->Print(msg);

        if constexpr (DynamicBackend)
        {
            WRLock.LockRead();
        }
        AddRefCount(*msg, Outputer.size());
        for (auto& backend : Outputer)
            backend->Print(msg);
        LogMessage::Consume(msg);
        if constexpr (DynamicBackend)
        {
            WRLock.UnlockRead();
        }
    }

    void AddOuputer([[maybe_unused]]const std::shared_ptr<LoggerBackend>& outputer)
    {
        if constexpr (DynamicBackend)
        {
            WRLock.LockWrite();
            Outputer.insert(outputer);
            WRLock.UnlockWrite();
        }
        else
            COMMON_THROW(BaseException, u"Add output only supported by Dynamic-backend Logger");
    }
    void DelOutputer([[maybe_unused]]const std::shared_ptr<LoggerBackend>& outputer)
    {
        if constexpr (DynamicBackend)
        {
            WRLock.LockWrite();
            Outputer.erase(outputer);
            WRLock.UnlockWrite();
        }
        else
            COMMON_THROW(BaseException, u"Del output only supported by Dynamic-backend Logger");
    }
};


}

