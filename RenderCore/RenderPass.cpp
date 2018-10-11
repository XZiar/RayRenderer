#include "RenderCoreRely.h"
#include "RenderPass.h"


namespace rayr
{

RenderPass::RenderPass() : Controllable(u"")
{

}

std::shared_ptr<Scene> rayr::RenderPass::GetScene() const
{
    return TheScene.lock();
}

void RenderPass::RegisterDrawable(std::weak_ptr<Drawable> drawable)
{
    Drawables.insert(std::move(drawable));
}


}
