#include "RenderCoreWrapRely.h"
#include "RenderCoreWrap.h"
#include "ImageUtil.h"
#include "common/CLIAsync.hpp"

using common::container::ValSet;

namespace Dizz
{


std::shared_ptr<rayr::RenderPass> RenderPass::GetSelf()
{
    return std::dynamic_pointer_cast<rayr::RenderPass>(GetControl()); // virtual base require dynamic_cast
}

RenderPass::RenderPass(const Wrapper<rayr::RenderPass>& shader) : Controllable(shader)
{
}
RenderPass::RenderPass(Wrapper<rayr::RenderPass>&& shader) : Controllable(shader), TempHandle(new Wrapper<rayr::RenderPass>(shader))
{
}
void RenderPass::ReleaseTempHandle()
{
    if (const auto ptr = ExchangeNullptr(TempHandle); ptr)
        delete ptr;
}
RenderPass::!RenderPass()
{
    ReleaseTempHandle();
}

String^ RenderPass::ToString()
{
    Object^ name = nullptr;
    if (DoGetMember("Name", name))
        return "[Pass]" + name;
    else
        return "[Pass]";
}


RenderCore::RenderCore() : Core(new rayr::RenderCore())
{
    Core->TestSceneInit();
    theScene = gcnew Scene(Core);
    theScene->Drawables->CollectionChanged += gcnew NotifyCollectionChangedEventHandler(this, &RenderCore::OnDrawablesChanged);
    Controls = gcnew List<Controllable^>();
    for (const auto ctl : Core->GetControllables())
        Controls->Add(gcnew Controllable(ctl));
    Passes = gcnew ObservableProxyContainer<RenderPass^>();
    Passes->BeforeAddObject += gcnew AddObjectEventHandler<RenderPass^>(this, &RenderCore::BeforeAddPass);
    Passes->BeforeDelObject += gcnew DelObjectEventHandler<RenderPass^>(this, &RenderCore::BeforeDelPass);
    Passes->CollectionChanged += gcnew NotifyCollectionChangedEventHandler(this, &RenderCore::OnPassesChanged);
    for (const auto& pass : Core->GetRenderPasses())
        Passes->InnerAdd(gcnew RenderPass(pass));
    ThumbMan = gcnew ThumbnailMan(Core->GetThumbMan());
}

RenderCore::!RenderCore()
{
    if (const auto ptr = ExchangeNullptr(Core); ptr)
        delete ptr;
}

void RenderCore::OnDrawablesChanged(Object ^ sender, NotifyCollectionChangedEventArgs^ e)
{
    switch (e->Action)
    {
    case NotifyCollectionChangedAction::Add:
        for each (Object^ item in e->NewItems)
        {
            const auto drawable = static_cast<Drawable^>(item)->GetSelf(); // type promised
            for (const auto& shd : Core->GetRenderPasses())
                shd->RegistDrawable(drawable);
        }
        break;
    default: break;
    }
}

void RenderCore::Draw()
{
    Core->Draw();
    theScene->PrepareScene();
}

void RenderCore::Resize(const uint32_t w, const uint32_t h)
{
    Core->Resize(w, h);
}


static Drawable^ ConstructDrawable(CLIWrapper<Wrapper<rayr::Model>>^ theModel)
{
    return gcnew Drawable(theModel->Extract());
}
Task<Drawable^>^ RenderCore::LoadModelAsync(String^ fname)
{
    return doAsync3<Drawable^>(gcnew Func<CLIWrapper<Wrapper<rayr::Model>>^, Drawable^>(&ConstructDrawable),
        &rayr::RenderCore::LoadModelAsync, *Core, ToU16Str(fname));
}

static RenderPass^ ConstructRenderPass(CLIWrapper<Wrapper<rayr::DefaultRenderPass>>^ pass)
{
    return gcnew RenderPass(pass->Extract());
}
Task<RenderPass^>^ RenderCore::LoadShaderAsync(String^ fname, String^ shaderName)
{
    return doAsync3<RenderPass^>(gcnew Func<CLIWrapper<Wrapper<rayr::DefaultRenderPass>>^, RenderPass^>(&ConstructRenderPass),
        &rayr::RenderCore::LoadShaderAsync, *Core, ToU16Str(fname), ToU16Str(shaderName));
}

void RenderCore::BeforeAddPass(Object^ sender, RenderPass^ object, bool% shouldAdd)
{
    const auto self = std::dynamic_pointer_cast<rayr::DefaultRenderPass>(object->GetSelf());
    if (self && Core->AddShader(self))
    {
        object->ReleaseTempHandle();
        shouldAdd = true;
    }
}
void RenderCore::BeforeDelPass(Object^ sender, RenderPass^ object, bool% shouldDel)
{
    const auto self = std::dynamic_pointer_cast<rayr::DefaultRenderPass>(object->GetSelf());
    if (Core->DelShader(self))
    {
        shouldDel = true;
    }
}
void RenderCore::OnPassesChanged(Object^ sender, NotifyCollectionChangedEventArgs^ e)
{
    switch (e->Action)
    {
    case NotifyCollectionChangedAction::Add:
        for each (Object^ item in e->NewItems)
        {
            const auto shd = static_cast<RenderPass^>(item)->GetSelf(); // type promised
            for (const auto& drawable : ValSet(Core->GetScene()->GetDrawables()))
                shd->RegistDrawable(drawable);
        }
        break;
    default: break;
    }
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
    RaisePropertyChanged("TheScene");
}

Action<String^>^ RenderCore::Screenshot()
{
    auto saver = gcnew XZiar::Img::ImageHolder(Core->Screenshot());
    return gcnew Action<String^>(saver, &XZiar::Img::ImageHolder::Save);
}


}