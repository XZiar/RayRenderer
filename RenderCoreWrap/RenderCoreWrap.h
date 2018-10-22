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


public ref class RenderCore : public BaseViewModel
{
private:
    void OnDrawablesChanged(Object^ sender, NotifyCollectionChangedEventArgs^ e);
internal:
    rayr::RenderCore * const Core;
    Scene^ theScene;
public:
    initonly Controllable^ PostProc;

    RenderCore();
    ~RenderCore() { this->!RenderCore(); }
    !RenderCore();

    void Draw();
    void Resize(const uint32_t w, const uint32_t h);

    Task<Drawable^>^ LoadModelAsync(String^ fname);

    void Serialize(String^ path);
    void DeSerialize(String^ path);
    Action<String^>^ Screenshot();

    CLI_READONLY_PROPERTY(uint32_t, Width, Core->GetWindowSize().first)
    CLI_READONLY_PROPERTY(uint32_t, Height, Core->GetWindowSize().second)
    CLI_READONLY_PROPERTY(Scene^, TheScene, theScene)
};



}

