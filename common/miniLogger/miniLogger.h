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

    template<typename Char, class... Args>
    LogMessage* GenerateLogMsg(const LogLevel level, const std::basic_string_view<Char>& formater, Args&&... args)
    {
        return LogMessage::MakeMessage(Prefix, StrFormater<Char>::ToU16Str(formater, std::forward<Args>(args)...), level);
    }
};

}


template<bool DynamicBackend>
class MINILOGAPI MiniLogger : public detail::MiniLoggerBase
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
    void error(const fmt::basic_memory_buffer<Char>& strBuffer)
    {
        log<Char>(LogLevel::Error, std::u16string_view(strBuffer.data(), strBuffer.size()));
    }
    template<typename Char, class... Args>
    void warning(const fmt::basic_memory_buffer<Char>& strBuffer)
    {
        log<Char>(LogLevel::Warning, std::u16string_view(strBuffer.data(), strBuffer.size()));
    }
    template<typename Char, class... Args>
    void success(const fmt::basic_memory_buffer<Char>& strBuffer)
    {
        log<Char>(LogLevel::Success, std::u16string_view(strBuffer.data(), strBuffer.size()));
    }
    template<typename Char, class... Args>
    void verbose(const fmt::basic_memory_buffer<Char>& strBuffer)
    {
        log<Char>(LogLevel::Verbose, std::u16string_view(strBuffer.data(), strBuffer.size()));
    }
    template<typename Char, class... Args>
    void info(const fmt::basic_memory_buffer<Char>& strBuffer)
    {
        log<Char>(LogLevel::Info, std::u16string_view(strBuffer.data(), strBuffer.size()));
    }
    template<typename Char, class... Args>
    void debug(const fmt::basic_memory_buffer<Char>& strBuffer)
    {
        log<Char>(LogLevel::Debug, std::u16string_view(strBuffer.data(), strBuffer.size()));
    }

    template<typename Char, class... Args>
    void error(const std::basic_string_view<Char>& formater, Args&&... args)
    {
        log<Char>(LogLevel::Error, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void warning(const std::basic_string_view<Char>& formater, Args&&... args)
    {
        log<Char>(LogLevel::Warning, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void success(const std::basic_string_view<Char>& formater, Args&&... args)
    {
        log<Char>(LogLevel::Success, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void verbose(const std::basic_string_view<Char>& formater, Args&&... args)
    {
        log<Char>(LogLevel::Verbose, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void info(const std::basic_string_view<Char>& formater, Args&&... args)
    {
        log<Char>(LogLevel::Info, formater, std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void debug(const std::basic_string_view<Char>& formater, Args&&... args)
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
    void log(const LogLevel level, const std::basic_string_view<Char>& formater, Args&&... args)
    {
        if ((uint8_t)level < (uint8_t)LeastLevel.load(std::memory_order_relaxed))
            return;
        LogMessage* msg = GenerateLogMsg(level, formater, std::forward<Args>(args)...);
        
        AddRefCount(*msg, 1);
        MiniLoggerBase::GlobalOutputer->Print(msg);

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

