#pragma once
#include "MiniLoggerRely.h"


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace common::mlog
{


MINILOGAPI CallbackToken AddGlobalCallback(const MLoggerCallback& cb);
MINILOGAPI void DelGlobalCallback(const CallbackToken& token);

namespace detail
{

class MINILOGAPI MiniLoggerBase : public NonCopyable
{
    friend CallbackToken common::mlog::AddGlobalCallback(const MLoggerCallback& cb);
    friend void common::mlog::DelGlobalCallback(const CallbackToken& id);
protected:
    std::atomic<LogLevel> LeastLevel;
    SharedString<char16_t> Prefix;
    std::set<std::shared_ptr<LoggerBackend>> Outputer;

    static void SentToGlobalOutputer(LogMessage* msg);
    forceinline void AddRefCount(LogMessage& msg, const size_t count) noexcept { msg.RefCount += (uint32_t)count; }
public:
    MiniLoggerBase(const std::u16string& name, std::set<std::shared_ptr<LoggerBackend>> outputer = {}, const LogLevel level = LogLevel::Debug) :
        LeastLevel(level), Prefix(name), Outputer(std::move(outputer))
    { }
    MiniLoggerBase(MiniLoggerBase&& other) noexcept:
        LeastLevel(other.LeastLevel.load()), Prefix(std::move(other.Prefix)), Outputer(std::move(other.Outputer)) 
    { };
    void SetLeastLevel(const LogLevel level) noexcept { LeastLevel = level; }
    LogLevel GetLeastLevel() noexcept { return LeastLevel; }

};

}


template<bool DynamicBackend>
class MINILOGAPI MiniLogger : public detail::MiniLoggerBase
{
protected:
    common::WRSpinLock WRLock;
public:
    using detail::MiniLoggerBase::MiniLoggerBase;
    template<bool T>
    MiniLogger(MiniLogger<T>&& other) noexcept :
        detail::MiniLoggerBase(std::move(other)), WRLock(std::move(other.WRLock))
    { }

    template<typename T, typename... Args>
    forceinline void error(T&& formatter, Args&&... args)
    {
        log(LogLevel::Error, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    forceinline void warning(T&& formatter, Args&&... args)
    {
        log(LogLevel::Warning, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    forceinline void success(T&& formatter, Args&&... args)
    {
        log(LogLevel::Success, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    forceinline void verbose(T&& formatter, Args&&... args)
    {
        log(LogLevel::Verbose, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    forceinline void info(T&& formatter, Args&&... args)
    {
        log(LogLevel::Info, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    forceinline void debug(T&& formatter, Args&&... args)
    {
        log(LogLevel::Debug, std::forward<T>(formatter), std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    void log(const LogLevel level, T&& formatter, Args&&... args)
    {
        if (level < LeastLevel.load(std::memory_order_relaxed))
            return;

        LogMessage* msg = nullptr;
        if constexpr (sizeof...(args) == 0)
            msg = LogMessage::MakeMessage(Prefix, std::forward<T>(formatter), level);
        else
            msg = LogMessage::MakeMessage(Prefix, detail::StrFormater::ToU16Str(std::forward<T>(formatter), std::forward<Args>(args)...), level);

        AddRefCount(*msg, 1);
        SentToGlobalOutputer(msg);

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

    template<bool R = DynamicBackend>
    std::enable_if_t<R, bool> AddOuputer(std::shared_ptr<LoggerBackend> outputer)
    {
        static_assert(DynamicBackend == R, "Should not override template arg");
        if (!outputer) return false;
        auto lock = WRLock.WriteScope();
        return Outputer.emplace(outputer).second;
    }
    template<bool R = DynamicBackend>
    std::enable_if_t<R, bool> DelOutputer(const std::shared_ptr<LoggerBackend>& outputer) noexcept
    {
        static_assert(DynamicBackend == R, "Should not override template arg");
        if (!outputer) return false;
        auto lock = WRLock.WriteScope();
        return Outputer.erase(outputer) > 0;
    }
};


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
