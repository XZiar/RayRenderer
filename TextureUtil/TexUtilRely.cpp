#include "TexUtilRely.h"
#include "common/ResourceHelper.inl"
#include "common/ThreadEx.inl"

#pragma message("Compiling TextureUtil with " STRINGIZE(COMMON_SIMD_INTRIN) )

namespace oglu::texutil
{
using common::ResourceHelper;


using namespace common::mlog;
MiniLogger<false>& texLog()
{
    static MiniLogger<false> texclog(u"TexUtil", { GetConsoleBackend() });
    return texclog;
}

string getShaderFromDLL(int32_t id)
{
    auto data = ResourceHelper::getData(L"SHADER", id);
    data.push_back('\0');
    return string((const char*)data.data());
}

}