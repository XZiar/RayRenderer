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
    oglProgram prog;
    FontViewer();
    void Draw();
    void BindTexture(const oglTexture& tex);
};

}
