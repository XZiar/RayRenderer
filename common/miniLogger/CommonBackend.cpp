#pragma once
#include "MiniLoggerRely.h"
#include "QueuedBackend.h"
#include "common/FileEx.hpp"
#include <filesystem>
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

namespace common::mlog
{

class ConsoleBackend : public LoggerQBackend
{
private:
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO csbInfo;
    LogLevel curlevel; //Single-thread executor, no need for thread-protection
    void changeState(const LogLevel lv)
    {
        constexpr WORD BLACK = 0;
        constexpr WORD BLUE = 1;
        constexpr WORD GREEN = 2;
        constexpr WORD CYAN = 3;
        constexpr WORD RED = 4;
        constexpr WORD MAGENTA = 5;
        constexpr WORD BROWN = 6;
        constexpr WORD LIGHTGRAY = 7;
        constexpr WORD DARKGRAY = 8;
        constexpr WORD LIGHTBLUE = 9;
        constexpr WORD LIGHTGREEN = 10;
        constexpr WORD LIGHTCYAN = 11;
        constexpr WORD LIGHTRED = 12;
        constexpr WORD LIGHTMAGENTA = 13;
        constexpr WORD YELLOW = 14;
        constexpr WORD WHITE = 15;
        if (curlevel != lv)
        {
            curlevel = lv;
            WORD attrb = BLACK;
            switch (lv)
            {
            case LogLevel::Error:
                attrb = LIGHTRED; break;
            case LogLevel::Warning:
                attrb = YELLOW; break;
            case LogLevel::Success:
                attrb = LIGHTGREEN; break;
            case LogLevel::Info:
                attrb = WHITE; break;
            case LogLevel::Verbose:
                attrb = LIGHTMAGENTA; break;
            case LogLevel::Debug:
                attrb = LIGHTCYAN; break;
            }
            SetConsoleTextAttribute(hConsole, attrb);
        }
    }
    void virtual OnStart() override 
    {
        common::SetThreadName(u"Console-MLogger-Backend");
    }
public:
    ConsoleBackend()
    {
        auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode;
        hConsole = GetConsoleMode(handle, &mode) ? handle : nullptr;
        curlevel = LogLevel::None;
    }
    ~ConsoleBackend() override 
    {
        if(hConsole)
            CloseHandle(hConsole);
    }
    void virtual OnPrint(const LogMessage& msg) override
    {
        if (hConsole == nullptr)
            return;
        changeState(msg.Level);
        auto& writer = detail::StrFormater<char16_t>::ToU16Str(u"[{}]{}", msg.Source, msg.GetContent());
        uint32_t outlen;
        WriteConsole(hConsole, writer.data(), (DWORD)writer.size(), (LPDWORD)&outlen, NULL);
    }
};


class DebuggerBackend : public LoggerQBackend
{
protected:
    void virtual OnStart() override
    {
        common::SetThreadName(u"Debugger-MLogger-Backend");
    }
public:
    void virtual OnPrint(const LogMessage& msg) override
    {
        auto& writer = detail::StrFormater<char16_t>::ToU16Str(u"{}[{}]{}", GetLogLevelStr(msg.Level), msg.Source, msg.GetContent());
        OutputDebugString((LPCWSTR)writer.c_str());
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
    }
public:
    FileBackend(const fs::path& path) : File(file::FileObject::OpenThrow(path, file::OpenFlag::APPEND | file::OpenFlag::CREATE | file::OpenFlag::TEXT)) { }
    ~FileBackend() override { }
    void virtual OnPrint(const LogMessage& msg) override
    {

        //File.Write();
    }
};


std::shared_ptr<LoggerBackend> __cdecl GetConsoleBackend()
{
    static std::shared_ptr<LoggerBackend> backend = LoggerQBackend::InitialQBackend<ConsoleBackend>();
    return backend;
}
std::shared_ptr<LoggerBackend> __cdecl GetDebuggerBackend()
{
    static std::shared_ptr<LoggerBackend> backend = LoggerQBackend::InitialQBackend<DebuggerBackend>();
    return backend;
}

std::shared_ptr<LoggerBackend> __cdecl GetFileBackend(const fs::path& path)
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