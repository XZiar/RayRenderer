#pragma once
#include "MiniLoggerRely.h"
#include "QueuedBackend.h"
#include <memory>
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

namespace common::mlog
{

class ConsoleBackend : public LoggerQBackend
{
private:
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO csbInfo;
    LogLevel curlevel; //SpinLock ensures no thread-race 
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
            case LogLevel::Sucess:
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
    void virtual OnPrint(const LogMessage& msg) override
    {
        if (hConsole == nullptr)
            return;
        changeState(msg.Level);
        uint32_t outlen;
        WriteConsole(hConsole, msg.Content.c_str(), (DWORD)msg.Content.length(), (LPDWORD)&outlen, NULL);
    }
};

class DebuggerLogger : public LoggerQBackend
{
protected:
    void virtual OnStart() override
    {
        common::SetThreadName(u"Debugger-MLogger-Backend");
    }
public:
    void virtual OnPrint(const LogMessage& msg) override
    {
        OutputDebugString((LPCWSTR)msg.Content.c_str());
    }
};

std::shared_ptr<LoggerBackend> GetConsoleBackend()
{
    static auto backend = LoggerQBackend::InitialQBackend<ConsoleBackend>();
    return std::dynamic_pointer_cast<LoggerBackend>(backend);
}
std::shared_ptr<LoggerBackend> GetDebuggerBackend()
{
    static auto backend = LoggerQBackend::InitialQBackend<DebuggerLogger>();
    return std::dynamic_pointer_cast<LoggerBackend>(backend);
}


}