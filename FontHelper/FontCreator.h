#pragma once

#include "FontRely.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oglu
{


class FONTHELPAPI FontCreator : public common::NonCopyable
{
private:
    std::unique_ptr<ft::FreeTyper> ft2;
    oglTex2DD testTex;
    oclu::oclKernel kerSdf, kerSdfGray, kerDownSamp, kerSdfGray4;
    oclu::oclBuffer sq256lut;
    oclu::oclContext clCtx;
    oclu::oclCmdQue clQue;
    void loadCL(const std::string& src);
    void loadDownSampler(const std::string& src);
public:
    FontCreator(const oclu::oclContext ctx);
    ~FontCreator();
    void reloadFont(const common::fs::path& fontpath);
    void reload(const std::string& src);
    oglTex2DD getTexture() const { return testTex; }
    void setChar(char32_t ch) const;
    xziar::img::Image clgraysdfs(char32_t ch, uint32_t count) const;
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
