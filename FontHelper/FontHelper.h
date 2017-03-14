#pragma once

#include "FontRely.h"
#include <filesystem>

namespace oglu
{

class FontViewer;
namespace inner
{


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
	using Path = std::experimental::filesystem::path;
	oglTexture testTex;
public:
	FontCreater(Path& fontpath);
	oglTexture getTexture() const;
};


class FONTHELPAPI FontViewer : public NonCopyable
{
private:
	static inner::FontViewerProgram& getProgram();
	oglBuffer viewRect;
	oglVAO viewVAO;
public:
	FontViewer();
	void draw();
};

}
