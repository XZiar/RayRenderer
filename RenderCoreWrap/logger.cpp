#include "RenderCoreRely.h"
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
	Sucess = (uint8_t)common::mlog::LogLevel::Sucess,
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

void __cdecl LogCallback(common::mlog::LogLevel lv_, const std::wstring& from_, const std::wstring& content_)
{
	LogLevel lv = (LogLevel)lv_;
	String^ from = gcnew String(from_.c_str());
	String^ content = gcnew String(content_.c_str());
	Logger::RaiseOnLog(lv, from, content);
}

#pragma unmanaged
void setLogCB()
{
	common::mlog::logger::setGlobalCallBack([](common::mlog::LogLevel lv, const std::wstring& from, const std::wstring& content) 
	{LogCallback(lv, from, content); });
}
void unsetLogCB()
{
	common::mlog::logger::setGlobalCallBack(nullptr);
}
#pragma managed

}
