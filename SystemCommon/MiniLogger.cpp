#include "SystemCommonPch.h"
#include "MiniLogger.h"
#include "MiniLoggerBackend.h"
#include "FileEx.h"
#include "ThreadEx.h"
#include "ConsoleEx.h"
#include "common/AlignedBase.hpp"
#include <thread>
#include <array>
#if COMMON_OS_ANDROID
#   include <android/log.h>
#endif


#if COMMON_COMPILER_MSVC
#   define SYSCOMMONTPL SYSCOMMONAPI
#else
#   define SYSCOMMONTPL
#endif


namespace common::mlog
{
using namespace std::string_view_literals;

namespace detail
{

LoggerName::LoggerName(std::u16string_view u16name) noexcept
{
    if (u16name.empty()) return;
    const auto u8name = str::to_u8string(u16name);
    const auto u16len = u16name.size(), u8len = u8name.size();
    Expects(u16len < UINT32_MAX && u8len < UINT32_MAX);
    const auto totalLen = u16len + 1 + (u8len + 2) / 2;
    const auto space = FixedLenRefHolder<LoggerName, char16_t>::Allocate(totalLen); // add space for '\0'
    if (space.size() >= totalLen)
    {
        const auto u16Ptr = space.data();
        memcpy_s(u16Ptr, sizeof(char16_t) * u16len, u16name.data(), sizeof(char16_t) * u16len);
        u16Ptr[u16len] = u'\0';
        const auto u8Ptr = reinterpret_cast<char*>(u16Ptr + u16len + 1);
        memcpy_s(u8Ptr, sizeof(char) * u8len, u8name.data(), sizeof(char) * u8len);
        u8Ptr[u8len] = '\0';
        Ptr = reinterpret_cast<uintptr_t>(u16Ptr); 
        U16Len = static_cast<uint32_t>(u16len); 
        U8Len  = static_cast<uint32_t>(u8len);
    }
}


MiniLoggerBase::MiniLoggerBase(const std::u16string& name, std::set<std::shared_ptr<LoggerBackend>> outputer, const LogLevel level) :
    LeastLevel(level), Prefix(name), Outputer(std::move(outputer))
{ }
MiniLoggerBase::~MiniLoggerBase() { }


//template<typename Char>
//std::vector<Char>& StrFormater::GetBuffer()
//{
//    static thread_local std::vector<Char> out;
//    out.resize(0);
//    return out;
//}
//template std::vector<char>&     StrFormater::GetBuffer();
//template std::vector<wchar_t>&  StrFormater::GetBuffer();
//template std::vector<char16_t>& StrFormater::GetBuffer();
//template std::vector<char32_t>& StrFormater::GetBuffer();

}


using namespace std::chrono;

struct TimeConv 
{ 
    system_clock::time_point SysClock;
    uint64_t Base;
    TimeConv() : SysClock(system_clock::now()),
        Base(static_cast<uint64_t>(high_resolution_clock::now().time_since_epoch().count())) {}
};


LogMessage* LogMessage::MakeMessage(const detail::LoggerName& prefix, const char16_t* content, const size_t len, 
    common::span<const detail::ColorSeg> seg, const LogLevel level, const uint64_t time)
{
    constexpr auto MsgSize = sizeof(LogMessage);
    if (len >= UINT32_MAX - 1024)
        COMMON_THROW(BaseException, u"Too long for a single LogMessage!");
    const auto segSize = seg.size_bytes();
    const auto ptr = reinterpret_cast<std::byte*>(malloc_align(MsgSize + sizeof(char16_t) * (len + 1) + segSize, 8));
    if (!ptr)
        return nullptr; //not throw an exception yet
    LogMessage* msg = new (ptr)LogMessage(prefix, static_cast<uint32_t>(len), static_cast<uint16_t>(seg.size()), level, time);
    const auto segPtr = reinterpret_cast<detail::ColorSeg*>(ptr + MsgSize);
    memcpy_s(segPtr, segSize, seg.data(), segSize);
    const auto txtPtr = reinterpret_cast<char16_t*>(ptr + MsgSize + segSize);
    memcpy_s(txtPtr, sizeof(char16_t) * len, content, sizeof(char16_t) * len);
    txtPtr[len] = u'\0';
    return msg;
}

bool LogMessage::Consume(LogMessage* msg)
{
    if (msg->RefCount-- == 1) //last one
    {
        msg->~LogMessage();
        free_align(msg);
        return true;
    }
    return false;
}

system_clock::time_point LogMessage::GetSysTime() const
{
    static TimeConv TimeBase;
    return TimeBase.SysClock + duration_cast<system_clock::duration>(nanoseconds(Timestamp - TimeBase.Base));
}


void LoggerBackend::Print(LogMessage* msg)
{
    if (msg->Level >= LeastLevel)
        OnPrint(*msg);
    LogMessage::Consume(msg);
}


static constexpr auto LevelNumStr = []() 
{
    std::array<char16_t, 256 * 4> ret{ u'\0' };
    for (uint16_t i = 0, j = 0; i < 256; ++i)
    {
        ret[j++] = i / 100 + u'0';
        ret[j++] = (i % 100) / 10 + u'0';
        ret[j] = (i % 10) + u'0';
        j += 2;
    }
    return ret;
}();

std::u16string_view GetLogLevelStr(const LogLevel level)
{
    using namespace std::literals;
    switch (level)
    {
    case LogLevel::Debug:   return u"Debug"sv;
    case LogLevel::Verbose: return u"Verbose"sv;
    case LogLevel::Info:    return u"Info"sv;
    case LogLevel::Warning: return u"Warning"sv;
    case LogLevel::Success: return u"Success"sv;
    case LogLevel::Error:   return u"Error"sv;
    case LogLevel::None:    return u"None"sv;
    default: break;
    }
    const auto lvNum = static_cast<uint8_t>(level);
    auto ptr = &LevelNumStr[lvNum * 4];
    if (lvNum < 10)
        return { ptr + 2, 1 };
    else if (lvNum < 100)
        return { ptr + 1, 2 };
    else
        return { ptr + 0, 3 };
}


LoggerQBackend::LoggerQBackend(const size_t initSize) :
    LoopBase(LoopBase::GetThreadedExecutor), MsgQueue(initSize)
{ }
LoggerQBackend::~LoggerQBackend()
{
    //release the msgs left
    LogMessage* msg;
    while (MsgQueue.pop(msg))
        LogMessage::Consume(msg);
}

bool LoggerQBackend::SleepCheck() noexcept
{
    return MsgQueue.empty();
}
loop::LoopBase::LoopAction LoggerQBackend::OnLoop()
{
    uintptr_t ptr = 0;
    for (uint32_t i = 16; !MsgQueue.pop(ptr) && i--;)
    {
        std::this_thread::yield();
    }
    if (ptr == 0)
        return LoopAction::Sleep();
    switch (ptr % 4)
    {
    case 0:
    {
        const auto msg = reinterpret_cast<LogMessage*>(ptr);
        OnPrint(*msg);
        LogMessage::Consume(msg);
        return LoopAction::Continue();
    }
    case 1:
    {
        const auto pms = reinterpret_cast<common::BasicPromise<void>*>(ptr - 1);
        pms->SetData();
        return LoopAction::Continue();
    }
    default:
        Expects(false);
        return LoopAction::Continue(); // Shuld not enter
    }
}

void LoggerQBackend::EnsureRunning()
{
    if (!IsRunning())
        Start();
}

void LoggerQBackend::Print(LogMessage* msg)
{
    if (msg->Level < LeastLevel || !IsRunning())
    {
        LogMessage::Consume(msg);
    }
    else
    {
        MsgQueue.push(reinterpret_cast<uintptr_t>(msg));
        Wakeup();
    }
}

PromiseResult<void> LoggerQBackend::Synchronize()
{
    if (!IsRunning())
        return common::FinishedResult<void>::Get();
    const auto pms = new common::BasicPromise<void>();
    const auto ptr = reinterpret_cast<uintptr_t>(pms);
    Ensures(ptr % 4 == 0);
    MsgQueue.push(ptr + 1);
    Wakeup();
    return pms->GetPromiseResult();
}


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


namespace detail
{

struct ColorSeperator
{
    ScreenColor Foreground, Background;
    boost::container::small_vector<std::pair<uint32_t, ScreenColor>, 6> Segments;
    void HandleColor(const ScreenColor& color, size_t offset)
    {
        auto& target = color.IsBackground ? Background : Foreground;
        if (target == color)
            return;
        Segments.emplace_back(gsl::narrow_cast<uint32_t>(offset), color);
        target = color;
    }
    ColorSeperator() : Foreground(false), Background(true) {}
};

template<typename Char>
struct COMMON_EMPTY_BASES LoggerFormatter final : public str::Formatter<Char>, public ColorSeperator
{
private:
    void PutColor(std::basic_string<Char>&, ScreenColor color) final
    {
        HandleColor(color, Str.size());
    }
public:
    std::basic_string<Char> Str;
};


template<typename Char>
LogMessage* MiniLoggerBase::GenerateMessage(const LogLevel level, const str::StrArgInfoCh<Char>& strInfo, const str::ArgInfo& argInfo, const str::ArgPack& argPack) const
{
    LoggerFormatter<Char> formatter;
    str::FormatterBase::FormatTo(formatter, formatter.Str, strInfo, argInfo, argPack);
    const auto segs = to_span(formatter.Segments);
    if constexpr (!std::is_same_v<Char, char16_t>)
    {
        const auto str16 = str::to_u16string(formatter.Str);
        return LogMessage::MakeMessage(this->Prefix, str16.data(), str16.size(), segs, level);
    }
    else
    {
        return LogMessage::MakeMessage(this->Prefix, formatter.Str.data(), formatter.Str.size(), segs, level);
    }
}
template SYSCOMMONTPL LogMessage* MiniLoggerBase::GenerateMessage(const LogLevel level, const str::StrArgInfoCh<char    >& strInfo, const str::ArgInfo& argInfo, const str::ArgPack& argPack) const;
template SYSCOMMONTPL LogMessage* MiniLoggerBase::GenerateMessage(const LogLevel level, const str::StrArgInfoCh<wchar_t >& strInfo, const str::ArgInfo& argInfo, const str::ArgPack& argPack) const;
template SYSCOMMONTPL LogMessage* MiniLoggerBase::GenerateMessage(const LogLevel level, const str::StrArgInfoCh<char16_t>& strInfo, const str::ArgInfo& argInfo, const str::ArgPack& argPack) const;
template SYSCOMMONTPL LogMessage* MiniLoggerBase::GenerateMessage(const LogLevel level, const str::StrArgInfoCh<char32_t>& strInfo, const str::ArgInfo& argInfo, const str::ArgPack& argPack) const;

template<typename Char>
LogMessage* MiniLoggerBase::GenerateMessage(const LogLevel level, const std::basic_string_view<Char> str) const
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
template SYSCOMMONTPL LogMessage* MiniLoggerBase::GenerateMessage(const LogLevel level, const std::basic_string_view<char    > str) const;
template SYSCOMMONTPL LogMessage* MiniLoggerBase::GenerateMessage(const LogLevel level, const std::basic_string_view<wchar_t > str) const;
template SYSCOMMONTPL LogMessage* MiniLoggerBase::GenerateMessage(const LogLevel level, const std::basic_string_view<char16_t> str) const;
template SYSCOMMONTPL LogMessage* MiniLoggerBase::GenerateMessage(const LogLevel level, const std::basic_string_view<char32_t> str) const;


}

void detail::MiniLoggerBase::SentToGlobalOutputer(LogMessage* msg)
{
    GetGlobalOutputer().Print(msg);
}


class DebuggerBackend : public LoggerQBackend
{
protected:
    bool virtual OnStart(std::any) noexcept override
    {
        common::SetThreadName(u"Debugger-MLogger-Backend");
        return true;
    }
public:
#if COMMON_OS_ANDROID
    static constexpr int GetLogLevelPrio(const LogLevel level) noexcept
    {
        switch (level)
        {
        case LogLevel::Debug:   return ANDROID_LOG_DEBUG;
        case LogLevel::Verbose: return ANDROID_LOG_VERBOSE;
        case LogLevel::Info:    return ANDROID_LOG_INFO;
        case LogLevel::Warning: return ANDROID_LOG_WARN;
        case LogLevel::Success: return ANDROID_LOG_INFO;
        case LogLevel::Error:   return ANDROID_LOG_FATAL;
        case LogLevel::None:    return ANDROID_LOG_SILENT;
        default:                return ANDROID_LOG_DEFAULT;
        }
    }
    static void PrintText(const std::u16string_view txt, const std::string_view tag = {}, int prio = ANDROID_LOG_INFO)
    {
        const auto text = str::to_string(txt, str::Encoding::UTF8, str::Encoding::UTF16LE);
        __android_log_write(prio, tag.data(), text.c_str());
    }
    void OnPrint(const LogMessage& msg) override
    {
        PrintText(msg.GetContent(), msg.GetSource<false>(), GetLogLevelPrio(msg.Level));
    }
#else
#  if COMMON_OS_WIN
    static void PrintText(const std::u16string_view txt)
    {
        OutputDebugString((LPCWSTR)txt.data());
    }
#  else
    static void PrintText(const std::u16string_view txt)
    {
        const auto text = str::to_string(txt, str::Encoding::UTF8, str::Encoding::UTF16LE);
        fprintf(stderr, "%s", text.c_str());
    }
#  endif
    void OnPrint(const LogMessage& msg) override
    {
        const auto lv = GetLogLevelStr(msg.Level);
        const auto src = msg.GetSource();
        const auto txt = msg.GetContent();
        std::u16string str;
        str.reserve(lv.size() + src.size() + txt.size() + 4);
        str.append(u"<").append(lv).append(7 - lv.size(), u' ').append(u">[").append(src).append(u"]"sv).append(txt);
        PrintText(str);
    }
#endif
};


class ConsoleBackend : public LoggerQBackend
{
private:
    const console::ConsoleEx& Console;
    std::string PrefixCacheU8;
    std::u16string PrefixCacheU16;
    bool UseFastU8Path;
    bool SupportColor;
    static constexpr CommonColor ToColor(const LogLevel lv) noexcept
    {
        switch (lv)
        {
        case LogLevel::Error:   return CommonColor::BrightRed;
        case LogLevel::Warning: return CommonColor::BrightYellow;
        case LogLevel::Success: return CommonColor::BrightGreen;
        case LogLevel::Info:    return CommonColor::BrightWhite;
        case LogLevel::Verbose: return CommonColor::BrightMagenta;
        case LogLevel::Debug:   return CommonColor::BrightCyan;
        default:                return CommonColor::White;
        }
    }
    bool virtual OnStart(std::any) noexcept override
    {
        common::SetThreadName(u"Console-MLogger-Backend");
        return true;
    }
public:
    ConsoleBackend() : Console(console::ConsoleEx::Get()),
        UseFastU8Path(Console.PreferEncoding() == str::Encoding::UTF8), SupportColor(Console.SupportColor())
    {
        if (UseFastU8Path)
        {
            PrefixCacheU8.reserve(64);
            PrefixCacheU8.push_back('[');
        }
        else
        {
            PrefixCacheU16.reserve(64);
            PrefixCacheU16.push_back(u'[');
        }
    }
    ~ConsoleBackend() override
    { }
    void virtual OnPrint(const LogMessage& msg) override
    {
        auto printer = Console.PrintSegments();
        if (SupportColor)
            printer.SetColor({ false, ToColor(msg.Level) });
        if (UseFastU8Path)
        {
            PrefixCacheU8.resize(1);
            PrefixCacheU8.append(msg.GetSource<false>());
            PrefixCacheU8.push_back(']');
            printer.Print(as_bytes(to_span(PrefixCacheU8)));
        }
        else
        {
            PrefixCacheU16.resize(1);
            PrefixCacheU16.append(msg.GetSource<true>());
            PrefixCacheU16.push_back(u']');
            printer.Print(PrefixCacheU16);
        }

        const auto txt = msg.GetContent();
        if (SupportColor)
        {
            uint32_t prevIdx = 0;
            for (const auto& [offset, color] : msg.GetSegments())
            {
                if (offset != prevIdx)
                    printer.Print(txt.substr(prevIdx, offset));
                printer.SetColor(color);
            }
            if (prevIdx < txt.length())
                printer.Print(txt.substr(prevIdx));
            printer.SetColor({ false });
            printer.SetColor({ true });
            printer.Print(span<const std::byte>{});
        }
        else
        {
            printer.Print(txt);
        }
    }
};


class FileBackend : public LoggerQBackend
{
protected:
    file::FileOutputStream Stream;
    bool virtual OnStart(std::any) noexcept override
    {
        common::SetThreadName(u"File-MLogger-Backend");
        return true;
    }
    void virtual OnStop() noexcept override
    {
        Stream.Flush();
    }
public:
    FileBackend(const fs::path& path) : 
        Stream(file::FileObject::OpenThrow(path, file::OpenFlag::Append))
    { } // using binary to bypass encoding
    ~FileBackend() override { }
    void virtual OnPrint(const LogMessage& msg) override
    {
        const auto lv  = GetLogLevelStr(msg.Level);
        const auto src = msg.GetSource();
        const auto txt = msg.GetContent();
        constexpr auto t0 = u"<"sv, t1 = u">["sv, t2 = u"]"sv;
        Stream.Write(sizeof(char16_t) * 1, t0.data());
        Stream.Write(sizeof(char16_t) * lv.size(), lv.data());
        Stream.Write(sizeof(char16_t) * 2, t1.data());
        Stream.Write(sizeof(char16_t) * src.size(), src.data());
        Stream.Write(sizeof(char16_t) * 1, t2.data());
        Stream.Write(sizeof(char16_t) * txt.size(), txt.data());
    }
};


std::shared_ptr<LoggerBackend> GetConsoleBackend()
{
    static std::shared_ptr<LoggerBackend> backend = LoggerQBackend::InitialQBackend<ConsoleBackend>();
    return backend;
}
void SyncConsoleBackend()
{
    const auto backend = std::dynamic_pointer_cast<ConsoleBackend>(GetConsoleBackend());
    const auto pms = backend->Synchronize();
    pms->WaitFinish();
}
std::shared_ptr<LoggerBackend> GetDebuggerBackend()
{
    static std::shared_ptr<LoggerBackend> backend = LoggerQBackend::InitialQBackend<DebuggerBackend>();
    return backend;
}

std::shared_ptr<LoggerBackend> GetFileBackend(const fs::path& path)
{
    try
    {
        std::shared_ptr<LoggerBackend> backend = LoggerQBackend::InitialQBackend<FileBackend>(path);
        return backend;
    }
    catch (...)
    {
        return nullptr;
    }
}


}
