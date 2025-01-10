#pragma once
#include "SystemCommonRely.h"
#include "Format.h"
#include "StringConvert.h"
#include "Delegate.h"
#include "SpinLock.h"
#include "common/StrBase.hpp"
#include "common/FileBase.hpp"
#include "common/EnumEx.hpp"
#include "common/SharedString.hpp"
#include <set>
#include <memory>

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace common::mlog
{

namespace detail
{

class MiniLoggerBase;

struct COMMON_EMPTY_BASES LoggerName : public FixedLenRefHolder<LoggerName, char16_t>
{
    friend RefHolder<LoggerName>;
    friend FixedLenRefHolder<LoggerName, char16_t>;
private:
    uintptr_t Ptr = 0;
    uint32_t U16Len = 0, U8Len = 0;
    [[nodiscard]] forceinline uintptr_t GetDataPtr() const noexcept
    {
        return Ptr;
    }
    forceinline void Destruct() noexcept { }
public:
    constexpr LoggerName() noexcept { }
    LoggerName(std::u16string_view name) noexcept;
    LoggerName(const LoggerName& other) noexcept : Ptr(other.Ptr), U16Len(other.U16Len), U8Len(other.U8Len)
    {
        this->Increase();
    }
    LoggerName(LoggerName&& other) noexcept : Ptr(other.Ptr), U16Len(other.U16Len), U8Len(other.U8Len)
    {
        other.Ptr = 0;
    }
    ~LoggerName()
    {
        this->Decrease();
    }
    std::u16string_view GetU16View() const noexcept
    {
        return { reinterpret_cast<const char16_t*>(Ptr), U16Len };
    }
    std::string_view GetU8View() const noexcept
    {
        return { reinterpret_cast<const char*>(Ptr + (U16Len + 1) * sizeof(char16_t)), U8Len };
    }
};

struct ColorSeperator;

template<typename Char>
struct LoggerFormatter final : public str::Formatter<Char>
{
private:
    void PutColor(std::basic_string<Char>&, ScreenColor color) final;
public:
    std::basic_string<Char> Str;
    std::unique_ptr<detail::ColorSeperator> Seperator;
    SYSCOMMONAPI LoggerFormatter();
    SYSCOMMONAPI ~LoggerFormatter();
    SYSCOMMONAPI span<const ColorSeg> GetColorSegements() const noexcept;
    SYSCOMMONAPI void Reset() noexcept;
};

}


struct LogMessage
{
    friend detail::MiniLoggerBase;
public:
    const uint64_t Timestamp;
private:
    const detail::LoggerName Source;
    std::atomic_uint32_t RefCount;
    const uint32_t Length;
    const uint16_t SegCount;
public:
    const LogLevel Level;
private:
    LogMessage(const detail::LoggerName& prefix, const uint32_t length, const uint16_t segCount, const LogLevel level, const uint64_t time)
        : Timestamp(time), Source(prefix), RefCount(1), Length(length), SegCount(segCount), Level(level) //RefCount is at first 1
    { }
    SYSCOMMONAPI static LogMessage* MakeMessage(const detail::LoggerName& prefix, const char16_t* content, const size_t len,
        span<const ColorSeg> seg, const LogLevel level, const uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count());
public:
    SYSCOMMONAPI static bool Consume(LogMessage* msg);

    COMMON_NO_COPY(LogMessage)
    COMMON_NO_MOVE(LogMessage)
    [[nodiscard]] std::u16string_view GetContent() const noexcept
    {
        const auto segSize = sizeof(ColorSeg) * SegCount;
        return { reinterpret_cast<const char16_t*>(reinterpret_cast<const std::byte*>(this) + sizeof(LogMessage) + segSize), Length };
    }
    [[nodiscard]] span<const ColorSeg> GetSegments() const noexcept
    {
        return { reinterpret_cast<const ColorSeg*>(reinterpret_cast<const std::byte*>(this) + sizeof(LogMessage)), SegCount };
    }
    template<bool IsU16 = true>
    [[nodiscard]] constexpr auto GetSource() const noexcept
    {
        if constexpr (IsU16)
            return Source.GetU16View();
        else
            return Source.GetU8View();
    }
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
    LoggerName Prefix;
    std::set<std::shared_ptr<LoggerBackend>> Outputer;

    SYSCOMMONAPI LogMessage* GenerateMessage(const LogLevel level, const str::StrArgInfoCh<char16_t>& strInfo, const str::ArgInfo& argInfo, span<const uint16_t> argStore, const str::NamedMapper& mapping) const;
    SYSCOMMONAPI LogMessage* GenerateMessage(const LogLevel level, std::basic_string_view<char16_t> formatter, const str::ArgInfo& argInfo, span<const uint16_t> argStore) const;
    template<typename Char>
    forceinline  LogMessage* GenerateMessage(const LogLevel level, const std::basic_string_view<Char> str) const
    {
        if constexpr (!std::is_same_v<Char, char16_t>)
        {
            const auto str16 = str::to_u16string(str);
            return LogMessage::MakeMessage(this->Prefix, str16.data(), str16.size(), {}, level);
        }
        else
        {
            return LogMessage::MakeMessage(this->Prefix, str.data(), str.size(), {}, level);
        }
    }

    SYSCOMMONAPI static void SentToGlobalOutputer(LogMessage* msg);
    forceinline void AddRefCount(LogMessage& msg, const size_t count) noexcept { msg.RefCount += (uint32_t)count; }
public:
    COMMON_NO_COPY(MiniLoggerBase)
    SYSCOMMONAPI MiniLoggerBase(const std::u16string& name, std::set<std::shared_ptr<LoggerBackend>> outputer = {}, const LogLevel level = LogLevel::Debug);
    MiniLoggerBase(MiniLoggerBase&& other) noexcept:
        LeastLevel(other.LeastLevel.load()), Prefix(std::move(other.Prefix)), Outputer(std::move(other.Outputer)) 
    { };
    SYSCOMMONAPI ~MiniLoggerBase();
    void SetLeastLevel(const LogLevel level) noexcept { LeastLevel = level; }
    LogLevel GetLeastLevel() noexcept { return LeastLevel; }
};


}


template<bool DynamicBackend>
class MiniLogger : public detail::MiniLoggerBase
{
protected:
    common::spinlock::WRSpinLock WRLock;
public:
    using detail::MiniLoggerBase::MiniLoggerBase;
    template<bool T>
    MiniLogger(MiniLogger<T>&& other) noexcept :
        detail::MiniLoggerBase(std::move(other)), WRLock(std::move(other.WRLock))
    { }

    template<typename T, typename... Args>
    forceinline void Error(T&& formatter, Args&&... args)
    {
        Log(LogLevel::Error, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    forceinline void Warning(T&& formatter, Args&&... args)
    {
        Log(LogLevel::Warning, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    forceinline void Success(T&& formatter, Args&&... args)
    {
        Log(LogLevel::Success, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    forceinline void Verbose(T&& formatter, Args&&... args)
    {
        Log(LogLevel::Verbose, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    forceinline void Info(T&& formatter, Args&&... args)
    {
        Log(LogLevel::Info, std::forward<T>(formatter), std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    forceinline void Debug(T&& formatter, Args&&... args)
    {
        Log(LogLevel::Debug, std::forward<T>(formatter), std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    void Log(const LogLevel level, T&& formatter, Args&&... args)
    {
        using namespace str;
        if (level < LeastLevel.load(std::memory_order_relaxed))
            return;

        LogMessage* msg = nullptr;
        using U = std::decay_t<T>;
        if constexpr (is_specialization<U, StrArgInfoCh>::value)
        {
            using Char = typename U::CharType;
            static_assert(std::is_same_v<Char, char16_t>, "Log formatter need to be char16_t");

            static constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
            const auto mapping = ArgChecker::CheckDD(formatter, ArgsInfo);
            const auto argStore = ArgInfo::PackArgsStatic(std::forward<Args>(args)...);

            msg = GenerateMessage(level, formatter, ArgsInfo, argStore, mapping);
        }
        else if constexpr (std::is_base_of_v<CompileTimeFormatter, U>)
        {
            using Char = typename U::CharType;
            static_assert(std::is_same_v<Char, char16_t>, "Log formatter need to be char16_t");

            static constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
            static constexpr auto Mapping = ArgChecker::CheckSS<U, Args...>();
            const auto argStore = ArgInfo::PackArgsStatic(std::forward<Args>(args)...);

            msg = GenerateMessage(level, formatter.ToStrArgInfo(), ArgsInfo, argStore.ArgStore, Mapping);
        }
        else if constexpr (std::is_base_of_v<CustomFormatterTag, U>)
        {
            using Char = typename U::CharType;
            static_assert(std::is_same_v<Char, char16_t>, "Log formatter need to be char16_t");

            static constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
            const auto [strinfo, mapping] = formatter.template TranslateInfo<Args...>();
            const auto argStore = ArgInfo::PackArgsStatic(std::forward<Args>(args)...);
            
            msg = GenerateMessage(level, strinfo, ArgsInfo, argStore.ArgStore, mapping);
        }
        else
        {
            const auto fmtSv = ToStringView(formatter);
            if constexpr (sizeof...(args) == 0)
            {
                msg = GenerateMessage(level, fmtSv);
            }
            else
            {
                using Char = typename decltype(fmtSv)::value_type;
                static_assert(std::is_same_v<Char, char16_t>, "Log formatter need to be char16_t");

                static constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
                const auto argStore = ArgInfo::PackArgsStatic(std::forward<Args>(args)...);
                msg = GenerateMessage(level, fmtSv, ArgsInfo, argStore.ArgStore);
            }
        }

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
