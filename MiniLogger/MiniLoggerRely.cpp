#include "MiniLoggerRely.h"
#include <array>

#include <boost/version.hpp>
#pragma message("Compiling MiniLogger with boost[" STRINGIZE(BOOST_LIB_VERSION) "]" )
#pragma message("Compiling MiniLogger with fmt[" STRINGIZE(FMT_VERSION) "]" )


namespace common::mlog
{

using namespace std::chrono;

struct TimeConv 
{ 
    system_clock::time_point SysClock;
    uint64_t Base;
    TimeConv() : SysClock(system_clock::now()),
        Base(static_cast<uint64_t>(high_resolution_clock::now().time_since_epoch().count())) {}
};


LogMessage* LogMessage::MakeMessage(const SharedString<char16_t>& prefix, const char16_t* content, const size_t len, const LogLevel level, const uint64_t time)
{
    if (len >= UINT32_MAX)
        COMMON_THROW(BaseException, u"Too long for a single LogMessage!");
    uint8_t* ptr = (uint8_t*)malloc(sizeof(LogMessage) + sizeof(char16_t) * len);
    Ensures(reinterpret_cast<uintptr_t>(ptr) % 4 == 0);
    if (!ptr)
        return nullptr; //not throw an exception yet
    LogMessage* msg = new (ptr)LogMessage(prefix, static_cast<uint32_t>(len), level, time);
    memcpy_s(ptr + sizeof(LogMessage), sizeof(char16_t) * len, content, sizeof(char16_t) * len);
    return msg;
}

bool LogMessage::Consume(LogMessage* msg)
{
    if (msg->RefCount-- == 1) //last one
    {
        msg->~LogMessage();
        free(msg);
        return true;
    }
    return false;
}

system_clock::time_point LogMessage::GetSysTime() const
{
    static TimeConv TimeBase;
    return TimeBase.SysClock + duration_cast<system_clock::duration>(nanoseconds(Timestamp - TimeBase.Base));
}


void LoggerBackend::Print(LogMessage* msg)
{
    if (msg->Level >= LeastLevel)
        OnPrint(*msg);
    LogMessage::Consume(msg);
}


namespace detail::detail
{

template<>
MINILOGAPI fmt::basic_memory_buffer<char>& GetBuffer<char>(const bool needClear)
{
    static thread_local fmt::basic_memory_buffer<char> out;
    if (needClear)
        out.resize(0);
    return out;
}
template<>
MINILOGAPI fmt::basic_memory_buffer<char16_t>& GetBuffer<char16_t>(const bool needClear)
{
    static thread_local fmt::basic_memory_buffer<char16_t> out;
    if (needClear)
        out.resize(0);
    return out;
}
template<>
MINILOGAPI fmt::basic_memory_buffer<char32_t>& GetBuffer<char32_t>(const bool needClear)
{
    static thread_local fmt::basic_memory_buffer<char32_t> out;
    if (needClear)
        out.resize(0);
    return out;
}
template<>
MINILOGAPI fmt::basic_memory_buffer<wchar_t>& GetBuffer<wchar_t>(const bool needClear)
{
    static thread_local fmt::basic_memory_buffer<wchar_t> out;
    if (needClear)
        out.resize(0);
    return out;
}


}


constexpr auto GenLevelNumStr()
{
    std::array<char16_t, 256 * 4> ret{ u'\0' };
    for (uint16_t i = 0, j = 0; i < 256; ++i)
    {
        ret[j++] = i / 100 + u'0';
        ret[j++] = (i % 100) / 10 + u'0';
        ret[j] = (i % 10) + u'0';
        j += 2;
    }
    return ret;
}
static constexpr auto LevelNumStr = GenLevelNumStr();

std::u16string_view GetLogLevelStr(const LogLevel level)
{
    using namespace std::literals;
    switch (level)
    {
    case LogLevel::Debug:   return u"Debug"sv;
    case LogLevel::Verbose: return u"Verbose"sv;
    case LogLevel::Info:    return u"Info"sv;
    case LogLevel::Warning: return u"Warning"sv;
    case LogLevel::Success: return u"Success"sv;
    case LogLevel::Error:   return u"Error"sv;
    case LogLevel::None:    return u"None"sv;
    default:
        {
            const auto lvNum = static_cast<uint8_t>(level);
            auto ptr = &LevelNumStr[lvNum * 4];
            if (lvNum < 10)
                return { ptr, 1 };
            else if (lvNum < 100)
                return { ptr, 2 };
            else
                return { ptr, 3 };
        }
    }
}


}

