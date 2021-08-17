#include "SystemCommonPch.h"
#include "MiniLogger.h"
#include "MiniLoggerBackend.h"
#include "FileEx.h"
#include "ThreadEx.h"
#include "ColorConsole.h"
#include <thread>
#include <array>


namespace common::mlog
{

using namespace std::chrono;

struct TimeConv 
{ 
    system_clock::time_point SysClock;
    uint64_t Base;
    TimeConv() : SysClock(system_clock::now()),
        Base(static_cast<uint64_t>(high_resolution_clock::now().time_since_epoch().count())) {}
};


LogMessage* LogMessage::MakeMessage(const SharedString<char16_t>& prefix, const char16_t* content, const size_t len, const LogLevel level, const uint64_t time)
{
    if (len >= UINT32_MAX)
        COMMON_THROW(BaseException, u"Too long for a single LogMessage!");
    uint8_t* ptr = (uint8_t*)malloc(sizeof(LogMessage) + sizeof(char16_t) * len);
    Ensures(reinterpret_cast<uintptr_t>(ptr) % 4 == 0);
    if (!ptr)
        return nullptr; //not throw an exception yet
    LogMessage* msg = new (ptr)LogMessage(prefix, static_cast<uint32_t>(len), level, time);
    memcpy_s(ptr + sizeof(LogMessage), sizeof(char16_t) * len, content, sizeof(char16_t) * len);
    return msg;
}

bool LogMessage::Consume(LogMessage* msg)
{
    if (msg->RefCount-- == 1) //last one
    {
        msg->~LogMessage();
        free(msg);
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

namespace detail
{
template<typename Char>
std::vector<Char>& StrFormater::GetBuffer()
{
    static thread_local std::vector<Char> out;
    out.resize(0);
    return out;
}
template std::vector<char>&     StrFormater::GetBuffer();
template std::vector<wchar_t>&  StrFormater::GetBuffer();
template std::vector<char16_t>& StrFormater::GetBuffer();
template std::vector<char32_t>& StrFormater::GetBuffer();
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
    static void PrintText(const std::u16string_view& txt)
    {
    #if defined(_WIN32)
        OutputDebugString((LPCWSTR)txt.data());
    #else
        const auto text = str::to_u8string(txt, str::Charset::UTF16LE);
        fprintf(stderr, "%s", text.c_str());
    #endif
    }
    void virtual OnPrint(const LogMessage& msg) override
    {
        auto& buffer = detail::StrFormater::ToU16Str(FMT_STRING(u"<{:6}>[{}]{}"), GetLogLevelStr(msg.Level), msg.GetSource(), msg.GetContent());
        buffer.push_back(u'\0');
        PrintText(std::u16string_view(buffer.data(), buffer.size()));
    }
};


class ConsoleBackend : public LoggerQBackend
{
private:
    std::unique_ptr<const console::ConsoleHelper> Helper;
    static constexpr console::ConsoleColor ToColor(const LogLevel lv) noexcept
    {
        using console::ConsoleColor;
        switch (lv)
        {
        case LogLevel::Error:   return ConsoleColor::BrightRed;
        case LogLevel::Warning: return ConsoleColor::BrightYellow;
        case LogLevel::Success: return ConsoleColor::BrightGreen;
        case LogLevel::Info:    return ConsoleColor::BrightWhite;
        case LogLevel::Verbose: return ConsoleColor::BrightMagenta;
        case LogLevel::Debug:   return ConsoleColor::BrightCyan;
        default:                return ConsoleColor::White;
        }
    }
    bool virtual OnStart(std::any) noexcept override
    {
        common::SetThreadName(u"Console-MLogger-Backend");
        return true;
    }
public:
    ConsoleBackend()
    {
        try
        {
            Helper = std::make_unique<const console::ConsoleHelper>();
        }
        catch (const std::runtime_error&)
        {
            DebuggerBackend::PrintText(u"Cannot initilzie the Console\n");
        }
    }
    ~ConsoleBackend() override
    { }
    void virtual OnPrint(const LogMessage& msg) override
    {
        if (!Helper)
            return;
        const auto& color = console::ConsoleHelper::GetColorStr(ToColor(msg.Level));

        const auto& buffer = detail::StrFormater::ToU16Str(FMT_STRING(u"{}[{}]{}\x1b[39m"), color, msg.GetSource(), msg.GetContent());
        Helper->Print(std::u16string_view(buffer.data(), buffer.size()));
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
        auto& buffer = detail::StrFormater::ToU16Str(FMT_STRING(u"<{:6}>[{}]{}"), GetLogLevelStr(msg.Level), msg.GetSource(), msg.GetContent());
        Stream.Write(buffer.size() * sizeof(char16_t), buffer.data());
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
