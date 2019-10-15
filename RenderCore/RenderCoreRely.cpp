#include "RenderCorePch.h"
#include "common/ResourceHelper.inl"

#include <boost/version.hpp>
#pragma message("Compiling RenderCore with boost[" STRINGIZE(BOOST_LIB_VERSION) "]" )
#pragma message("Compiling RenderCore with " STRINGIZE(COMMON_SIMD_INTRIN) )

namespace rayr
{
using common::ResourceHelper;

using namespace common::mlog;
MiniLogger<false>& dizzLog()
{
    static MiniLogger<false> dizzlog(u"RenderCore", { GetConsoleBackend() });
    return dizzlog;
}

string getShaderFromDLL(int32_t id)
{
    auto data = ResourceHelper::getData(L"SHADER", id);
    data.push_back('\0');
    return string((const char*)data.data());
}

}