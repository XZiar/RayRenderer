#pragma once

#include "../RenderElement.h"
#include "common/AsyncExecutor/AsyncAgent.h"

namespace rayr
{
class Model;

namespace detail
{
namespace img = xziar::img;
using xziar::img::Image;
using xziar::img::ImageDataType;
namespace fs = std::experimental::filesystem;

class _ModelImage : public Image
{
    friend class ::rayr::Model;
    friend class _ModelData;
private:
    static map<u16string, Wrapper<_ModelImage>> images;
    static Wrapper<_ModelImage> getImage(fs::path picPath, const fs::path& curPath);
    static Wrapper<_ModelImage> getImage(const u16string& pname);
    static void shrink();

    _ModelImage(const u16string& pfname);
    void CompressData(vector<uint8_t>& output, const oglu::TextureInnerFormat format);
public:
    u16string Name;
    _ModelImage(const uint16_t w, const uint16_t h, const uint32_t color = 0x0);
    oglu::oglTex2DS genTexture(const oglu::TextureInnerFormat format = oglu::TextureInnerFormat::BC3);
    oglu::oglTex2DS genTextureAsync(const oglu::TextureInnerFormat format = oglu::TextureInnerFormat::BC3);
};
}
using ModelImage = Wrapper<detail::_ModelImage>;



}

