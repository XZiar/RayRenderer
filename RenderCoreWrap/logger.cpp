#include "RenderCoreWrapRely.h"
#include <vcclr.h>

using namespace System;
using namespace common::mlog;


namespace common
{

public enum class LogLevel : uint8_t
{
	Debug = (uint8_t)common::mlog::LogLevel::Debug,
	Verbose = (uint8_t)common::mlog::LogLevel::Verbose,
	Info = (uint8_t)common::mlog::LogLevel::Info,
	Success = (uint8_t)common::mlog::LogLevel::Success,
	Warning = (uint8_t)common::mlog::LogLevel::Warning,
	Error = (uint8_t)common::mlog::LogLevel::Error,
	None = (uint8_t)common::mlog::LogLevel::None,
};

#pragma unmanaged
void setLogCB();
void unsetLogCB();
#pragma managed

public ref class Logger
{
public:
	delegate void LogEventHandler(LogLevel lv, String^ from, String^ content);
private:
	static Logger^ thelogger;
	Logger()
	{
		setLogCB();
	}
	~Logger() { this->!Logger(); }
	!Logger() { unsetLogCB(); }
internal:
	static void RaiseOnLog(LogLevel lv, String^ from, String^ content)
	{
		OnLog(lv, from, content);
	}
public:
	static event LogEventHandler^ OnLog;
	static Logger()
	{
		thelogger = gcnew Logger();
	}
};


void __cdecl LogCallback(const common::mlog::LogMessage& msg)
{
    LogLevel lv = (LogLevel)msg.Level;
    String^ from = gcnew String(reinterpret_cast<const wchar_t*>(msg.Source.c_str()), 0, (int)msg.Source.size());
    String^ content = gcnew String(reinterpret_cast<const wchar_t*>(msg.Content.c_str()), 0, (int)msg.Content.size());
    Logger::RaiseOnLog(lv, from, content);
}

#pragma unmanaged
void setLogCB()
{
    common::mlog::detail::MiniLoggerBase::SetGlobalCallback(LogCallback);
}
void unsetLogCB()
{
    common::mlog::detail::MiniLoggerBase::SetGlobalCallback(nullptr);
}
#pragma managed

}
