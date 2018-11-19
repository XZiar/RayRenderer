#pragma once

#include "RenderCoreRely.h"
#include "Material.h"
#include "TextureUtil/TexMipmap.h"
#include "common/Controllable.hpp"


namespace rayr
{


enum class TexLoadType : uint8_t { Color, Normal };
enum class TexProcType : uint8_t { CompressBC7, CompressBC5, Plain };

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif
class RAYCOREAPI TextureLoader : public NonCopyable, public NonMovable, public common::Controllable
{
private:
    struct TexProc
    {
        TexProcType Proc;
        bool NeedMipmap;
    };
    std::unique_ptr<common::asyexe::AsyncManager> Compressor;
    std::shared_ptr<oglu::texutil::TexMipmap> MipMapper;
    map<u16string, FakeTex> TexCache;
    common::RWSpinLock CacheLock;
    map<TexLoadType, TexProc> ProcessMethod;
    common::PromiseResult<FakeTex> LoadImgToFakeTex(const fs::path& picPath, xziar::img::Image&& img, const TexLoadType type, const TexProc proc);
    void RegistControllable();
public:
    using LoadResult = variant<FakeTex, common::PromiseResult<FakeTex>>;
    TextureLoader(const std::shared_ptr<oglu::texutil::TexMipmap>& mipmapper);
    ~TextureLoader();
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"TextureLoader"sv;
    }
    LoadResult GetTexureAsync(const fs::path& picPath, const TexLoadType type);
    void Shrink();
    void SetLoadPreference(const TexLoadType type, const TexProcType proc, const bool mipmap)
    {
        ProcessMethod[type] = TexProc{ proc, mipmap };
    }
};
#if COMPILER_MSVC
#   pragma warning(pop)
#endif


}

