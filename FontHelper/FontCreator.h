#pragma once

#include "FontRely.h"
#include "../common/SharedResource.hpp"

namespace oglu
{

class FONTHELPAPI FontCreator : public NonCopyable
{
private:
	ft::FreeTyper ft2;
	oglTexture testTex;
	oclu::oclKernel sdfker, sdfgreyker;
	static SharedResource<oclu::oclContext> clRes;
	oclu::oclContext clCtx;
	oclu::oclCmdQue clQue;
	void loadCL(const string& src);
public:
	FontCreator(const fs::path& fontpath);
	~FontCreator();
	void reload(const string& src);
	oglTexture getTexture() const { return testTex; }
	void setChar(wchar_t ch, bool custom) const;
	void stroke() const;
	void bmpsdf(wchar_t ch) const;
	void clbmpsdf(wchar_t ch) const;
	void clbmpsdfgrey(wchar_t ch) const;
	void clbmpsdfs(wchar_t ch, uint16_t count) const;
	Image<ImageType::GREY> clgreysdfs(wchar_t ch, uint16_t count) const;
};

}
