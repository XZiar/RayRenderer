#include "RenderCoreWrapRely.h"
#include <vcclr.h>

using namespace System;
using namespace common::mlog;
using common::container::FindInMap;

namespace common
{

public enum class LogLevel : uint8_t
{
    Debug   = (uint8_t)common::mlog::LogLevel::Debug,
    Verbose = (uint8_t)common::mlog::LogLevel::Verbose,
    Info    = (uint8_t)common::mlog::LogLevel::Info,
    Success = (uint8_t)common::mlog::LogLevel::Success,
    Warning = (uint8_t)common::mlog::LogLevel::Warning,
    Error   = (uint8_t)common::mlog::LogLevel::Error,
    None    = (uint8_t)common::mlog::LogLevel::None,
};

#pragma unmanaged
static common::CallbackToken setLogCB();
static void unsetLogCB(const common::CallbackToken*);
struct SVComparator
{
    constexpr bool operator()(const std::u16string_view& left, const std::u16string_view& right) const noexcept
    {
        return left.data() == right.data() ? left.size() < right.size() : left.data() < right.data();
    }
};
static std::map<std::u16string_view, gcroot<String^>, SVComparator> STR_CACHE;
#pragma managed


public ref class Logger
{
public:
    delegate void LogEventHandler(LogLevel level, String^ from, String^ content);
private:
    static Logger^ TheLogger;
    NativeWrapper<common::CallbackToken> ID;
    Logger()
    {
        ID.Construct(setLogCB());
    }
    ~Logger() { this->!Logger(); }
    !Logger() 
    { 
        WRAPPER_NATIVE_PTR(ID, pID);
        unsetLogCB(pID);
    }
internal:
    static void RaiseOnLog(LogLevel level, String^ from, String^ content)
    {
        OnLog(level, from, content);
    }
public:
    static event LogEventHandler^ OnLog;
    static Logger()
    {
        TheLogger = gcnew Logger();
    }
};

//promise thread-safety since in other thread
static void __cdecl LogCallback(const common::mlog::LogMessage& msg)
{
    const auto src = msg.GetSource();
    if (const auto msrc = STR_CACHE.find(src); msrc != STR_CACHE.cend())
        Logger::RaiseOnLog((LogLevel)msg.Level, msrc->second, ToStr(msg.GetContent()));
    else
        Logger::RaiseOnLog((LogLevel)msg.Level, STR_CACHE.emplace(src, gcroot<String^>(ToStr(src))).first->second, ToStr(msg.GetContent()));
}

#pragma unmanaged
static common::CallbackToken setLogCB()
{
    //common::mlog::GetConsoleBackend()->SetLeastLevel(common::mlog::LogLevel::Error);
    return common::mlog::AddGlobalCallback(LogCallback);
}
static void unsetLogCB(const common::CallbackToken* token)
{
    common::mlog::DelGlobalCallback(*token);
}
#pragma managed

}
