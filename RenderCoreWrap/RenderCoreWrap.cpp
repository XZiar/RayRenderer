#include "RenderCoreWrapRely.h"
#include "RenderCoreWrap.h"
#include "ImageUtil.h"
#include "common/CLIAsync.hpp"


namespace Dizz
{


RenderCore::RenderCore() : Core(new rayr::RenderCore())
{
    Core->TestSceneInit();
    theScene = gcnew Scene(Core);
    PostProc = gcnew Common::Controllable(Core->GetPostProc());
}

RenderCore::!RenderCore()
{
    if (const auto ptr = ExchangeNullptr(Core); ptr)
        delete ptr;
}

void RenderCore::Draw()
{
    Core->Draw();
}

void RenderCore::Resize(const uint32_t w, const uint32_t h)
{
    Core->Resize(w, h);
}


Drawable^ ConstructDrawable(CLIWrapper<Wrapper<rayr::Model>>^ theModel)
{
    return gcnew Drawable(theModel->Extract());
}
Task<Drawable^>^ RenderCore::LoadModelAsync(String^ fname)
{
    return doAsync3<Drawable^>(gcnew Func<CLIWrapper<Wrapper<rayr::Model>>^, Drawable^>(&ConstructDrawable),
        &rayr::RenderCore::LoadModelAsync, *Core, ToU16Str(fname));
}

void RenderCore::Serialize(String^ path)
{
    Core->Serialize(ToU16Str(path));
}

void RenderCore::DeSerialize(String^ path)
{
    Core->DeSerialize(ToU16Str(path));
    delete theScene;
    theScene = gcnew Scene(Core);
    OnPropertyChanged("TheScene");
}

Action<String^>^ RenderCore::Screenshot()
{
    auto saver = gcnew XZiar::Img::ImageHolder(Core->Screenshot());
    return gcnew Action<String^>(saver, &XZiar::Img::ImageHolder::Save);
}


}