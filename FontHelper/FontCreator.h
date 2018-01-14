#pragma once

#include "FontRely.h"
#include "common/SharedResource.hpp"

namespace oglu
{
using common::SharedResource;


class FONTHELPAPI FontCreator : public NonCopyable
{
private:
	ft::FreeTyper ft2;
	oglTexture testTex;
	oclu::oclKernel kerSdf, kerSdfGray;
	oclu::oclBuffer sqlut, sq256lut;
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
	void clbmpsdf(wchar_t ch) const;
	void clbmpsdfgray(wchar_t ch) const;
	void clbmpsdfs(wchar_t ch, uint16_t count) const;
	xziar::img::Image clgraysdfs(wchar_t ch, uint16_t count) const;
};

}
