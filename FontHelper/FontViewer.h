#pragma once

#include "FontRely.h"


namespace oglu
{


class FONTHELPAPI FontViewer : public NonCopyable
{
private:
    oglBuffer viewRect;
    oglVAO viewVAO;
public:
    oglProgram prog;
    FontViewer();
    void draw();
    void bindTexture(const oglTexture& tex);
};

}
