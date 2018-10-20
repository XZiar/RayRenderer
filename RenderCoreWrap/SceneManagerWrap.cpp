#include "RenderCoreWrapRely.h"
#include "SceneManagerWrap.h"
#include "ImageUtil.h"
#include "common/CLIAsync.hpp"


namespace Dizz
{


Scene::Scene(const rayr::RenderCore * core) : Core(core)
{
    TheScene = new std::weak_ptr<rayr::Scene>(Core->GetScene());
}

Scene::!Scene()
{
    if (const auto ptr = ExchangeNullptr(TheScene); ptr)
        delete ptr;
}


}