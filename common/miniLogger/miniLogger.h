#pragma once
#include "MiniLoggerRely.h"


namespace common::mlog
{


MINILOGAPI uint32_t __cdecl AddGlobalCallback(const MLoggerCallback& cb);
MINILOGAPI void __cdecl DelGlobalCallback(const uint32_t id);

namespace detail
{

class MINILOGAPI MiniLoggerBase : public NonCopyable
{
    friend uint32_t __cdecl ::common::mlog::AddGlobalCallback(const MLoggerCallback& cb);
    friend void __cdecl ::common::mlog::DelGlobalCallback(const uint32_t id);
protected:
    static const std::unique_ptr<LoggerBackend> GlobalOutputer;
    std::atomic<LogLevel> LeastLevel;
    const std::u16string Prefix;
    std::set<std::shared_ptr<LoggerBackend>> Outputer;
    forceinline void AddRefCount(LogMessage& msg, const size_t count) { msg.RefCount += (uint32_t)count; }
public:
    MiniLoggerBase(const std::u16string& name, const std::set<std::shared_ptr<LoggerBackend>>& outputer = {}, const LogLevel level = LogLevel::Debug)
        : Prefix(name), Outputer(outputer), LeastLevel(level)
    { }
    void SetLeastLevel(const LogLevel level) { LeastLevel = level; }
    LogLevel GetLeastLevel() { return LeastLevel; }

    template<class... Args>
    LogMessage* GenerateLogMsg(const LogLevel level, const fmt::BasicCStringRef<char>& formater, Args&&... args)
    {
        return LogMessage::MakeMessage(Prefix, StrFormater<char>::ToU16Str(formater, std::forward<Args>(args)...), level);
    }
    template<class... Args>
    LogMessage* GenerateLogMsg(const LogLevel level, const fmt::BasicCStringRef<char16_t>& formater, Args&&... args)
    {
        return LogMessage::MakeMessage(Prefix, StrFormater<char16_t>::ToU16Str(formater, std::forward<Args>(args)...), level);
    }
    template<class... Args>
    LogMessage* GenerateLogMsg(const LogLevel level, const fmt::BasicCStringRef<char32_t>& formater, Args&&... args)
    {
        return LogMessage::MakeMessage(Prefix, StrFormater<char32_t>::ToU16Str(formater, std::forward<Args>(args)...), level);
    }
    template<class... Args>
    LogMessage* GenerateLogMsg(const LogLevel level, const fmt::BasicCStringRef<wchar_t>& formater, Args&&... args)
    {
        return LogMessage::MakeMessage(Prefix, StrFormater<wchar_t>::ToU16Str(formater, std::forward<Args>(args)...), level);
    }
};

}


template<bool DynamicBackend>
class COMMONTPL MiniLogger : public detail::MiniLoggerBase
{
protected:
    common::WRSpinLock WRLock;
public:
    using detail::MiniLoggerBase::MiniLoggerBase;
    template<typename Char, class... Args>
    void error(const Char* __restrict formater, Args&&... args)
    {
        log<Char>(LogLevel::Error, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void warning(const Char* __restrict formater, Args&&... args)
    {
        log<Char>(LogLevel::Warning, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void success(const Char* __restrict formater, Args&&... args)
    {
        log<Char>(LogLevel::Success, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void verbose(const Char* __restrict formater, Args&&... args)
    {
        log<Char>(LogLevel::Verbose, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void info(const Char* __restrict formater, Args&&... args)
    {
        log<Char>(LogLevel::Info, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void debug(const Char* __restrict formater, Args&&... args)
    {
        log<Char>(LogLevel::Debug, formater, std::forward<Args>(args)...);
    }

    template<typename Char, class... Args>
    void error(const std::basic_string<Char>& formater, Args&&... args)
    {
        log<Char>(LogLevel::Error, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void warning(const std::basic_string<Char>& formater, Args&&... args)
    {
        log<Char>(LogLevel::Warning, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void success(const std::basic_string<Char>& formater, Args&&... args)
    {
        log<Char>(LogLevel::Success, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void verbose(const std::basic_string<Char>& formater, Args&&... args)
    {
        log<Char>(LogLevel::Verbose, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void info(const std::basic_string<Char>& formater, Args&&... args)
    {
        log<Char>(LogLevel::Info, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void debug(const std::basic_string<Char>& formater, Args&&... args)
    {
        log<Char>(LogLevel::Debug, formater, std::forward<Args>(args)...);
    }

    template<typename Char, class... Args>
    void log(const LogLevel level, const fmt::BasicCStringRef<Char>& formater, Args&&... args)
    {
        if ((uint8_t)level < (uint8_t)LeastLevel.load(std::memory_order_relaxed))
            return;
        LogMessage* msg = GenerateLogMsg(level, formater, std::forward<Args>(args)...);
        
        AddRefCount(*msg, 1);
        GlobalOutputer->Print(msg);

        if constexpr(DynamicBackend)
        {
            WRLock.LockRead();
        }
        AddRefCount(*msg, Outputer.size());
        for (auto& backend : Outputer)
            backend->Print(msg);
        LogMessage::Consume(msg);
        if constexpr(DynamicBackend)
        {
            WRLock.UnlockRead();
        }
    }

    void AddOuputer(const std::shared_ptr<LoggerBackend>& outputer)
    {
        static_assert(DynamicBackend, "Add output only supported by Dynamic-backend Logger");
        WRLock.LockWrite();
        Outputer.insert(outputer);
        WRLock.UnlockWrite();
    }
    void DelOutputer(const std::shared_ptr<LoggerBackend>& outputer)
    {
        static_assert(DynamicBackend, "Del output only supported by Dynamic-backend Logger");
        WRLock.LockWrite();
        Outputer.erase(outputer);
        WRLock.UnlockWrite();
    }
};


}

