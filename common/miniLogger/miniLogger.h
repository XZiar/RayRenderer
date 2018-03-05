#pragma once
#include "MiniLoggerRely.h"


namespace common::mlog
{

enum class LogOutput : uint8_t { None = 0x0, Console = 0x1, File = 0x2, Callback = 0x4, Buffer = 0x8, Debugger = 0x10 };
MAKE_ENUM_BITFIELD(LogOutput)

/*-return whether should continue logging*/
using LogCallBack = std::function<bool __cdecl(const LogLevel lv, const std::wstring& content)>;
/*global log callback*/
using GLogCallBack = std::function<void __cdecl(const LogLevel lv, const std::wstring& from, const std::wstring& content)>;
/*
Logging to Buffer is not thread-safe since it's now lack of memmory barrier
Changing logging file caould is not thread-safe since it's now lack of memmory barrier
I am tired of considering these situations cause I just wanna write a simple library for my own use
And I guess no one would see the words above, so I decide just to leave them alone
*/
class MINILOGAPI logger : public NonCopyable
{
public:
private:
    std::atomic_flag flagConsole = ATOMIC_FLAG_INIT, flagFile = ATOMIC_FLAG_INIT, flagCallback = ATOMIC_FLAG_INIT, flagBuffer = ATOMIC_FLAG_INIT;
    std::atomic<LogLevel> leastLV;
    std::atomic<LogOutput> outputs;
    bool ownFile = false;
    const std::wstring name, prefix;
    FILE *fp;
    std::vector<std::pair<LogLevel, std::wstring>> buffer;
    static GLogCallBack onGlobalLog;
    LogCallBack onLog;
    bool checkLevel(const LogLevel lv) { return (uint8_t)lv >= (uint8_t)leastLV.load(); }
    void printDebugger(const LogLevel lv, const std::wstring& content);
    void printConsole(const LogLevel lv, const std::wstring& content);
    void printFile(const LogLevel lv, const std::wstring& content);
    void printBuffer(const LogLevel lv, const std::wstring& content);
    bool onCallBack(const LogLevel lv, const std::wstring& content);
    void innerLog(const LogLevel lv, const std::wstring& content);
public:
    logger(const std::wstring loggername, const std::wstring logfile, LogCallBack cb = nullptr, const LogOutput lo = LogOutput::Console, const LogLevel lv = LogLevel::Info);
    logger(const std::wstring loggername, FILE * const logfile = nullptr, LogCallBack cb = nullptr, const LogOutput lo = LogOutput::Console, const LogLevel lv = LogLevel::Info);
    ~logger();
    static void __cdecl setGlobalCallBack(GLogCallBack cb);
    void setLeastLevel(const LogLevel lv);
    void setOutput(const LogOutput method, const bool isEnable);
    std::vector<std::pair<LogLevel, std::wstring>> __cdecl getLogBuffer();
    template<class... Args>
    void error(const std::wstring& formater, Args&&... args)
    {
        log(LogLevel::Error, formater, std::forward<Args>(args)...);
    }
    template<class... Args>
    void warning(const std::wstring& formater, Args&&... args)
    {
        log(LogLevel::Warning, formater, std::forward<Args>(args)...);
    }
    template<class... Args>
    void success(const std::wstring& formater, Args&&... args)
    {
        log(LogLevel::Sucess, formater, std::forward<Args>(args)...);
    }
    template<class... Args>
    void verbose(const std::wstring& formater, Args&&... args)
    {
        log(LogLevel::Verbose, formater, std::forward<Args>(args)...);
    }
    template<class... Args>
    void info(const std::wstring& formater, Args&&... args)
    {
        log(LogLevel::Info, formater, std::forward<Args>(args)...);
    }
    template<class... Args>
    void debug(const std::wstring& formater, Args&&... args)
    {
        log(LogLevel::Debug, formater, std::forward<Args>(args)...);
    }
    template<class... Args>
    void log(const LogLevel lv, const std::wstring& formater, Args&&... args)
    {
        if (!checkLevel(lv))
            return;
        std::wstring logdat = fmt::format(formater, std::forward<Args>(args)...);
        if (onGlobalLog)
            onGlobalLog(lv, name, logdat);
        logdat = prefix + logdat;
        innerLog(lv, logdat);
    }
};


namespace detail
{
template<typename Char>
struct MINILOGAPI StrFormater;
template<>
struct MINILOGAPI StrFormater<char>
{
    static fmt::BasicMemoryWriter<char>& GetWriter();
    template<class... Args>
    static std::u16string ToU16Str(const fmt::BasicCStringRef<char>& formater, Args&&... args)
    {
        auto& writer = GetWriter();
        writer.clear();
        writer.write(formater, std::forward<Args>(args)...);
        return std::u16string(writer.data(), writer.data() + writer.size());
    }
};
template<>
struct MINILOGAPI StrFormater<char16_t>
{
    static fmt::UTFMemoryWriter<char16_t>& GetWriter();
    template<class... Args>
    static std::u16string ToU16Str(const fmt::BasicCStringRef<char16_t>& formater, Args&&... args)
    {
        auto& writer = GetWriter();
        writer.clear();
        writer.write(formater, std::forward<Args>(args)...);
        return writer.c_str();
    }
};

template<>
struct MINILOGAPI StrFormater<char32_t>
{
    static fmt::UTFMemoryWriter<char32_t>& GetWriter();
    template<class... Args>
    static std::u16string ToU16Str(const fmt::BasicCStringRef<char32_t>& formater, Args&&... args)
    {
        auto& writer = GetWriter();
        writer.clear();
        writer.write(formater, std::forward<Args>(args)...);
        return common::str::detail::CharsetConvertor<common::str::detail::UTF32, common::str::detail::UTF16, char32_t, char16_t>::Convert(writer.data(), writer.size(), true, true);
    }
};

template<>
struct MINILOGAPI StrFormater<wchar_t>
{
    static fmt::BasicMemoryWriter<wchar_t>& GetWriter();
    template<class... Args>
    static std::u16string ToU16Str(const fmt::BasicCStringRef<wchar_t>& formater, Args&&... args)
    {
        auto& writer = GetWriter();
        writer.clear();
        writer.write(formater, std::forward<Args>(args)...);
        if constexpr(sizeof(wchar_t) == sizeof(char16_t))
            return *(std::u16string*)&writer.c_str();
        else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
            return common::str::detail::CharsetConvertor<common::str::detail::UTF32, common::str::detail::UTF16, wchar_t, char16_t>::Convert(writer.data(), writer.size(), true, true);;
        else
            return u"";
    }
};

class MINILOGAPI MiniLoggerGBase : public NonCopyable
{
protected:
    static MLoggerCallback OnLog;
    std::atomic<LogLevel> LeastLevel;
    const std::u16string Prefix;
    std::set<std::shared_ptr<LoggerBackend>> Outputer;
    static fmt::UTFMemoryWriter<char16_t>& GetWriter();
public:
    //set global callback is not thread-safe
    static void __cdecl SetGlobalCallback(const MLoggerCallback& cb);
    //static std::shared_ptr<LoggerBackend> __cdecl GetConsoleBackend();
    MiniLoggerGBase(const std::u16string& name, std::set<std::shared_ptr<LoggerBackend>>&& outputer) : Prefix(name), Outputer(outputer)
    { }
    MiniLoggerGBase(const std::u16string& name, const std::set<std::shared_ptr<LoggerBackend>>& outputer = {}) : Prefix(name), Outputer(outputer)
    { }
    void SetLeastLevel(const LogLevel level) { LeastLevel = level; }
};
template<class Child>
class COMMONTPL MiniLoggerBase : public MiniLoggerGBase
{
protected:
    using detail::MiniLoggerGBase::MiniLoggerGBase;
    
public:
    template<typename Char, class... Args>
    void error(const Char* __restrict formater, Args&&... args)
    {
        log(LogLevel::Error, fmt::BasicCStringRef<Char>(formater), std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void warning(const Char* __restrict formater, Args&&... args)
    {
        log(LogLevel::Warning, fmt::BasicCStringRef<Char>(formater), std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void success(const Char* __restrict formater, Args&&... args)
    {
        log(LogLevel::Sucess, fmt::BasicCStringRef<Char>(formater), std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void verbose(const Char* __restrict formater, Args&&... args)
    {
        log(LogLevel::Verbose, fmt::BasicCStringRef<Char>(formater), std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void info(const Char* __restrict formater, Args&&... args)
    {
        log(LogLevel::Info, fmt::BasicCStringRef<Char>(formater), std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void debug(const Char* __restrict formater, Args&&... args)
    {
        log(LogLevel::Debug, fmt::BasicCStringRef<Char>(formater), std::forward<Args>(args)...);
    }

    template<typename Char, class... Args>
    void error(const std::basic_string<Char>& formater, Args&&... args)
    {
        log(LogLevel::Error, fmt::BasicCStringRef<Char>(formater), std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void warning(const std::basic_string<Char>& formater, Args&&... args)
    {
        log(LogLevel::Warning, fmt::BasicCStringRef<Char>(formater), std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void success(const std::basic_string<Char>& formater, Args&&... args)
    {
        log(LogLevel::Sucess, fmt::BasicCStringRef<Char>(formater), std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void verbose(const std::basic_string<Char>& formater, Args&&... args)
    {
        log(LogLevel::Verbose, fmt::BasicCStringRef<Char>(formater), std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void info(const std::basic_string<Char>& formater, Args&&... args)
    {
        log(LogLevel::Info, fmt::BasicCStringRef<Char>(formater), std::forward<Args>(args)...);
    }
    template<typename Char, class... Args>
    void debug(const std::basic_string<Char>& formater, Args&&... args)
    {
        log(LogLevel::Debug, fmt::BasicCStringRef<Char>(formater), std::forward<Args>(args)...);
    }

    template<typename Char, class... Args>
    void log(const LogLevel level, const fmt::BasicCStringRef<Char>& formater, Args&&... args)
    {
        if ((uint8_t)level < (uint8_t)LeastLevel.load())
            return;
        //const std::wstring logdat = fmt::format(formater, std::forward<Args>(args)...);
        LogMessage* msg = new LogMessage(Prefix, detail::StrFormater<Char>::ToU16Str(formater, std::forward<Args>(args)...), level);
        if (OnLog)
            OnLog(*msg);
        ((Child*)this)->LockLog();
        msg->RefCount = (uint32_t)Outputer.size();
        for (auto& backend : Outputer)
            backend->Print(msg);
        ((Child*)this)->UnlockLog();
    }
};
}

template<bool DynamicBend>
class MiniLogger;
template<>
class MiniLogger<true> : public detail::MiniLoggerBase<MiniLogger<true>>
{
    friend class detail::MiniLoggerBase<MiniLogger<true>>;
private:
    common::WRSpinLock WRLock; 
protected:
    void LockLog() { WRLock.LockRead(); }
    void UnlockLog() { WRLock.UnlockRead();  }
public:
    using detail::MiniLoggerBase<MiniLogger<true>>::MiniLoggerBase;
    void AddOuputer(const std::shared_ptr<LoggerBackend>& outputer)
    {
        WRLock.LockWrite();
        Outputer.insert(outputer);
        WRLock.UnlockWrite();
    }
    void DelOutputer(const std::shared_ptr<LoggerBackend>& outputer)
    {
        WRLock.LockWrite();
        Outputer.erase(outputer);
        WRLock.UnlockWrite();
    }
};

template<>
class MiniLogger<false> : public detail::MiniLoggerBase<MiniLogger<false>>
{
    friend class detail::MiniLoggerBase<MiniLogger<false>>;
protected:
    void LockLog() {}
    void UnlockLog() {}
public:
    using detail::MiniLoggerBase<MiniLogger<false>>::MiniLoggerBase;
};


}

