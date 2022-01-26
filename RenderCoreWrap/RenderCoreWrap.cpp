#include "RenderCoreWrapRely.h"
#include "RenderCoreWrap.h"
#include "ImageUtil.h"
#include "common/CLIAsync.hpp"
#pragma comment(lib, "user32.lib")

using common::container::ValSet;

namespace Dizz
{


std::shared_ptr<dizz::RenderPass> RenderPass::GetSelf()
{
    return std::dynamic_pointer_cast<dizz::RenderPass>(GetControl()); // virtual base require dynamic_cast
}

RenderPass::RenderPass(const std::shared_ptr<dizz::RenderPass>& shader) : Controllable(shader)
{
}
RenderPass::RenderPass(std::shared_ptr<dizz::RenderPass>&& shader) : Controllable(shader), TempHandle(new std::shared_ptr<dizz::RenderPass>(shader))
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

static dizz::RenderCore* CreateCore(void* hdc)
{
    const auto loader = static_cast<oglu::WGLLoader*>(oglu::oglLoader::GetLoader("WGL"));
    const auto host = loader->CreateHost(reinterpret_cast<HDC>(hdc));
    return TryConstruct<dizz::RenderCore>(*host);
}

RenderCore::RenderCore(IntPtr hdc) : Core(CreateCore(hdc.ToPointer()))
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
    TexLoader = gcnew TextureLoader(Core->GetTexLoader());
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
    try
    {
        const auto ctx = oglu::oglContext_::CurrentContext();
        oglu::oglDefaultFrameBuffer_::Get()->ClearAll();
        Core->Draw();
        ctx->SwapBuffer();
        theScene->PrepareScene();
    }
    catch (const common::BaseException& be)
    {
        throw Common::CPPException::FromException(be);
    }
}

void RenderCore::Resize(const uint32_t w, const uint32_t h)
{
    Core->Resize(w, h);
}



static gcroot<Drawable^> __cdecl ConvDrawable(std::shared_ptr<dizz::Model> theModel)
{
    return gcnew Drawable(std::move(theModel));
}

Task<Drawable^>^ RenderCore::LoadModelAsync(String^ fname)
{
    return NewDoAsync<&dizz::RenderCore::LoadModelAsync2, ConvDrawable>(*Core, ToU16Str(fname));
}

static gcroot<RenderPass^> ConvRenderPass(std::shared_ptr<dizz::DefaultRenderPass> pass)
{
    return gcnew RenderPass(std::move(pass));
}
Task<RenderPass^>^ RenderCore::LoadShaderAsync(String^ fname, String^ shaderName)
{
    return NewDoAsync<&dizz::RenderCore::LoadShaderAsync2, ConvRenderPass>(*Core, ToU16Str(fname), ToU16Str(shaderName));
}


//static RenderPass^ ConstructRenderPass(CLIWrapper<Wrapper<dizz::DefaultRenderPass>>^ pass)
//{
//    return gcnew RenderPass(pass->Extract());
//}
//Task<RenderPass^>^ RenderCore::LoadShaderAsync(String^ fname, String^ shaderName)
//{
//    return doAsync3<RenderPass^>(gcnew Func<CLIWrapper<Wrapper<dizz::DefaultRenderPass>>^, RenderPass^>(&ConstructRenderPass),
//        &dizz::RenderCore::LoadShaderAsync, *Core, ToU16Str(fname), ToU16Str(shaderName));
//}

void RenderCore::BeforeAddPass(Object^ sender, RenderPass^ object, bool% shouldAdd)
{
    const auto self = std::dynamic_pointer_cast<dizz::DefaultRenderPass>(object->GetSelf());
    if (self && Core->AddShader(self))
    {
        object->ReleaseTempHandle();
        shouldAdd = true;
    }
}
void RenderCore::BeforeDelPass(Object^ sender, RenderPass^ object, bool% shouldDel)
{
    const auto self = std::dynamic_pointer_cast<dizz::DefaultRenderPass>(object->GetSelf());
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


void RenderCore::InjectRenderDoc(String^ dllPath)
{
    oglu::oglUtil::InJectRenderDoc(ToU16Str(dllPath));
}
void RenderCore::InitGLEnvironment()
{
    HWND tmpWND = CreateWindow(
        L"Static", L"Fake Window",            // window class, title
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN,  // style
        0, 0,                               // position x, y
        1, 1,                               // width, height
        NULL, NULL,                         // parent window, menu
        nullptr, NULL);                     // instance, param
    HDC tmpDC = GetDC(tmpWND);
    const auto loader = static_cast<oglu::WGLLoader*>(oglu::oglLoader::GetLoader("WGL"));
    loader->CreateHost(tmpDC);
}


}