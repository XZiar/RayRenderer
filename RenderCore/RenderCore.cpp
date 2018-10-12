#include "RenderCoreRely.h"
#include "resource.h"
#include "RenderCore.h"

namespace rayr
{


PBRBasicPass::PBRBasicPass()
{
}

void PBRBasicPass::OnPrepare(RenderPassContext& context)
{
}

void PBRBasicPass::OnDraw(RenderPassContext& context)
{
    DefaultRenderPass::OnDraw(context);
}


}