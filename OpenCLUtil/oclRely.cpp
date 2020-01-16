#include "oclPch.h"

#pragma message("Compiling miniBLAS with " STRINGIZE(COMMON_SIMD_INTRIN) )

namespace oclu
{

using namespace common::mlog;
MiniLogger<false>& oclLog()
{
    static MiniLogger<false> ocllog(u"OpenCLUtil", { GetConsoleBackend() });
    return ocllog;
}


std::pair<uint32_t, uint32_t> ParseVersionString(std::u16string_view str, const size_t verPos)
{
    std::pair<uint32_t, uint32_t> version { 0, 0 };
    const auto ver = common::str::SplitStream(str, u' ', false).Skip(verPos).TryGetFirst();
    if (ver.has_value())
    {
        common::str::SplitStream(ver.value(), u'.', false)
            .ForEach([&](const auto part, const auto idx)
                {
                    if (idx < 2)
                    {
                        uint32_t& target = idx == 0 ? version.first : version.second;
                        target = part[0] - u'0';
                    }
                });
    }
    return version;
}


}