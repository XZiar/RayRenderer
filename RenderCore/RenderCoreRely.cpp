#include "RenderCoreRely.h"
#include "common/ResourceHelper.h"
#include "common/ThreadEx.inl"

#include <boost/version.hpp>
#pragma message("Compiling RenderCore with boost[" STRINGIZE(BOOST_LIB_VERSION) "]" )

template struct RAYCOREAPI common::AlignBase<16>;
template struct RAYCOREAPI common::AlignBase<32>;

namespace rayr
{
using common::ResourceHelper;

using namespace common::mlog;
MiniLogger<false>& basLog()
{
    static MiniLogger<false> baslog(u"BasicTest", { GetConsoleBackend() });
    return baslog;
}

string getShaderFromDLL(int32_t id)
{
    auto data = ResourceHelper::getData(L"SHADER", id);
    data.push_back('\0');
    return string((const char*)data.data());
}

}