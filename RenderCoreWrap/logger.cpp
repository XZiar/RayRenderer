#include "logger.h"
#include <vcclr.h>

using namespace System::Diagnostics;
using namespace common::mlog;

void __cdecl onLog(LogLevel lv, const std::wstring& from, const std::wstring& content)
{
	static gcroot<ManagedLogger^> logger = gcnew ManagedLogger();
	String^ prefix = gcnew String(from.c_str());
	String^ cont = gcnew String(content.c_str());
	logger->log(lv, prefix, cont);
}

void setLogger()
{
	//common::mlog::logger::setGlobalCallBack(onLog);
}

void ManagedLogger::log(LogLevel lv, String^ from, String^ content)
{
	auto txt = String::Format("[{0}]{1}", from, content);
	switch (lv)
	{
	case LogLevel::Debug:
		Debug::Write(txt, "information"); break;
	case LogLevel::Warning:
		Debug::Write(txt, "warning"); break;
	case LogLevel::Error:
		Debug::Write(txt, "error"); break;
	default:
		Debug::Write(txt); break;
	}
}
