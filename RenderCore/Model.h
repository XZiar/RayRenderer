#pragma once

#include "RenderElement.h"
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
    oglu::oglTexture genTexture(const oglu::TextureInnerFormat format = oglu::TextureInnerFormat::BC3);
    oglu::oglTexture genTextureAsync(const common::asyexe::AsyncAgent& agent, const oglu::TextureInnerFormat format = oglu::TextureInnerFormat::BC3);
};
using ModelImage = Wrapper<_ModelImage>;

class alignas(Vec3) _ModelData : public NonCopyable, public AlignBase<alignof(Vec3)>
{
    friend class ::rayr::Model;
private:
    static map<u16string, Wrapper<_ModelData>> models;
    static Wrapper<_ModelData> getModel(const u16string& fname, bool asyncload = false);
    static void releaseModel(const u16string& fname);
    struct alignas(MaterialData) MtlStub
    {
        MaterialData mtl;
        float scalex, offsetx, scaley, offsety;
        uint16_t width = 0, height = 0, sx = 0, sy = 0, posid = UINT16_MAX;
        ModelImage texs[2];
        ModelImage& diffuse() { return texs[0]; }
        const ModelImage& diffuse() const { return texs[0]; }
        ModelImage& normal() { return texs[1]; }
        const ModelImage& normal() const { return texs[1]; }
        bool hasImage(const ModelImage& img) const
        {
            for (const auto& tex : texs)
                if (tex == img)
                    return true;
            return false;
        }
    };
public:
    Vec3 size;
private:
    using TexMergeItem = std::tuple<ModelImage, uint16_t, uint16_t>;
    vectorEx<PointEx> pts;
    vector<uint32_t> indexs;
    vector<std::pair<string, uint32_t>> groups;
    ModelImage diffuse, normal;
    oglu::oglTexture texd, texn;
    oglu::oglBuffer vbo;
    oglu::oglEBO ebo;
    const u16string mfname;
    std::tuple<ModelImage, ModelImage> mergeTex(map<string, MtlStub>& mtlmap, vector<TexMergeItem>& texposs);
    map<string, MtlStub> loadMTL(const fs::path& mtlfname);
    void loadOBJ(const fs::path& objfname);
    void initData();
    void initDataAsync(const common::asyexe::AsyncAgent& agent);
    _ModelData(const u16string& fname, bool asyncload = false);
public:
    ~_ModelData();
    oglu::oglVAO getVAO() const;
};

}

using ModelData = Wrapper<detail::_ModelData>;

class alignas(16) Model : public Drawable
{
protected:
public:
    static constexpr auto TYPENAME = u"Model";
    ModelData data;
    Model(const u16string& fname, bool asyncload = false);
    ~Model() override;
    virtual void prepareGL(const oglu::oglProgram& prog, const map<string, string>& translator = map<string, string>()) override;
    virtual void draw(Drawcall& drawcall) const override;
};

}