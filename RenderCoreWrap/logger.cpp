#include "RenderCoreWrapRely.h"
#include <vcclr.h>

using namespace System;
using namespace common::mlog;


namespace common
{

public enum class LogLevel : uint8_t
{
    Debug =     (uint8_t)common::mlog::LogLevel::Debug,
    Verbose =   (uint8_t)common::mlog::LogLevel::Verbose,
    Info =      (uint8_t)common::mlog::LogLevel::Info,
    Success =   (uint8_t)common::mlog::LogLevel::Success,
    Warning =   (uint8_t)common::mlog::LogLevel::Warning,
    Error =     (uint8_t)common::mlog::LogLevel::Error,
    None =      (uint8_t)common::mlog::LogLevel::None,
};

#pragma unmanaged
static uint32_t setLogCB();
static void unsetLogCB(const uint32_t);
#pragma managed

public ref class Logger
{
public:
    delegate void LogEventHandler(LogLevel level, String^ from, String^ content);
private:
    static Logger^ TheLogger;
    static LogEventHandler^ onLog;
    initonly uint32_t ID;
    Logger()
    {
        ID = setLogCB();
    }
    ~Logger() { this->!Logger(); }
    !Logger() { unsetLogCB(ID); }
internal:
    static void RaiseOnLog(LogLevel level, String^ from, String^ content)
    {
        OnLog(level, from, content);
    }
public:
    static event LogEventHandler^ OnLog
    {
        void add(LogEventHandler^ handler)
        {
            onLog += handler;
        }
        void remove(LogEventHandler^ handler)
        {
            onLog -= handler;
        }
        void raise(LogLevel level, String^ from, String^ content)
        {
            auto handler = onLog;
            if (handler == nullptr)
                return;
            handler->Invoke(level, from, content);
        }
    }
    static Logger()
    {
        TheLogger = gcnew Logger();
    }
};


static void __cdecl LogCallback(const common::mlog::LogMessage& msg)
{
    Logger::RaiseOnLog((LogLevel)msg.Level, ToStr(msg.GetSource()), ToStr(msg.GetContent()));
}

#pragma unmanaged
static uint32_t setLogCB()
{
    return common::mlog::AddGlobalCallback(LogCallback);
}
static void unsetLogCB(const uint32_t id)
{
    common::mlog::DelGlobalCallback(id);
}
#pragma managed

}
