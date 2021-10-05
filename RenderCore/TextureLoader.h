#pragma once

#include "RenderCoreRely.h"
#include "Material.h"
#include "TextureUtil/TexMipmap.h"
#include "SystemCommon/AsyncAgent.h"
#include "common/Controllable.hpp"


namespace dizz
{


enum class TexLoadType : uint8_t { Color, Normal };
enum class TexProcType : uint8_t { CompressBC7, CompressBC5, Plain };

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
class RENDERCOREAPI TextureLoader : public common::NonCopyable, public common::NonMovable, public common::Controllable
{
private:
    struct TexProc
    {
        TexProcType Proc;
        bool NeedMipmap;
    };
    std::unique_ptr<common::asyexe::AsyncManager> Compressor;
    std::shared_ptr<oglu::texutil::TexMipmap> MipMapper;
    std::map<u16string, FakeTex> TexCache;
    common::RWSpinLock CacheLock;
    std::map<TexLoadType, TexProc> ProcessMethod;
    common::PromiseResult<FakeTex> LoadImgToFakeTex(const fs::path& picPath, xziar::img::Image&& img, const TexLoadType type, const TexProc proc);
    void RegistControllable();
public:
    using LoadResult = std::variant<FakeTex, common::PromiseResult<FakeTex>>;
    TextureLoader(const std::shared_ptr<oglu::texutil::TexMipmap>& mipmapper);
    ~TextureLoader();
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"TextureLoader"sv;
    }
    LoadResult GetTexureAsync(const fs::path& picPath, const TexLoadType type, const bool async = false);
    LoadResult GetTexureAsync(const fs::path& picPath, xziar::img::Image&& img, const TexLoadType type, const bool async = false);
    void Shrink();
    void SetLoadPreference(const TexLoadType type, const TexProcType proc, const bool mipmap)
    {
        ProcessMethod[type] = TexProc{ proc, mipmap };
    }
};
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


}

