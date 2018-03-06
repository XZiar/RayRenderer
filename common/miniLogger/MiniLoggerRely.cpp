#include "MiniLoggerRely.h"
#include "common/ThreadEx.inl"
#include <array>

namespace common::mlog
{

namespace detail
{

constexpr auto GenLevelNumStr()
{
    std::array<char16_t, 256 * 4> ret{ u'\0' };
    for (uint32_t i = 0, j = 0; i < 256; ++i)
    {
        ret[j++] = i / 100 + u'0';
        ret[j++] = (i % 100) / 10 + u'0';
        ret[j] = (i % 10) + u'0';
        j += 2;
    }
    return ret;
}
const static auto LevelNumStr = GenLevelNumStr();


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
template<size_t N>
struct EquType { };
template<> struct EquType<1> { using Formater = StrFormater<char>; };
template<> struct EquType<2> { using Formater = StrFormater<char16_t>; };
template<> struct EquType<4> { using Formater = StrFormater<char32_t>; };
fmt::BasicMemoryWriter<wchar_t>& StrFormater<wchar_t>::GetWriter()
{
    return *reinterpret_cast<fmt::BasicMemoryWriter<wchar_t>*>(&EquType<sizeof(wchar_t)>::Formater::GetWriter());
}

}
const char16_t* __cdecl GetLogLevelStr(const LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug: return u"Debug";
    case LogLevel::Verbose: return u"Verbose";
    case LogLevel::Info: return u"Info";
    case LogLevel::Warning: return u"Warning";
    case LogLevel::Success: return u"Success";
    case LogLevel::Error: return u"Error";
    case LogLevel::None: return u"None";
    default:
        return &detail::LevelNumStr[(uint8_t)level * 4];
    }
}

}
