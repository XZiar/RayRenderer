#pragma once

#include "FontRely.h"
#include "common/SharedResource.hpp"

namespace oglu
{
using common::SharedResource;


class FONTHELPAPI FontCreator : public NonCopyable
{
private:
    Wrapper<ft::FreeTyper> ft2;
    oglTex2D testTex;
    oclu::oclKernel kerSdf, kerSdfGray, kerDownSamp, kerSdfGray4;
    oclu::oclBuffer sq256lut, infoBuf, inputBuf, middleBuf, outputBuf;
    oclu::oclContext clCtx;
    oclu::oclCmdQue clQue;
    void loadCL(const string& src);
    void loadDownSampler(const string& src);
public:
    FontCreator(const oclu::Vendor preferredVendor = oclu::Vendor::NVIDIA);
    ~FontCreator();
    void reloadFont(const fs::path& fontpath);
    void reload(const string& src);
    oglTex2D getTexture() const { return testTex; }
    void setChar(char32_t ch, bool custom) const;
    xziar::img::Image clgraysdfs(char32_t ch, uint32_t count) const;
};

}
