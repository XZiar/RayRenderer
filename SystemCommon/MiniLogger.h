#pragma once
#include "SystemCommonRely.h"
#include "StringUtil/Convert.h"
#include "StringUtil/Format.h"
#include "common/FileBase.hpp"
#include "common/EnumEx.hpp"
#include "common/Delegate.hpp"
#include "common/SharedString.hpp"
#include "common/SpinLock.hpp"
#include <set>

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace common::mlog
{

enum class LogLevel : uint8_t { Debug = 20, Verbose = 40, Info = 60, Success = 70, Warning = 85, Error = 100, None = 120 };
MAKE_ENUM_RANGE(LogLevel)

namespace detail
{
class MiniLoggerBase;
}

struct LogMessage
{
    friend class detail::MiniLoggerBase;
public:
    const uint64_t Timestamp;
private:
    const SharedString<char16_t> Source;
    std::atomic_uint32_t RefCount;
    const uint32_t Length;
public:
    const LogLevel Level;
private:
    LogMessage(const SharedString<char16_t>& prefix, const uint32_t length, const LogLevel level, const uint64_t time)
        : Timestamp(time), Source(prefix), RefCount(1), Length(length), Level(level) //RefCount is at first 1
    { }
public:
    SYSCOMMONAPI static LogMessage* MakeMessage(const SharedString<char16_t>& prefix, const char16_t* content, const size_t len, const LogLevel level, const uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count());
    template<size_t N>
    forceinline static LogMessage* MakeMessage(const SharedString<char16_t>& prefix, const char16_t(&content)[N], const LogLevel level, const uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count())
    {
        return MakeMessage(prefix, content, N - 1, level, time);
    }
    template<typename T>
    forceinline static LogMessage* MakeMessage(const SharedString<char16_t>& prefix, const T& content, const LogLevel level, const uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count())
    {
        static_assert(std::is_convertible_v<decltype(std::declval<const T>().data()), const char16_t*>, "only accept container of char16_t");
        return MakeMessage(prefix, content.data(), content.size(), level, time);
    }
    SYSCOMMONAPI static bool Consume(LogMessage* msg);

    COMMON_NO_COPY(LogMessage)
    COMMON_NO_MOVE(LogMessage)
    std::u16string_view GetContent() const 
    { 
        return std::u16string_view(reinterpret_cast<const char16_t*>(reinterpret_cast<const uint8_t*>(this) + sizeof(LogMessage)), Length);
    }
    constexpr std::u16string_view GetSource() const { return Source; }
    SYSCOMMONAPI std::chrono::system_clock::time_point GetSysTime() const;
};


class SYSCOMMONAPI LoggerBackend
{
protected:
    volatile LogLevel LeastLevel = LogLevel::Debug; //non-atomic, should increase performance
    constexpr LoggerBackend() { }
    void virtual OnPrint(const LogMessage& msg) = 0;
public:
    COMMON_NO_COPY(LoggerBackend)
    virtual ~LoggerBackend() { }
    void virtual Print(LogMessage* msg);
    void SetLeastLevel(const LogLevel level) { LeastLevel = level; }
    LogLevel GetLeastLevel() { return LeastLevel; }
};


using MLoggerCallback = std::function<void(const LogMessage& msg)>;

SYSCOMMONAPI std::shared_ptr<LoggerBackend> GetConsoleBackend();
SYSCOMMONAPI void SyncConsoleBackend();
SYSCOMMONAPI std::shared_ptr<LoggerBackend> GetDebuggerBackend();
SYSCOMMONAPI std::shared_ptr<LoggerBackend> GetFileBackend(const fs::path& path);

SYSCOMMONAPI std::u16string_view GetLogLevelStr(const LogLevel level);
SYSCOMMONAPI CallbackToken AddGlobalCallback(const MLoggerCallback& cb);
SYSCOMMONAPI void DelGlobalCallback(const CallbackToken& token);


namespace detail
{

class MiniLoggerBase
{
    friend CallbackToken common::mlog::AddGlobalCallback(const MLoggerCallback& cb);
    friend void common::mlog::DelGlobalCallback(const CallbackToken& id);
protected:
    std::atomic<LogLevel> LeastLevel;
    SharedString<char16_t> Prefix;
    std::set<std::shared_ptr<LoggerBackend>> Outputer;

    SYSCOMMONAPI static void SentToGlobalOutputer(LogMessage* msg);
    forceinline void AddRefCount(LogMessage& msg, const size_t count) noexcept { msg.RefCount += (uint32_t)count; }
public:
    COMMON_NO_COPY(MiniLoggerBase)
    MiniLoggerBase(const std::u16string& name, std::set<std::shared_ptr<LoggerBackend>> outputer = {}, const LogLevel level = LogLevel::Debug) :
        LeastLevel(level), Prefix(name), Outputer(std::move(outputer))
    { }
    MiniLoggerBase(MiniLoggerBase&& other) noexcept:
        LeastLevel(other.LeastLevel.load()), Prefix(std::move(other.Prefix)), Outputer(std::move(other.Outputer)) 
    { };
    void SetLeastLevel(const LogLevel level) noexcept { LeastLevel = level; }
    LogLevel GetLeastLevel() noexcept { return LeastLevel; }
};


struct StrFormater
{
private:
    template<typename Char>
    static decltype(auto) BufToU16(std::vector<Char>& buffer)
    {
        if constexpr (std::is_same_v<Char, char16_t>)
            return buffer;
        else if constexpr (std::is_same_v<Char, char>)
            return common::str::to_u16string(buffer.data(), buffer.size(), common::str::Charset::UTF8);
        else if constexpr (std::is_same_v<Char, char32_t>)
            return common::str::to_u16string(buffer.data(), buffer.size(), common::str::Charset::UTF32LE);
        else
            static_assert(!common::AlwaysTrue<Char>, "unexpected Char type");
    }
    template<typename Char>
    SYSCOMMONAPI static std::vector<Char>& GetBuffer();
public:
    template<typename T, typename... Args>
    static decltype(auto) ToU16Str(const T& formatter, Args&&... args)
    {
        [[maybe_unused]] constexpr bool hasArgs = sizeof...(Args) > 0;
        if constexpr (std::is_base_of_v<fmt::compile_string, T>)
        {
            using Char = typename T::char_type;
            static_assert(!std::is_same_v<Char, wchar_t>, "no plan to support wchar_t at compile time");
            auto& buffer = GetBuffer<Char>();
            fmt::format_to(std::back_inserter(buffer), formatter, std::forward<Args>(args)...);
            return BufToU16(buffer);
        }
        else if constexpr (std::is_convertible_v<const T&, const std::string_view&>)
        {
            const auto u8str = static_cast<const std::string_view&>(formatter);
            if constexpr (!hasArgs)
                return common::str::to_u16string(u8str.data(), u8str.size(), common::str::Charset::UTF8);
            else
            {
                auto& buffer = GetBuffer<char>();
                fmt::format_to(std::back_inserter(buffer), u8str, std::forward<Args>(args)...);
                return BufToU16(buffer);
            }
        }
        else if constexpr (std::is_convertible_v<const T&, const std::u16string_view&>)
        {
            const auto u16str = static_cast<const std::u16string_view&>(formatter);
            if constexpr (!hasArgs)
                return u16str;
            else
            {
                auto& buffer = GetBuffer<char16_t>();
                fmt::format_to(std::back_inserter(buffer), u16str, std::forward<Args>(args)...);
                return BufToU16(buffer);
            }
        }
        else if constexpr (std::is_convertible_v<const T&, const std::u32string_view&>)
        {
            const auto u32str = static_cast<const std::u32string_view&>(formatter);
            if constexpr (!hasArgs)
                return common::str::to_u16string(u32str.data(), u32str.size(), common::str::Charset::UTF32LE);
            else
            {
                auto& buffer = GetBuffer<char32_t>();
                fmt::format_to(std::back_inserter(buffer), u32str, std::forward<Args>(args)...);
                return BufToU16(buffer);
            }
        }
        else if constexpr (std::is_convertible_v<const T&, const std::wstring_view&>)
        {
            const auto wstr = static_cast<const std::wstring_view&>(formatter);
            if constexpr (sizeof(wchar_t) == sizeof(char16_t))
                return ToU16Str(std::u16string_view(reinterpret_cast<const char16_t*>(wstr.data()), wstr.size()), std::forward<Args>(args)...);
            else if constexpr (sizeof(wchar_t) == sizeof(char32_t))
                return ToU16Str(std::u32string_view(reinterpret_cast<const char32_t*>(wstr.data()), wstr.size()), std::forward<Args>(args)...);
            else
                static_assert(!common::AlwaysTrue<T>, "unexpected wchar_t size");
        }
        else
        {
            static_assert(!common::AlwaysTrue<T>, "unknown formatter type");
        }
    }
};


}


template<bool DynamicBackend>
class MiniLogger : public detail::MiniLoggerBase
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
            msg = LogMessage::MakeMessage(Prefix, detail::StrFormater::ToU16Str(formatter), level);
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
