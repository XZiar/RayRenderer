#pragma once

#include "FontRely.h"


namespace oglu
{

class FontViewer;
namespace detail
{
class FONTHELPAPI FontViewerProgram
{
private:
	friend class FontViewer;
    oglProgram prog;
    FontViewerProgram();
};
}


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
