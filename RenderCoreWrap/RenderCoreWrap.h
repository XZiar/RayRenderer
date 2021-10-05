#pragma once

#include "RenderCoreWrapRely.h"
#include "ControllableWrap.h"
#include "SceneManagerWrap.h"
#include "ThumbManWrap.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;
using namespace msclr::interop;


namespace Dizz
{

public ref class RenderPass : public Controllable
{
private:
    const std::shared_ptr<dizz::RenderPass>* TempHandle;
internal:
    std::shared_ptr<dizz::RenderPass> GetSelf();
    RenderPass(const std::shared_ptr<dizz::RenderPass>& shader);
    RenderPass(std::shared_ptr<dizz::RenderPass>&& shader);
    void ReleaseTempHandle();
public:
    ~RenderPass() { this->!RenderPass(); }
    !RenderPass();
    virtual String^ ToString() override;
};


public ref class RenderCore : public BaseViewModel
{
private:
    void OnDrawablesChanged(Object^ sender, NotifyCollectionChangedEventArgs^ e);
internal:
    dizz::RenderCore * const Core;
    Scene^ theScene;
    void BeforeAddPass(Object^ sender, RenderPass^ object, bool% shouldAdd);
    void BeforeDelPass(Object^ sender, RenderPass^ object, bool% shouldDel);
    void OnPassesChanged(Object^ sender, NotifyCollectionChangedEventArgs^ e);
public:
    initonly List<Controllable^>^ Controls;
    initonly ObservableProxyContainer<RenderPass^>^ Passes;
    initonly ThumbnailMan^ ThumbMan;
    initonly TextureLoader^ TexLoader;

    RenderCore(IntPtr hdc);
    ~RenderCore() { this->!RenderCore(); }
    !RenderCore();

    void Draw();
    void Resize(const uint32_t w, const uint32_t h);

    Task<Drawable^>^ LoadModelAsync(String^ fname);
    Task<RenderPass^>^ LoadShaderAsync(String^ fname, String^ shaderName);

    void Serialize(String^ path);
    void DeSerialize(String^ path);
    Action<String^>^ Screenshot();

    CLI_READONLY_PROPERTY(uint32_t, Width, Core->GetWindowSize().first)
    CLI_READONLY_PROPERTY(uint32_t, Height, Core->GetWindowSize().second)
    CLI_READONLY_PROPERTY(Scene^, TheScene, theScene)

    static void InjectRenderDoc(String^ dllPath);
    static void InitGLEnvironment();
};



}

