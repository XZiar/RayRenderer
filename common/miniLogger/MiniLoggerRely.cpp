#include "MiniLoggerRely.h"
#include "common/ThreadEx.inl"

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
