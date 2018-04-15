#pragma once

#include "../RenderCoreRely.h"
#include "common/AsyncExecutor/AsyncAgent.h"

namespace rayr
{
using oglu::oglTex2DS;
using oglu::oglTex2DV;
class Model;

namespace detail
{
class _ModelMesh;

class ModelImage
{
    friend class ::rayr::Model;
    friend class _ModelMesh;
private:
    static oglTex2DV GetTexure(const fs::path& picPath, const xziar::img::Image& img);
public:
    using LoadResult = variant<oglTex2DV, common::PromiseResult<oglTex2DV>>;
    static std::optional<xziar::img::Image> ReadImage(const fs::path& picPath);
    static oglTex2DV GetTexure(const fs::path& picPath);
    static LoadResult GetTexureAsync(const fs::path& picPath);
    static void Shrink();
};

}

}

