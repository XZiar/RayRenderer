#include "MiniLoggerRely.h"
#include "miniLogger.h"
#include "InnerLogger.inl"

namespace common::mlog
{
using common::SpinLocker;

GLogCallBack logger::onGlobalLog = nullptr;
void logger::setGlobalCallBack(GLogCallBack cb)
{
	try
	{
		onGlobalLog = cb;
	}
	catch (std::exception& e)
	{
		printf("%s\n", e.what());
		getchar();
	}
}

logger::logger(const std::wstring loggername, const std::wstring logfile, LogCallBack cb, const LogOutput lo, const LogLevel lv)
	: name(loggername), prefix(fmt::format(L"[{}]", loggername)), leastLV(lv), outputs(lo), onLog(cb)
{
	const auto ret = _wfopen_s(&fp, logfile.c_str(), L"wt");
	ownFile = (ret == 0);
}

logger::logger(const std::wstring loggername, FILE * const logfile, LogCallBack cb, const LogOutput lo, const LogLevel lv) 
	: name(loggername), prefix(fmt::format(L"[{}]", loggername)), leastLV(lv), outputs(lo), fp(logfile), ownFile(false), onLog(cb)
{
}

logger::~logger()
{
	if (ownFile)
		fclose(fp);
}


void logger::printDebugger(const LogLevel lv, const std::wstring& content)
{
    static DebuggerLogger dbglog;
    //SpinLocker locker(flagConsole);
    dbglog.print(lv, content);
}

void logger::printConsole(const LogLevel lv, const std::wstring& content)
{
	static ConsoleLogger conlog;
	//SpinLocker locker(flagConsole);
	conlog.print(lv, content);
}

void logger::printFile(const LogLevel lv, const std::wstring& content)
{
	if (fp == nullptr)
		return;
	SpinLocker locker(flagFile);
	fwprintf_s(fp, L"%s", content.c_str());
}

void logger::printBuffer(const LogLevel lv, const std::wstring& content)
{
	SpinLocker locker(flagBuffer);
	buffer.push_back({ lv,content });
}

bool logger::onCallBack(const LogLevel lv, const std::wstring& content)
{
	if (!onLog)
		return true;
	{
		SpinLocker locker(flagCallback);
		const bool ret = onLog(lv, content);
		return ret;
	}
}

void logger::innerLog(const LogLevel lv, const std::wstring& content)
{
    if (!onCallBack(lv, content))
        return;
    const auto outputBits = outputs.load();
    if (HAS_FIELD(outputs, LogOutput::Buffer))
        printBuffer(lv, content);
    if (HAS_FIELD(outputs, LogOutput::Console))
        printConsole(lv, content);
    if (HAS_FIELD(outputs, LogOutput::Debugger))
        printDebugger(lv, content);
    if (HAS_FIELD(outputs, LogOutput::File))
        printFile(lv, content);
}



void logger::setLeastLevel(const LogLevel lv)
{
	leastLV.exchange(lv);
}

void logger::setOutput(const LogOutput method, const bool isEnable)
{
	const bool curState = (outputs & LogOutput::Buffer) != LogOutput::None;
	if (curState == isEnable)
		return;
	std::atomic_flag *flag;
	switch (method)
	{
	case LogOutput::Console:
		flag = &flagConsole; break;
	case LogOutput::File:
		flag = &flagFile; break;
	case LogOutput::Buffer:
		flag = &flagBuffer; break;
	case LogOutput::Callback:
		flag = &flagCallback; break;
	default:
		return;
	}
	{
		SpinLocker locker(*flag);
		if (isEnable)
			std::atomic_fetch_and((std::atomic_uint8_t*)&outputs, (uint8_t)method);
		else
			std::atomic_fetch_and((std::atomic_uint8_t*)&outputs, (uint8_t)~method);
	}
}

std::vector<std::pair<LogLevel, std::wstring>> logger::getLogBuffer()
{
	std::vector<std::pair<LogLevel, std::wstring>> ret;
	{
		SpinLocker locker(flagBuffer);
		ret.swap(buffer);
	}
	return ret;
}


namespace detail
{
MLoggerCallback MiniLoggerGBase::OnLog = nullptr;
void MiniLoggerGBase::SetGlobalCallback(const MLoggerCallback& cb)
{
    OnLog = cb;
}

fmt::BasicMemoryWriter<char>& StrFormater<char>::GetWriter()
{
    static thread_local fmt::BasicMemoryWriter<char> out;
    return out;
}
fmt::UTFMemoryWriter<char16_t>& StrFormater<char16_t>::GetWriter()
{
    static thread_local fmt::UTFMemoryWriter<char16_t> out;
    return out;
}

fmt::UTFMemoryWriter<char32_t>& StrFormater<char32_t>::GetWriter()
{
    static thread_local fmt::UTFMemoryWriter<char32_t> out;
    return out;
}

fmt::BasicMemoryWriter<wchar_t>& StrFormater<wchar_t>::GetWriter()
{
    static thread_local fmt::BasicMemoryWriter<wchar_t> out;
    return out;
}


}


}
