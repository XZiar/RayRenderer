#include "MiniLoggerRely.h"
#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include <Windows.h>
#endif
#include "QueuedBackend.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/ThreadEx.h"
#include "SystemCommon/ColorConsole.h"


namespace common::mlog
{

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
        const auto text = strchset::to_u8string(txt, str::Charset::UTF16LE);
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
    static constexpr console::ConsoleColor ToColor(const LogLevel lv)
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
    pms->Wait();
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