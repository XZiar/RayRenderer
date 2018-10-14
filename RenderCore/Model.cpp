#include "RenderCoreRely.h"
#include "Model.h"
#include "OpenGLUtil/oglWorker.h"


namespace rayr
{
using common::container::FindInMap;
using common::asyexe::AsyncAgent;


MultiMaterialHolder Model::PrepareMaterial() const
{
    MultiMaterialHolder holder((uint8_t)Mesh->groups.size());
    uint8_t i = 0;
    for (const auto& group : Mesh->groups)
    {
        if (const auto mat = FindInMap(Mesh->MaterialMap, group.first))
            holder[i] = *mat;
        ++i;
    }

    return holder;
}

Model::Model(ModelMesh mesh, const Wrapper<oglu::oglWorker>& asyncer) 
    : Drawable(this, TYPENAME), Mesh(mesh)
{
    const auto resizer = 2 / max(max(Mesh->size.x, Mesh->size.y), Mesh->size.z);
    scale = Vec3(resizer, resizer, resizer);
    if (asyncer)
    {
        const auto task = asyncer->InvokeShare([&](const common::asyexe::AsyncAgent& agent)
        {
            agent.Await(oglu::oglUtil::SyncGL());
        });
        AsyncAgent::SafeWait(task);
    }
}

Model::Model(const u16string& fname, const std::shared_ptr<detail::TextureLoader>& texLoader, const Wrapper<oglu::oglWorker>& asyncer)
    : Model(detail::_ModelMesh::GetModel(fname, texLoader, asyncer), asyncer) {}

Model::~Model()
{
    const auto mfname = Mesh->mfname;
    Mesh.release();
    detail::_ModelMesh::ReleaseModel(mfname);
}

void Model::PrepareGL(const oglu::oglDrawProgram& prog, const map<string, string>& translator)
{
    using oglu::PointEx;
    oglu::oglVAO vao(oglu::VAODrawMode::Triangles);
    const GLint attrs[4] = { prog->GetLoc("@VertPos"), prog->GetLoc("@VertNorm"), prog->GetLoc("@VertTexc"), prog->GetLoc("@VertTan") };
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
    MaterialHolder.BindTexture(drawcall);
    DrawPosition(drawcall)
        .SetUBO(MaterialUBO, "materialBlock")
        .Draw(GetVAO(drawcall.GetProg()));
}

ejson::JObject Model::Serialize(SerializeUtil& context) const
{
    auto jself = Drawable::Serialize(context);
    jself.Add("mesh", context.AddObject(*Mesh));
    return jself;
}
void Model::Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object)
{
    Drawable::Deserialize(context, object);
}

RESPAK_IMPL_COMP_DESERIALIZE(Model, u16string, std::shared_ptr<detail::TextureLoader>, Wrapper<oglu::oglWorker>)
{
    return std::any{};
}

}