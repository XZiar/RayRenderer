#pragma once

#include "FontRely.h"

namespace oglu
{

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
class FONTHELPAPI FontViewer : public common::NonCopyable, public common::Controllable
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
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

}
