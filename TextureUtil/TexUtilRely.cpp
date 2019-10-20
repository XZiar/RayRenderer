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

std::string LoadShaderFromDLL(int32_t id)
{
    auto data = ResourceHelper::GetData(L"SHADER", id);
    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

}