#pragma once
#include "RenderCoreRely.h"
#include "Material.h"


namespace rayr
{

class RAYCOREAPI ThumbnailManager
{
private:
    using Image = xziar::img::Image;
    using ImageView = xziar::img::ImageView;
    using PmssType = std::vector<std::pair<TexHolder, common::PromiseResult<Image>>>;
    std::shared_ptr<oglu::texutil::TexResizer> Resizer;
    std::shared_ptr<oglu::oglWorker> GLWorker;
    common::RWSpinLock CacheLock;
    std::map<std::weak_ptr<void>, ImageView, std::owner_less<void>> ThumbnailMap;
    common::PromiseResult<std::optional<ImageView>> InnerPrepareThumbnail(const TexHolder& holder);
public:

    ThumbnailManager(const std::shared_ptr<oglu::texutil::TexUtilWorker>& texWorker, const std::shared_ptr<oglu::oglWorker>& glWorker);

    common::PromiseResult<std::optional<ImageView>> GetThumbnail(const TexHolder& holder);

};

}
