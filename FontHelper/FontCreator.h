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
	oclu::oclKernel kerSdf, kerSdfGray, kerDownSamp;
	oclu::oclBuffer sq256lut, infoBuf, inputBuf, middleBuf, outputBuf;
	static SharedResource<oclu::oclContext> clRes;
	oclu::oclContext clCtx;
	oclu::oclCmdQue clQue;
	void loadCL(const string& src);
	void loadDownSampler(const string& src);
public:
	FontCreator(const fs::path& fontpath);
	~FontCreator();
	void reload(const string& src);
	oglTexture getTexture() const { return testTex; }
	void setChar(char32_t ch, bool custom) const;
	void clbmpsdfs(char32_t ch, uint32_t count) const;
	xziar::img::Image clgraysdfs(char32_t ch, uint32_t count) const;
};

}
