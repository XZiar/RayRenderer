#pragma once

#include "../RenderCoreRely.h"
#include "../Material.h"
#include "OpenGLUtil/PointEnhance.hpp"

namespace dizz
{
class Model;


namespace detail
{

class alignas(b3d::Vec3)_ModelMesh : public common::NonCopyable, public xziar::respak::Serializable
{
    friend class ::dizz::Model;
private:
    static std::shared_ptr<_ModelMesh> GetModel(xziar::respak::DeserializeUtil& context, const string& id);
    static std::shared_ptr<_ModelMesh> GetModel(const u16string& fname, const std::shared_ptr<TextureLoader>& texLoader, const std::shared_ptr<oglu::oglWorker>& asyncer = {});
    static void ReleaseModel(const u16string& fname);
public:
    b3d::Vec3 size;
private:
    std::vector<oglu::PointEx> pts;
    std::vector<uint32_t> indexs;
    std::vector<std::pair<string, uint32_t>> groups;
    std::map<string, PBRMaterial> MaterialMap;
    oglu::oglVBO vbo;
    oglu::oglEBO ebo;
    oglu::oglIBO ibo;
    const u16string mfname;
    void loadOBJ(const fs::path& objfname, const std::shared_ptr<TextureLoader>& texLoader);
    void InitDataBuffers(const std::shared_ptr<oglu::oglWorker>& asyncer = {});
    _ModelMesh(const u16string& fname);
    _ModelMesh(const u16string& fname, const std::shared_ptr<TextureLoader>& texLoader, const std::shared_ptr<oglu::oglWorker>& asyncer = {});
public:
    void PrepareVAO(oglu::oglVAO_::VAOPrep& vaoPrep) const;

    RESPAK_DECL_COMP_DESERIALIZE("dizz#ModelMesh")
    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;
};

}
using ModelMesh = std::shared_ptr<detail::_ModelMesh>;

}
