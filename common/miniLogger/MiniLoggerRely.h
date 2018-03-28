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
#include <set>



namespace common
{
namespace mlog
{
namespace fs = std::experimental::filesystem;

namespace detail
{
class MiniLoggerBase;
}

enum class LogLevel : uint8_t { Debug = 20, Verbose = 40, Info = 60, Success = 70, Warning = 85, Error = 100, None = 120 };
MINILOGAPI const char16_t* __cdecl GetLogLevelStr(const LogLevel level);

struct MINILOGAPI LogMessage : public NonCopyable
{
    friend class detail::MiniLoggerBase;
public:
    const uint64_t Timestamp;
    const std::u16string& Source;
private:
    std::atomic_uint32_t RefCount;
    const uint32_t Length;
public:
    const LogLevel Level;
private:
    LogMessage(const std::u16string& prefix, const uint32_t length, const LogLevel level, const uint64_t time)
        : Source(prefix), Length(length), Level(level), Timestamp(time), RefCount(1) //RefCount is at first 1
    { }
public:
    static LogMessage* MakeMessage(const std::u16string& prefix, const char16_t *content, const size_t len, const LogLevel level, const uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count())
    {
        if (len >= UINT32_MAX)
            COMMON_THROW(BaseException, L"Too long for a single LogMessage!");
        uint8_t* ptr = (uint8_t*)malloc_align(sizeof(LogMessage) + sizeof(char16_t)*len, 64);
        if (!ptr)
            return nullptr; //not throw an exception yet
        LogMessage* msg = new (ptr)LogMessage(prefix, static_cast<uint32_t>(len), level, time);
        memcpy_s(ptr + sizeof(LogMessage), sizeof(char16_t)*len, content, sizeof(char16_t)*len);
        return msg;
    }
    static LogMessage* MakeMessage(const std::u16string& prefix, const std::u16string& content, const LogLevel level, const uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count())
    {
        return MakeMessage(prefix, content.c_str(), content.size(), level, time);
    }
    static LogMessage* MakeMessage(const std::u16string& prefix, const fmt::BasicCStringRef<char16_t>& content, const LogLevel level, const uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count())
    {
        return MakeMessage(prefix, content.c_str(), std::char_traits<char16_t>::length(content.c_str()), level, time);
    }
    static LogMessage* MakeMessage(const std::u16string& prefix, const fmt::UTFMemoryWriter<char16_t>& writer, const LogLevel level, const uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count())
    {
        return MakeMessage(prefix, writer.data(), writer.size(), level, time);
    }
    static bool Consume(LogMessage* msg)
    {
        if (msg->RefCount-- == 1) //last one
        {
            free_align(msg);
            return true;
        }
        return false;
    }
    std::u16string_view GetContent() const { return std::u16string_view(reinterpret_cast<const char16_t*>(reinterpret_cast<const uint8_t*>(this) + sizeof(LogMessage)), Length); }
};

class MINILOGAPI LoggerBackend : public NonCopyable
{
protected:
    volatile LogLevel LeastLevel = LogLevel::Debug; //non-atomic, should increase performance
    void virtual OnPrint(const LogMessage& msg) = 0;
public:
    virtual ~LoggerBackend() { }
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
    static const fmt::UTFMemoryWriter<char16_t>& ToU16Str(const fmt::BasicCStringRef<char16_t>& formater, Args&&... args)
    {
        auto& writer = GetWriter();
        writer.clear();
        writer.write(formater, std::forward<Args>(args)...);
        return writer;
    }
    static const fmt::BasicCStringRef<char16_t>& ToU16Str(const fmt::BasicCStringRef<char16_t>& content)
    {
        return content;
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
            return std::u16string(reinterpret_cast<const fmt::BasicCStringRef<char16_t>*>(&content)->c_str());
        else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
            return StrFormater<char32_t>::ToU16Str(*(const fmt::BasicCStringRef<char32_t>*)&content);
        else
            return u"";
    }
};
}


}
}