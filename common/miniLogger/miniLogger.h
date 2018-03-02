#pragma once

#ifdef MINILOG_EXPORT
#   define MINILOGAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define MINILOGAPI _declspec(dllimport)
#endif

#include <cstdint>
#include <cstdio>
#include <string>
#include <atomic>
#include <vector>
#include <functional>

#include "common/CommonMacro.hpp"
#include "common/CommonRely.hpp"

#include "fmt/format.h"


namespace common
{
namespace mlog
{


enum class LogLevel : uint8_t { Debug = 20, Verbose = 40, Info = 60, Sucess = 70, Warning = 85, Error = 100, None = 120 };
enum class LogOutput : uint8_t { None = 0x0, Console = 0x1, File = 0x2, Callback = 0x4, Buffer = 0x8, Debugger = 0x10 };
MAKE_ENUM_BITFIELD(LogOutput)

/*-return whether should continue logging*/
using LogCallBack = std::function<bool __cdecl(const LogLevel lv, const std::wstring& content)>;
/*global log callback*/
using GLogCallBack = std::function<void __cdecl(const LogLevel lv, const std::wstring& from, const std::wstring& content)>;
/*
Logging to Buffer is not thread-safe since it's now lack of memmory barrier
Changing logging file caould is not thread-safe since it's now lack of memmory barrier
I am tired of considering these situations cause I just wanna write a simple library for my own use
And I guess no one would see the words above, so I decide just to leave them alone
*/
class MINILOGAPI logger : public NonCopyable
{
public:
private:
	std::atomic_flag flagConsole = ATOMIC_FLAG_INIT, flagFile = ATOMIC_FLAG_INIT, flagCallback = ATOMIC_FLAG_INIT, flagBuffer = ATOMIC_FLAG_INIT;
	std::atomic<LogLevel> leastLV;
	std::atomic<LogOutput> outputs;
	bool ownFile = false;
	const std::wstring name, prefix;
	FILE *fp;
	std::vector<std::pair<LogLevel, std::wstring>> buffer;
	static GLogCallBack onGlobalLog;
	LogCallBack onLog;
	bool checkLevel(const LogLevel lv) { return (uint8_t)lv >= (uint8_t)leastLV.load(); }
    void printDebugger(const LogLevel lv, const std::wstring& content);
	void printConsole(const LogLevel lv, const std::wstring& content);
	void printFile(const LogLevel lv, const std::wstring& content);
	void printBuffer(const LogLevel lv, const std::wstring& content);
	bool onCallBack(const LogLevel lv, const std::wstring& content);
    void innerLog(const LogLevel lv, const std::wstring& content);
public:
	logger(const std::wstring loggername, const std::wstring logfile, LogCallBack cb = nullptr, const LogOutput lo = LogOutput::Console, const LogLevel lv = LogLevel::Info);
	logger(const std::wstring loggername, FILE * const logfile = nullptr, LogCallBack cb = nullptr, const LogOutput lo = LogOutput::Console, const LogLevel lv = LogLevel::Info);
	~logger();
	static void __cdecl setGlobalCallBack(GLogCallBack cb);
	void setLeastLevel(const LogLevel lv);
	void setOutput(const LogOutput method, const bool isEnable);
	std::vector<std::pair<LogLevel, std::wstring>> __cdecl getLogBuffer();
	template<class... Args>
	void error(const std::wstring& formater, Args&&... args)
	{
		log(LogLevel::Error, formater, std::forward<Args>(args)...);
	}
	template<class... Args>
	void warning(const std::wstring& formater, Args&&... args)
	{
		log(LogLevel::Warning, formater, std::forward<Args>(args)...);
	}
	template<class... Args>
	void success(const std::wstring& formater, Args&&... args)
	{
		log(LogLevel::Sucess, formater, std::forward<Args>(args)...);
	}
	template<class... Args>
	void verbose(const std::wstring& formater, Args&&... args)
	{
		log(LogLevel::Verbose, formater, std::forward<Args>(args)...);
	}
	template<class... Args>
	void info(const std::wstring& formater, Args&&... args)
	{
		log(LogLevel::Info, formater, std::forward<Args>(args)...);
	}
	template<class... Args>
	void debug(const std::wstring& formater, Args&&... args)
	{
		log(LogLevel::Debug, formater, std::forward<Args>(args)...);
	}
	template<class... Args>
	void log(const LogLevel lv, const std::wstring& formater, Args&&... args)
	{
		if (!checkLevel(lv))
			return;
		std::wstring logdat = fmt::format(formater, std::forward<Args>(args)...);
        if (onGlobalLog)
            onGlobalLog(lv, name, logdat);
        logdat = prefix + logdat;
        innerLog(lv, logdat);
	}
};


}
}

