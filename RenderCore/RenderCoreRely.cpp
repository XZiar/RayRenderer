#include "RenderCoreRely.h"
#include "common/ResourceHelper.h"
#include "common/ThreadEx.inl"

#include <boost/version.hpp>
#pragma message("Compiling RenderCore with boost[" STRINGIZE(BOOST_LIB_VERSION) "]" )
#pragma message("Compiling RenderCore with " STRINGIZE(COMMON_SIMD_INTRIN) )

template struct RAYCOREAPI common::AlignBase<16>;
template struct RAYCOREAPI common::AlignBase<32>;

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