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
#include <chrono>
#include <string>
#include <atomic>
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
    const uint64_t Timestamp;
    const std::u16string& Source;
    const std::u16string Content;
    std::atomic_uint32_t RefCount;
    const LogLevel Level;
    LogMessage(const std::u16string& prefix, const std::u16string& content, const LogLevel level, const uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count())
        : Source(prefix), Content(content), Level(level), Timestamp(time), RefCount(0)
    { }
    std::u16string_view GetContent() { return Content; }
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

namespace detail
{
template<typename Char>
struct StrFormater;
template<>
struct MINILOGAPI StrFormater<char>
{
    static fmt::BasicMemoryWriter<char>& GetWriter();
    template<class... Args>
    static std::u16string ToU16Str(const fmt::BasicCStringRef<char>& formater, Args&&... args)
    {
        auto& writer = GetWriter();
        writer.clear();
        writer.write(formater, std::forward<Args>(args)...);
        return std::u16string(writer.data(), writer.data() + writer.size());
    }
    static std::u16string ToU16Str(const fmt::BasicCStringRef<char>& content)
    {
        return std::u16string(content.c_str(), content.c_str() + std::char_traits<char>::length(content.c_str()));
    }
};
template<>
struct MINILOGAPI StrFormater<char16_t>
{
    static fmt::UTFMemoryWriter<char16_t>& GetWriter();
    template<class... Args>
    static std::u16string ToU16Str(const fmt::BasicCStringRef<char16_t>& formater, Args&&... args)
    {
        auto& writer = GetWriter();
        writer.clear();
        writer.write(formater, std::forward<Args>(args)...);
        return writer.c_str();
    }
    static std::u16string ToU16Str(const fmt::BasicCStringRef<char16_t>& content)
    {
        return std::u16string(content.c_str());
    }
};

template<>
struct MINILOGAPI StrFormater<char32_t>
{
    static fmt::UTFMemoryWriter<char32_t>& GetWriter();
    template<class... Args>
    static std::u16string ToU16Str(const fmt::BasicCStringRef<char32_t>& formater, Args&&... args)
    {
        auto& writer = GetWriter();
        writer.clear();
        writer.write(formater, std::forward<Args>(args)...);
        return common::str::detail::CharsetConvertor<common::str::detail::UTF32, common::str::detail::UTF16, char32_t, char16_t>::Convert(writer.data(), writer.size(), true, true);
    }
    static std::u16string ToU16Str(const fmt::BasicCStringRef<char32_t>& content)
    {
        std::u32string_view sv(content.c_str());
        return common::str::detail::CharsetConvertor<common::str::detail::UTF32, common::str::detail::UTF16, char32_t, char16_t>::Convert(sv.data(), sv.size(), true, true);
    }
};

template<>
struct MINILOGAPI StrFormater<wchar_t>
{
    static fmt::BasicMemoryWriter<wchar_t>& GetWriter();
    template<class... Args>
    static std::u16string ToU16Str(const fmt::BasicCStringRef<wchar_t>& formater, Args&&... args)
    {
        auto& writer = GetWriter();
        writer.clear();
        writer.write(formater, std::forward<Args>(args)...);
        if constexpr(sizeof(wchar_t) == sizeof(char16_t))
            return std::u16string((const char16_t*)writer.data(), writer.size());
        else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
            return common::str::detail::CharsetConvertor<common::str::detail::UTF32, common::str::detail::UTF16, wchar_t, char16_t>::Convert(writer.data(), writer.size(), true, true);
        else
            return u"";
    }
    static std::u16string ToU16Str(const fmt::BasicCStringRef<wchar_t>& content)
    {
        if constexpr(sizeof(wchar_t) == sizeof(char16_t))
            return StrFormater<char16_t>::ToU16Str(*(const fmt::BasicCStringRef<char16_t>*)&content);
        else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
            return StrFormater<char32_t>::ToU16Str(*(const fmt::BasicCStringRef<char32_t>*)&content);
        else
            return u"";
    }
};
}


}
}