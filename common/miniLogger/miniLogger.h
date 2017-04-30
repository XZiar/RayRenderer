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

#include "../CommonRely.hpp"
#include "../Wrapper.hpp"

#define FMT_HEADER_ONLY
#include "../../3rdParty/fmt/format.h"


namespace common
{
namespace mlog
{


enum class LogLevel : uint8_t { Debug = 20, Verbose = 40, Info = 60, Sucess = 70, Warning = 85, Error = 100, None = 120 };
enum class LogOutput : uint8_t { Console = 0x1, File = 0x2, Callback = 0x4, Buffer = 0x8 };

/*-return whether should continue logging*/
using LogCallBack = std::function<bool __cdecl(LogLevel lv, std::wstring content)>;
/*global log callback*/
using GLogCallBack = std::function<void __cdecl(LogLevel lv, const std::wstring& from, const std::wstring& content)>;
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
	std::atomic_uint_least8_t leastLV;
	std::atomic_uint_least8_t outputs;
	bool ownFile = false;
	const std::wstring name, prefix;
	FILE *fp;
	std::vector<std::pair<LogLevel, std::wstring>> buffer;
	static GLogCallBack onGlobalLog;
	LogCallBack onLog;
	bool checkLevel(const LogLevel lv);
	void printConsole(const LogLevel lv, const std::wstring& content);
	void printFile(const LogLevel lv, const std::wstring& content);
	void printBuffer(const LogLevel lv, const std::wstring& content);
	bool onCallBack(const LogLevel lv, const std::wstring& content);
public:
	logger(const std::wstring loggername, const std::wstring logfile, LogCallBack cb = nullptr, const LogOutput lo = LogOutput::Console, const LogLevel lv = LogLevel::Info);
	logger(const std::wstring loggername, FILE * const logfile = nullptr, LogCallBack cb = nullptr, const LogOutput lo = LogOutput::Console, const LogLevel lv = LogLevel::Info);
	~logger();
	static void __cdecl setGlobalCallBack(GLogCallBack cb);
	void setLeastLevel(const LogLevel lv);
	void setOutput(const LogOutput method, const bool isEnable);
	std::vector<std::pair<LogLevel, std::wstring>> __cdecl getLogBuffer();
	template<class... ARGS>
	void error(std::wstring formater, const ARGS&... args)
	{
		log(LogLevel::Error, formater, args...);
	}
	template<class... ARGS>
	void warning(std::wstring formater, const ARGS&... args)
	{
		log(LogLevel::Warning, formater, args...);
	}
	template<class... ARGS>
	void success(std::wstring formater, const ARGS&... args)
	{
		log(LogLevel::Sucess, formater, args...);
	}
	template<class... ARGS>
	void verbose(std::wstring formater, const ARGS&... args)
	{
		log(LogLevel::Verbose, formater, args...);
	}
	template<class... ARGS>
	void info(std::wstring formater, const ARGS&... args)
	{
		log(LogLevel::Info, formater, args...);
	}
	template<class... ARGS>
	void debug(std::wstring formater, const ARGS&... args)
	{
		log(LogLevel::Debug, formater, args...);
	}
	template<class... ARGS>
	void log(const LogLevel lv, std::wstring formater, const ARGS&... args)
	{
		if (!checkLevel(lv))
			return;
		const std::wstring logdat = fmt::format(formater, args...);
		if (onGlobalLog)
			onGlobalLog(lv, name, logdat);
		const std::wstring logstr = prefix + logdat;
		if (!onCallBack(lv, logstr))
			return;
		printBuffer(lv, logstr);
		printConsole(lv, logstr);
		printFile(lv, logstr);
	}
};


}
}

