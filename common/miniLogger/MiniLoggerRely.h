#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef MINILOG_EXPORT
#   define MINILOGAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define MINILOGAPI _declspec(dllimport)
# endif
#else
# ifdef MINILOG_EXPORT
#   define MINILOGAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define MINILOGAPI
# endif
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
    template<size_t N>
    static LogMessage* MakeMessage(const SharedString<char16_t>& prefix, const char16_t(&content)[N], const LogLevel level, const uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count())
    {
        return MakeMessage(prefix, content, N - 1, level, time);
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
struct MINILOGAPI StrFormater
{
private:
    template<typename Char>
    static decltype(auto) BufToU16(fmt::basic_memory_buffer<Char>& buffer)
    {
        if constexpr (std::is_same_v<Char, char16_t>)
            return buffer;
        else if constexpr (std::is_same_v<Char, char>)
            return common::str::detail::CharsetConvertor<common::str::detail::UTF8, common::str::detail::UTF16, char, char16_t>::Convert(buffer.data(), buffer.size(), true, true);
        else if constexpr (std::is_same_v<Char, char32_t>)
            return common::str::detail::CharsetConvertor<common::str::detail::UTF32, common::str::detail::UTF16, char32_t, char16_t>::Convert(buffer.data(), buffer.size(), true, true);
        else
            static_assert(!common::AlwaysTrue<Char>(), "unexpected Char type");
    }
public:
    template<typename Char>
    static fmt::basic_memory_buffer<Char>& GetBuffer();
    template<typename T, typename... Args>
    static decltype(auto) ToU16Str(const T& formater, Args&&... args)
    {
        if constexpr (std::is_base_of_v<fmt::compile_string, T>)
        {
            using Char = typename T::Char;
            static_assert(!std::is_same_v<Char, wchar_t>, "no plan to support wchar_t at compile time");
            auto& buffer = GetBuffer<Char>();
            buffer.resize(0);
            fmt::format_to(buffer, formater, std::forward<Args>(args)...);
            return BufToU16(buffer);
        }
        else if constexpr (std::is_convertible_v<const T&, const std::string_view&>)
        {
            auto& buffer = GetBuffer<char>();
            buffer.resize(0);
            fmt::format_to(buffer, static_cast<const std::string_view&>(formater), std::forward<Args>(args)...);
            return BufToU16(buffer);
        }
        else if constexpr (std::is_convertible_v<const T&, const std::u16string_view&>)
        {
            auto& buffer = GetBuffer<char16_t>();
            buffer.resize(0);
            fmt::format_to(buffer, static_cast<const std::u16string_view&>(formater), std::forward<Args>(args)...);
            return BufToU16(buffer);
        }
        else if constexpr (std::is_convertible_v<const T&, const std::u32string_view&>)
        {
            auto& buffer = GetBuffer<char32_t>();
            buffer.resize(0);
            fmt::format_to(buffer, static_cast<const std::u32string_view&>(formater), std::forward<Args>(args)...);
            return BufToU16(buffer);
        }
        else if constexpr (std::is_convertible_v<const T&, const std::wstring_view&>)
        {
            const auto& wstr = static_cast<const std::wstring_view&>(formater);
            if constexpr (sizeof(wchar_t) == sizeof(char16_t))
                return ToU16Str(std::u16string_view(reinterpret_cast<const char16_t*>(wstr.data()), wstr.size()), std::forward<Args>(args)...);
            else if constexpr (sizeof(wchar_t) == sizeof(char32_t))
                return ToU16Str(std::u32string_view(reinterpret_cast<const char32_t*>(wstr.data()), wstr.size()), std::forward<Args>(args)...);
            else
                static_assert(!common::AlwaysTrue<T>(), "unexpected wchar_t size");
        }
        else
        {
            static_assert(!common::AlwaysTrue<T>(), "unknown formater type");
        }
    }
    template<typename Char>
    static decltype(auto) ToU16Str(const std::basic_string_view<Char>& content)
    {
        if constexpr (std::is_same_v<Char, char16_t>)
            return content;
        else if constexpr (std::is_same_v<Char, char>)
            return common::str::detail::CharsetConvertor<common::str::detail::UTF8, common::str::detail::UTF16, char, char16_t>::Convert(content.data(), content.size(), true, true);
        else if constexpr (std::is_same_v<Char, char32_t>)
            return common::str::detail::CharsetConvertor<common::str::detail::UTF32, common::str::detail::UTF16, char32_t, char16_t>::Convert(content.data(), content.size(), true, true);
        else if constexpr (std::is_same_v<Char, wchar_t>)
        {
            if constexpr (sizeof(wchar_t) == sizeof(char16_t))
                return ToU16Str(std::u16string_view(reinterpret_cast<const char16_t*>(content.data()), content.size()));
            else if constexpr (sizeof(wchar_t) == sizeof(char32_t))
                return ToU16Str(std::u32string_view(reinterpret_cast<const char32_t*>(content.data()), content.size()));
            else
                static_assert(!common::AlwaysTrue<Char>(), "unexpected wchar_t size");
        }
        else
            static_assert(!common::AlwaysTrue<Char>(), "unexpected Char type");
    }
};


}

}
}