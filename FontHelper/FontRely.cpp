#include "FontPch.h"
#include "common/ResourceHelper.inl"

namespace oglu
{
using common::ResourceHelper;


using namespace common::mlog;
MiniLogger<false>& fntLog()
{
    static MiniLogger<false> fntlog(u"FontHelper", { GetConsoleBackend() });
    return fntlog;
}

std::string LoadShaderFromDLL(int32_t id)
{
    auto data = ResourceHelper::GetData(L"SHADER", id);
    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

}