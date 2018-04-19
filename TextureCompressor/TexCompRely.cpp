#include "TexCompRely.h"
#include "common/ResourceHelper.h"

namespace oglu::texcomp
{
using common::ResourceHelper;


using namespace common::mlog;
MiniLogger<false>& texcLog()
{
    static MiniLogger<false> texclog(u"TexComp", { GetConsoleBackend() });
    return texclog;
}

string getShaderFromDLL(int32_t id)
{
    auto data = ResourceHelper::getData(L"SHADER", id);
    data.push_back('\0');
    return string((const char*)data.data());
}

}