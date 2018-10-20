#pragma once

#include "RenderCoreWrapRely.h"
#include "ControllableWrap.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;
using namespace msclr::interop;


namespace Dizz
{

public ref class Scene : public BaseViewModel
{
internal:
    const rayr::RenderCore * const Core;
    const std::weak_ptr<rayr::Scene> *TheScene;
    Scene(const rayr::RenderCore * core);
public:
    ~Scene() { this->!Scene(); }
    !Scene();
};



}

