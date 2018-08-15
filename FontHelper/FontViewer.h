#pragma once

#include "FontRely.h"


namespace oglu
{


class FONTHELPAPI FontViewer : public NonCopyable
{
private:
    oglVBO viewRect;
    oglVAO viewVAO;
public:
    oglDrawProgram prog;
    FontViewer();
    void Draw();
    void BindTexture(const oglTex2D& tex);
};

}
