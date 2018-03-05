#pragma once

#ifdef MINILOG_EXPORT
#   define MINILOGAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define MINILOGAPI _declspec(dllimport)
#endif


#include "common/CommonMacro.hpp"
#include "common/CommonRely.hpp"
#include "common/SpinLock.hpp"
#include "common/StrCharset.hpp"
#include "fmt/format.h"
#include "fmt/utfext.h"
#include <cstdint>
#include <cstdio>
#include <chrono>
#include <string>
#include <atomic>
#include <vector>
#include <array>
#include <set>
#include <filesystem>
#include <functional>
#include <memory>



namespace common
{
namespace mlog
{
namespace fs = std::experimental::filesystem;


enum class LogLevel : uint8_t { Debug = 20, Verbose = 40, Info = 60, Success = 70, Warning = 85, Error = 100, None = 120 };
MINILOGAPI const char16_t* __cdecl GetLogLevelStr(const LogLevel level);

struct MINILOGAPI LogMessage : public NonCopyable
{
    const std::u16string& Source;
    const std::u16string Content;
    const uint64_t Timestamp;
    std::atomic_uint32_t RefCount;
    const LogLevel Level;
    LogMessage(const std::u16string& prefix, const std::u16string& content, const LogLevel level, const uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count())
        : Source(prefix), Content(content), Level(level), Timestamp(time), RefCount(0)
    { }
    static bool Consume(LogMessage* msg)
    {
        if (msg->RefCount-- == 1) //last one
        {
            delete msg;
            return true;
        }
        return false;
    }
};

class MINILOGAPI LoggerBackend : public NonCopyable
{
protected:
    volatile LogLevel LeastLevel = LogLevel::Debug; //non-atomic, should increase performance
    void virtual OnPrint(const LogMessage& msg) = 0;
public:
    void virtual Print(LogMessage* msg) 
    {
        if ((uint8_t)msg->Level >= (uint8_t)LeastLevel)
            OnPrint(*msg);
        LogMessage::Consume(msg);
    }
    void SetLeastLevel(const LogLevel level) { LeastLevel = level; }
    LogLevel GetLeastLevel() { return LeastLevel; }
};

using MLoggerCallback = std::function<void __cdecl(const LogMessage& msg)>;

MINILOGAPI std::shared_ptr<LoggerBackend> __cdecl GetConsoleBackend();
MINILOGAPI std::shared_ptr<LoggerBackend> __cdecl GetDebuggerBackend();
MINILOGAPI std::shared_ptr<LoggerBackend> __cdecl GetFileBackend(const fs::path& path);

}
}