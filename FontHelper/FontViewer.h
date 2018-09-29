#pragma once

#include "FontRely.h"
#include "common/Controllable.hpp"

namespace oglu
{

#if defined(COMPILER_MSVC) && COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif
class FONTHELPAPI FontViewer : public NonCopyable, public common::Controllable
{
private:
    oglVBO viewRect;
    oglVAO viewVAO;
    void RegisterControllable();
public:
    oglDrawProgram prog;
    FontViewer();
    virtual std::u16string_view GetControlType() const override 
    {
        using namespace std::literals;
        return u"FontViewer"sv;
    }
    void Draw();
    void BindTexture(const oglTex2D& tex);
};
#if defined(COMPILER_MSVC) && COMPILER_MSVC
#   pragma warning(pop)
#endif

}
