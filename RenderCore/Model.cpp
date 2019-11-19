#include "RenderCorePch.h"
#include "Model.h"


namespace rayr
{
using std::map;
using common::container::FindInMap;
using common::asyexe::AsyncAgent;
using b3d::Vec3;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;


MultiMaterialHolder Model::OnPrepareMaterial() const
{
    MultiMaterialHolder holder((uint8_t)Mesh->groups.size());
    uint8_t i = 0;
    for (const auto& group : Mesh->groups)
    {
        if (const auto mat = FindInMap(Mesh->MaterialMap, group.first))
            holder[i] = std::make_shared<PBRMaterial>(*mat);
        ++i;
    }
    return holder;
}

Model::Model(ModelMesh mesh) : Drawable(this, TYPENAME), Mesh(mesh)
{
    const auto resizer = 2 / common::max(common::max(Mesh->size.x, Mesh->size.y), Mesh->size.z);
    Scale = Vec3(resizer, resizer, resizer);
}

Model::Model(const u16string& fname, const std::shared_ptr<TextureLoader>& texLoader, const std::shared_ptr<oglu::oglWorker>& asyncer)
    : Model(detail::_ModelMesh::GetModel(fname, texLoader, asyncer)) {}

Model::~Model()
{
    const auto mfname = Mesh->mfname;
    Mesh.reset();
    detail::_ModelMesh::ReleaseModel(mfname);
}

void Model::PrepareGL(const oglu::oglDrawProgram& prog, const map<string, string>&)
{
    using oglu::PointEx;
    auto vao = oglu::oglVAO_::Create(oglu::VAODrawMode::Triangles);
    const oglu::GLint attrs[4] = { prog->GetLoc("@VertPos"), prog->GetLoc("@VertNorm"), prog->GetLoc("@VertTexc"), prog->GetLoc("@VertTan") };
    {
        auto vaoprep = std::move(vao->Prepare()
            .SetAttribs<PointEx>(Mesh->vbo, 0, attrs)
            .SetDrawId(prog)
            .SetIndex(Mesh->ebo));
        Mesh->PrepareVAO(vaoprep);
    }
    SetVAO(prog, vao);
    vao->Test();
}

void Model::Draw(Drawcall& drawcall) const
{
    MaterialHolder.BindTexture(drawcall.Drawer);
    DrawPosition(drawcall)
        .SetUBO(MaterialUBO, "materialBlock")
        .Draw(GetVAO(drawcall.Drawer.GetProg()));
}

void Model::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    Drawable::Serialize(context, jself);
    jself.Add("mesh", context.AddObject(*Mesh));
}
void Model::Deserialize(DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object)
{
    Drawable::Deserialize(context, object);
}

RESPAK_IMPL_COMP_DESERIALIZE(Model, ModelMesh)
{
    return std::any(std::tuple(detail::_ModelMesh::GetModel(context, object.Get<string>("mesh"))));
}

}