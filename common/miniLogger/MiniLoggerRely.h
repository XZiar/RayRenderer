#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef MINILOG_EXPORT
#   define MINILOGAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define MINILOGAPI _declspec(dllimport)
# endif
#else
# define MINILOGAPI
#endif


#include "common/CommonRely.hpp"
#include "common/SpinLock.hpp"
#include "common/StrCharset.hpp"
#include "common/SharedString.hpp"
#include "3rdParty/fmt/format.h"
#include "3rdParty/fmt/utfext.h"
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

namespace detail
{
class MiniLoggerBase;
}

enum class LogLevel : uint8_t { Debug = 20, Verbose = 40, Info = 60, Success = 70, Warning = 85, Error = 100, None = 120 };
MINILOGAPI const char16_t* GetLogLevelStr(const LogLevel level);

struct MINILOGAPI LogMessage : public NonCopyable, public NonMovable
{
    friend class detail::MiniLoggerBase;
public:
    const uint64_t Timestamp;
private:
    const SharedString<char16_t>& Source;
    std::atomic_uint32_t RefCount;
    const uint32_t Length;
public:
    const LogLevel Level;
private:
    LogMessage(const SharedString<char16_t>& prefix, const uint32_t length, const LogLevel level, const uint64_t time)
        : Timestamp(time), Source(prefix), RefCount(1), Length(length), Level(level) //RefCount is at first 1
    { }
public:
    static LogMessage* MakeMessage(const SharedString<char16_t>& prefix, const char16_t *content, const size_t len, const LogLevel level, const uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count())
    {
        if (len >= UINT32_MAX)
            COMMON_THROW(BaseException, u"Too long for a single LogMessage!");
        uint8_t* ptr = (uint8_t*)malloc_align(sizeof(LogMessage) + sizeof(char16_t)*len, 64);
        if (!ptr)
            return nullptr; //not throw an exception yet
        LogMessage* msg = new (ptr)LogMessage(prefix, static_cast<uint32_t>(len), level, time);
        memcpy_s(ptr + sizeof(LogMessage), sizeof(char16_t)*len, content, sizeof(char16_t)*len);
        return msg;
    }
    template<typename T>
    static LogMessage* MakeMessage(const SharedString<char16_t>& prefix, const T& content, const LogLevel level, const uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count())
    {
        static_assert(std::is_convertible_v<decltype(std::declval<const T>().data()), const char16_t*>, "only accept container of char16_t");

        return MakeMessage(prefix, content.data(), content.size(), level, time);
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
    const std::u16string_view& GetSource() const { return Source; }
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

using MLoggerCallback = std::function<void(const LogMessage& msg)>;

MINILOGAPI std::shared_ptr<LoggerBackend> GetConsoleBackend();
MINILOGAPI std::shared_ptr<LoggerBackend> GetDebuggerBackend();
MINILOGAPI std::shared_ptr<LoggerBackend> GetFileBackend(const fs::path& path);

namespace detail
{
template<typename Char>
struct StrFormater;
template<>
struct MINILOGAPI StrFormater<char>
{
    static fmt::basic_memory_buffer<char>& GetBuffer();
    template<class... Args>
    static std::u16string ToU16Str(const std::basic_string_view<char>& formater, Args&&... args)
    {
        auto& buffer = GetBuffer();
        buffer.resize(0);
        fmt::format_to(buffer, formater, std::forward<Args>(args)...);
        return std::u16string(buffer.begin(), buffer.end());
    }
    static std::u16string ToU16Str(const std::basic_string_view<char>& content)
    {
        return std::u16string(content.cbegin(), content.cend());
    }
};
template<>
struct MINILOGAPI StrFormater<char16_t>
{
    static fmt::basic_memory_buffer<char16_t>& GetBuffer();
    template<class... Args>
    static fmt::basic_memory_buffer<char16_t>& ToU16Str(const std::basic_string_view<char16_t>& formater, Args&&... args)
    {
        auto& buffer = GetBuffer();
        buffer.resize(0);
        fmt::format_to(buffer, formater, std::forward<Args>(args)...);
        return buffer;
    }
    static const std::basic_string_view<char16_t>& ToU16Str(const std::basic_string_view<char16_t>& content)
    {
        return content;
    }
};

template<>
struct MINILOGAPI StrFormater<char32_t>
{
    static fmt::basic_memory_buffer<char32_t>& GetBuffer();
    template<class... Args>
    static std::u16string ToU16Str(const std::basic_string_view<char32_t>& formater, Args&&... args)
    {
        auto& buffer = GetBuffer();
        buffer.resize(0);
        fmt::format_to(buffer, formater, std::forward<Args>(args)...);
        return common::str::detail::CharsetConvertor<common::str::detail::UTF32, common::str::detail::UTF16, char32_t, char16_t>::Convert(buffer.data(), buffer.size(), true, true);
    }
    static std::u16string ToU16Str(const std::basic_string_view<char32_t>& content)
    {
        std::u32string_view sv(content.data());
        return common::str::detail::CharsetConvertor<common::str::detail::UTF32, common::str::detail::UTF16, char32_t, char16_t>::Convert(sv.data(), sv.size(), true, true);
    }
};

template<>
struct MINILOGAPI StrFormater<wchar_t>
{
    static fmt::basic_memory_buffer<wchar_t>& GetBuffer();
    template<class... Args>
    static std::u16string ToU16Str(const std::basic_string_view<wchar_t>& formater, Args&&... args)
    {
        auto& buffer = GetBuffer();
        buffer.resize(0);
        fmt::format_to(buffer, formater, std::forward<Args>(args)...);
        if constexpr(sizeof(wchar_t) == sizeof(char16_t))
            return std::u16string((const char16_t*)buffer.data(), buffer.size());
        else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
            return common::str::detail::CharsetConvertor<common::str::detail::UTF32, common::str::detail::UTF16, wchar_t, char16_t>::Convert(buffer.data(), buffer.size(), true, true);
        else
            return u"";
    }
    static std::u16string ToU16Str(const std::basic_string_view<wchar_t>& content)
    {
        if constexpr(sizeof(wchar_t) == sizeof(char16_t))
            return std::u16string(content.cbegin(), content.cend());
        else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
            return StrFormater<char32_t>::ToU16Str(*reinterpret_cast<const std::basic_string_view<char32_t>*>(&content));
        else
            return u"";
    }
};
}


}
}