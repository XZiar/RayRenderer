#pragma once

#include "RenderElement.h"
#include "Material.h"
#include "common/AsyncExecutor/AsyncAgent.h"
#include "Model/ModelImage.h"

namespace rayr
{
class Model;


namespace detail
{
class alignas(Vec3) _ModelData : public NonCopyable, public AlignBase<alignof(Vec3)>
{
    friend class ::rayr::Model;
private:
    static map<u16string, Wrapper<_ModelData>> models;
    static Wrapper<_ModelData> getModel(const u16string& fname, bool asyncload = false);
    static void releaseModel(const u16string& fname);
    struct alignas(RawMaterialData) MtlStub
    {
        RawMaterialData mtl;
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
    oglu::oglTex2DS texd, texn;
    oglu::oglVBO vbo;
    oglu::oglEBO ebo;
    oglu::oglIBO ibo;
    const u16string mfname;
    std::tuple<ModelImage, ModelImage> mergeTex(map<string, MtlStub>& mtlmap, vector<TexMergeItem>& texposs);
    map<string, MtlStub> loadMTL(const fs::path& mtlfname);
    void loadOBJ(const fs::path& objfname);
    void InitDataBuffers();
    void initData(const bool asyncload);
    _ModelData(const u16string& fname, bool asyncload = false);
public:
    ~_ModelData();
    void PrepareVAO(oglu::detail::_oglVAO::VAOPrep& vaoPrep) const;
};
}
using ModelData = Wrapper<detail::_ModelData>;


class alignas(16) Model : public Drawable
{
protected:
    void InitMaterial();
public:
    static constexpr auto TYPENAME = u"Model";
    ModelData data;
    Model(const u16string& fname, bool asyncload = false);
    ~Model() override;
    virtual void PrepareGL(const oglu::oglProgram& prog, const map<string, string>& translator = map<string, string>()) override;
    virtual void Draw(Drawcall& drawcall) const override;
};

}