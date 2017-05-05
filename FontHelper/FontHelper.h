#pragma once

#include "FontRely.h"
#include "../common/SharedResource.hpp"

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
	oclu::oclKernel sdfker, sdfgreyker;
	static SharedResource<oclu::oclContext> clRes;
	oclu::oclContext clCtx;
	oclu::oclCmdQue clQue;
public:
	FontCreater(const fs::path& fontpath);
	~FontCreater();
	oglTexture getTexture() const { return testTex; }
	void setChar(wchar_t ch, bool custom) const;
	void stroke() const;
	void bmpsdf(wchar_t ch) const;
	void clbmpsdf(wchar_t ch) const;
	void clbmpsdfgrey(wchar_t ch) const;
	void clbmpsdfs(wchar_t ch, uint16_t count) const;
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
	void bindTexture(const oglTexture& tex);
};

}
