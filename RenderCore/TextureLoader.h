#pragma once

#include "RenderCoreRely.h"
#include "Material.h"
#include "TextureUtil/TexMipmap.h"
#include "common/AsyncExecutor/AsyncManager.h"


namespace rayr
{

namespace detail
{

class TextureLoader : public NonCopyable, public NonMovable
{
private:
    map<u16string, FakeTex> TEX_CACHE;
    common::asyexe::AsyncManager Compressor;
    std::shared_ptr<oglu::texutil::TexMipmap> MipMapper;
    common::PromiseResult<FakeTex> LoadImgToFakeTex(const fs::path& picPath, xziar::img::Image&& img, const oglu::TextureInnerFormat format);
public:
    using LoadResult = variant<FakeTex, common::PromiseResult<FakeTex>>;
    TextureLoader(const std::shared_ptr<oglu::texutil::TexMipmap>& mipmapper);
    ~TextureLoader();
    std::optional<xziar::img::Image> ReadImage(const fs::path& picPath);
    LoadResult GetTexureAsync(const fs::path& picPath, const oglu::TextureInnerFormat format = oglu::TextureInnerFormat::BC7SRGB);
    void Shrink();
};

}

}

