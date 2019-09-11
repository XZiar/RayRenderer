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
Task<Drawable^>^ RenderCore::LoadModelAsync2(String^ fname)
{
    return doAsync3<Drawable^>(gcnew Func<CLIWrapper<Wrapper<rayr::Model>>^, Drawable^>(&ConstructDrawable),
        &rayr::RenderCore::LoadModelAsync, *Core, ToU16Str(fname));
}


template<class RetType>
inline void __cdecl SetTcsResult(const gcroot<TaskCompletionSource<RetType>^>& tcs, const gcroot<RetType>& ret)
{
    tcs->SetResult(ret);
}

#pragma managed(push, off)

template<auto Caller, auto Convertor, typename NativeT, typename ManagedT, typename C, typename... Args>
inline void NewDoAsyncNative(C& self, gcroot<TaskCompletionSource<ManagedT>^> tcs, Args... args)
{
    //common::PromiseResult<NativeT> pms = self.*Caller(std::forward<Args>(args)...);
    common::PromiseResult<NativeT> pms = std::invoke(Caller, self, std::forward<Args>(args)...);
    common::asyexe::AsyncProxy::OnComplete<NativeT>(pms, [=](NativeT obj)
        {
            auto obj2 = Convertor(obj);
            SetTcsResult(tcs, obj2);
        });

}
#pragma managed(pop)

template<auto Caller, auto Convertor, typename C, typename... Args>
inline auto NewDoAsync(C& self, Args... args)
{
    static_assert(std::is_invocable_v<decltype(Caller), C, Args...>, "invalid caller");
    using TaskPtr = std::invoke_result_t<decltype(Caller), C, Args...>;
    static_assert(common::is_specialization<TaskPtr, std::shared_ptr>::value, "task should be wrapped by shared_ptr");
    using TaskType = typename TaskPtr::element_type;
    static_assert(common::is_specialization<TaskType, common::detail::PromiseResult_>::value, "task should be PromiseResult");
    using NativeT = typename TaskType::ResultType;
    static_assert(std::is_invocable_v<decltype(Convertor), NativeT&>, "convertor should accept a reference of ResultType");
    //using ManagedT = std::invoke_result_t<decltype(Convertor), NativeT&>;
    using ManagedT2 = std::invoke_result_t<decltype(Convertor), NativeT&>;
    using ManagedT = decltype(std::declval<ManagedT2&>().operator->());
    //using ManagedT = decltype(*std::declval<ManagedT2&>());

    gcroot<TaskCompletionSource<ManagedT>^> tcs = gcnew TaskCompletionSource<ManagedT>();
    NewDoAsyncNative<Caller, Convertor, NativeT, ManagedT>(self, tcs, std::forward<Args>(args)...);
    return tcs->Task;

}

static gcroot<Drawable^> __cdecl ConvDrawable(Wrapper<rayr::Model>& theModel)
{
    return gcnew Drawable(theModel);
}

Task<Drawable^>^ RenderCore::LoadModelAsync(String^ fname)
{
    /*static_assert(std::is_invocable_v<decltype(&rayr::RenderCore::LoadModelAsync2), rayr::RenderCore, std::u16string>, "invalid caller");
    static_assert(std::is_invocable_v<decltype(ConvDrawable), Wrapper<rayr::Model>&>, "convertor should accept a reference of ResultType");
    using ManagedT = std::invoke_result_t<decltype(ConvDrawable), Wrapper<rayr::Model>&>;

    using MT = decltype(std::declval<ManagedT&>().operator->());*/

    return NewDoAsync<&rayr::RenderCore::LoadModelAsync2, ConvDrawable>(*Core, ToU16Str(fname));
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