#include "ResourcePackagerRely.h"

namespace xziar::respak
{

using namespace common::mlog;
MiniLogger<false>& rpakLog()
{
    static MiniLogger<false> respaklog(u"ResourcePacker", { GetConsoleBackend() });
    return respaklog;
}


}