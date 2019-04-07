#pragma once

#include "FontRely.h"
#include "common/SharedResource.hpp"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace oglu
{
using common::SharedResource;


class FONTHELPAPI FontCreator : public NonCopyable
{
private:
    Wrapper<ft::FreeTyper> ft2;
    oglTex2DD testTex;
    oclu::oclKernel kerSdf, kerSdfGray, kerDownSamp, kerSdfGray4;
    oclu::oclBuffer sq256lut;
    oclu::oclContext clCtx;
    oclu::oclCmdQue clQue;
    void loadCL(const string& src);
    void loadDownSampler(const string& src);
public:
    FontCreator(const oclu::oclContext ctx);
    ~FontCreator();
    void reloadFont(const fs::path& fontpath);
    void reload(const string& src);
    oglTex2DD getTexture() const { return testTex; }
    void setChar(char32_t ch) const;
    xziar::img::Image clgraysdfs(char32_t ch, uint32_t count) const;
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
