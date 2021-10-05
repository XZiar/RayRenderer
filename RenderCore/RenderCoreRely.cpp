#include "RenderCorePch.h"
#include "common/ResourceHelper.inl"

#include <boost/version.hpp>
#pragma message("Compiling RenderCore with boost[" STRINGIZE(BOOST_LIB_VERSION) "]" )
#pragma message("Compiling RenderCore with " STRINGIZE(COMMON_SIMD_INTRIN) )

namespace dizz
{
using common::ResourceHelper;

using namespace common::mlog;
MiniLogger<false>& dizzLog()
{
    static MiniLogger<false> dizzlog(u"RenderCore", { GetConsoleBackend() });
    return dizzlog;
}

std::string LoadShaderFromDLL(int32_t id)
{
    auto data = ResourceHelper::GetData(L"SHADER", id);
    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

}