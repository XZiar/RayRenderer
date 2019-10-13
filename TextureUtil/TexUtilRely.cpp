#include "TexUtilPch.h"
#include "common/ResourceHelper.inl"

#pragma message("Compiling TextureUtil with " STRINGIZE(COMMON_SIMD_INTRIN) )

namespace oglu::texutil
{
using common::ResourceHelper;



common::mlog::MiniLogger<false>& texLog()
{
    static common::mlog::MiniLogger<false> texclog(u"TexUtil", { common::mlog::GetConsoleBackend() });
    return texclog;
}

std::string getShaderFromDLL(int32_t id)
{
    auto data = ResourceHelper::getData(L"SHADER", id);
    data.push_back('\0');
    return std::string((const char*)data.data());
}

}