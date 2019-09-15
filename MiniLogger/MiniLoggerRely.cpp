#include "MiniLoggerRely.h"
#include "common/ThreadEx.inl"
#include "common/ColorConsole.inl"
#include <array>

#include <boost/version.hpp>
#pragma message("Compiling MiniLogger with boost[" STRINGIZE(BOOST_LIB_VERSION) "]" )
#pragma message("Compiling MiniLogger with fmt[" STRINGIZE(FMT_VERSION) "]" )


namespace common::mlog
{

const LogMessage::TimeConv LogMessage::TimeBase = []() -> LogMessage::TimeConv
{
    return { std::chrono::system_clock::now(), static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) };
}();

namespace detail
{

template<>
MINILOGAPI fmt::basic_memory_buffer<char>& StrFormater::GetBuffer<char>(const bool needClear)
{
    static thread_local fmt::basic_memory_buffer<char> out;
    if (needClear)
        out.resize(0);
    return out;
}
template<>
MINILOGAPI fmt::basic_memory_buffer<char16_t>& StrFormater::GetBuffer<char16_t>(const bool needClear)
{
    static thread_local fmt::basic_memory_buffer<char16_t> out;
    if (needClear)
        out.resize(0);
    return out;
}
template<>
MINILOGAPI fmt::basic_memory_buffer<char32_t>& StrFormater::GetBuffer<char32_t>(const bool needClear)
{
    static thread_local fmt::basic_memory_buffer<char32_t> out;
    if (needClear)
        out.resize(0);
    return out;
}
template<>
MINILOGAPI fmt::basic_memory_buffer<wchar_t>& StrFormater::GetBuffer<wchar_t>(const bool needClear)
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
            auto ptr = &LevelNumStr[(uint8_t)level * 4];
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

