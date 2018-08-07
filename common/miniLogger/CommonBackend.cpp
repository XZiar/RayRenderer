#include "MiniLoggerRely.h"
#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include <Windows.h>
#endif
#include "QueuedBackend.h"
#include "common/FileEx.hpp"
#include "common/ThreadEx.h"
#include "common/ColorConsole.h"


namespace common::mlog
{

class DebuggerBackend : public LoggerQBackend
{
protected:
    void virtual OnStart() override
    {
        common::SetThreadName(u"Debugger-MLogger-Backend");
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
        auto& writer = detail::StrFormater<char16_t>::ToU16Str(u"{}[{}]{}", GetLogLevelStr(msg.Level), msg.GetSource(), msg.GetContent());
        writer.push_back(u'\0');
        PrintText(std::u16string_view(writer.data(), writer.size()));
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
    void virtual OnStart() override
    {
        common::SetThreadName(u"Console-MLogger-Backend");
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
        auto& writer = detail::StrFormater<char16_t>::ToU16Str(u"{}[{}]{}\x1b[39m", color, msg.GetSource(), msg.GetContent());
        //Helper->Print(ToColor(msg.Level), std::u16string_view(writer.data(), writer.size()));
        Helper->Print(std::u16string_view(writer.data(), writer.size()));
    }
};


class FileBackend : public LoggerQBackend
{
protected:
    file::FileObject File;
    void virtual OnStart() override
    {
        common::SetThreadName(u"File-MLogger-Backend");
    }
    void virtual OnStop() override
    {
        File.Flush();
    }
public:
    FileBackend(const fs::path& path) : File(file::FileObject::OpenThrow(path, file::OpenFlag::APPEND | file::OpenFlag::CREATE | file::OpenFlag::BINARY)) 
    { } // using binary to bypass encoding
    ~FileBackend() override { }
    void virtual OnPrint(const LogMessage& msg) override
    {
        auto& writer = detail::StrFormater<char16_t>::ToU16Str(u"{}[{}]{}", GetLogLevelStr(msg.Level), msg.GetSource(), msg.GetContent());
        File.Write(writer.size() * sizeof(char16_t), writer.data());
    }
};


std::shared_ptr<LoggerBackend> GetConsoleBackend()
{
    static std::shared_ptr<LoggerBackend> backend = LoggerQBackend::InitialQBackend<ConsoleBackend>();
    return backend;
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