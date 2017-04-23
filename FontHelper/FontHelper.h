#pragma once

#include "FontRely.h"
#include <filesystem>

namespace oglu
{

class FontViewer;
namespace detail
{

namespace fs = std::experimental::filesystem;
class FONTHELPAPI FontViewerProgram
{
private:
	friend class FontViewer;
	oglProgram prog;
	FontViewerProgram();
};


}

class FONTHELPAPI FontCreater : public NonCopyable
{
private:
	ft::FreeTyper ft2;
	oglTexture testTex;
public:
	FontCreater(const fs::path& fontpath);
	oglTexture getTexture() const;
};


class FONTHELPAPI FontViewer : public NonCopyable
{
private:
	oglBuffer viewRect;
	oglVAO viewVAO;
public:
	static detail::FontViewerProgram& getProgram();
	FontViewer();
	void draw();
};

}
