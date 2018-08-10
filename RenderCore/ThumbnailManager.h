#pragma once
#include "RenderCoreRely.h"
#include "Material.h"


namespace oglu::texutil
{
class GLTexResizer;
class CLTexResizer;
}

namespace rayr
{
namespace detail
{

class RAYCOREAPI ThumbnailManager
{
private:
    using Image = xziar::img::Image;
    using PmssType = std::vector<std::pair<TexHolder, common::PromiseResult<Image>>>;
    Wrapper<oglu::texutil::GLTexResizer> GlResizer;
    Wrapper<oglu::texutil::CLTexResizer> ClResizer;
    map<std::weak_ptr<void>, std::shared_ptr<Image>, std::owner_less<void>> ThumbnailMap;
    common::PromiseResult<Image> InnerPrepareThumbnail(const TexHolder& holder);
    void InnerWaitPmss(const PmssType& pmss);
    std::shared_ptr<Image> GetThumbnail(const std::weak_ptr<void>& weakref) const;
public:
    static constexpr std::tuple<bool, uint32_t, uint32_t> CalcSize(const std::pair<uint32_t, uint32_t>& size)
    {
        constexpr uint32_t thredshold = 128;
        const auto larger = std::max(size.first, size.second);
        if (larger <= thredshold)
            return { false, 0,0 };
        return { true, size.first * thredshold / larger, size.second * thredshold / larger };
    }

    ThumbnailManager(const oglu::oglContext& ctx);

    std::shared_ptr<Image> GetThumbnail(const TexHolder& holder) const;

    template<typename T>
    void PrepareThumbnails(const T& container)
    {
        PmssType pmss;
        for (const TexHolder& holder : container)
        {
            auto pms = InnerPrepareThumbnail(holder);
            if(pms)
                pmss.emplace_back(holder, pms);
        }
        InnerWaitPmss(pmss);
    }
};

}
}
