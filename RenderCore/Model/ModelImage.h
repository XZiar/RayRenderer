#pragma once

#include "../RenderCoreRely.h"
#include "../Material.h"
#include "common/AsyncExecutor/AsyncAgent.h"

namespace rayr
{
class Model;

namespace detail
{
class _ModelMesh;

class ModelImage
{
    friend class ::rayr::Model;
    friend class _ModelMesh;
public:
    using LoadResult = variant<FakeTex, common::PromiseResult<FakeTex>>;
    static std::optional<xziar::img::Image> ReadImage(const fs::path& picPath);
    static LoadResult GetTexureAsync(const fs::path& picPath, const oglu::TextureInnerFormat format = oglu::TextureInnerFormat::BC7SRGB);
    static void Shrink();
};

}

}

