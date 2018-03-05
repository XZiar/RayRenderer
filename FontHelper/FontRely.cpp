#include "FontRely.h"
#include "common/ResourceHelper.h"

namespace oglu
{
using common::ResourceHelper;


using namespace common::mlog;
MiniLogger<false>& fntLog()
{
    static MiniLogger<false> fntlog(u"FontHelper", { GetConsoleBackend() });
    return fntlog;
}

string getShaderFromDLL(int32_t id)
{
    auto data = ResourceHelper::getData(L"SHADER", id);
    data.push_back('\0');
    return string((const char*)data.data());
}

}