#include "RenderCoreRely.h"
#include "RenderElement.h"

namespace rayr
{


struct DrawableHelper
{
    friend class Drawable;
private:
    static inline common::RWSpinLock Locker;
    static inline map<std::type_index, u16string> TypeMap;
    static void Regist(const std::type_index& id, const u16string& name)
    {
        Locker.LockWrite();
        const auto isAdd = TypeMap.try_emplace(id, name).second;
        if(isAdd)
            basLog().debug(u"Regist Drawable [{}]\n", name);
        Locker.UnlockWrite();
    }
    static u16string GetType(const std::type_index& obj)
    {
        Locker.LockRead();
        const auto name = common::container::FindInMapOrDefault(TypeMap, obj);
        Locker.UnlockRead();
        return name;
    }
    static std::type_index Get(const Drawable* const obj) { return std::type_index(typeid(obj)); }
};


struct VAOPack
{
    const Drawable *drawable;
    const oglu::oglProgram::weak_type prog;
    oglu::oglVAO vao;
};

using VAOKey = boost::multi_index::composite_key<VAOPack,
    boost::multi_index::member<VAOPack, const Drawable*, &VAOPack::drawable>,
    boost::multi_index::member<VAOPack, const oglu::oglProgram::weak_type, &VAOPack::prog>
>;
using ProgKey = boost::multi_index::member<VAOPack, const oglu::oglProgram::weak_type, &VAOPack::prog>;
using VAOKeyComp = boost::multi_index::composite_key_compare<
    std::less<const Drawable*>, std::owner_less<std::weak_ptr<oglu::detail::_oglProgram>>
>;
using VAOMap = boost::multi_index_container<VAOPack, boost::multi_index::indexed_by<
    boost::multi_index::ordered_unique<VAOKey, VAOKeyComp>,
    boost::multi_index::ordered_non_unique<ProgKey, std::owner_less<std::weak_ptr<oglu::detail::_oglProgram>>>
>>;

oglu::detail::ContextResource<std::shared_ptr<VAOMap>, false> CTX_VAO_MAP;


struct VAOKeyX
{
    const oglu::oglProgram::weak_type prog;
    const Drawable *drawable;
    bool operator<(const VAOKeyX& other)
    {
        if (prog.owner_before(other.prog))
            return true;
        if (other.prog.owner_before(prog))
            return false;
        //same prog
        return drawable < other.drawable;
    }
};

Drawable::Drawable(const std::type_index type, const u16string& typeName) : DrawableType(type)
{
    DrawableHelper::Regist(DrawableType, typeName);
}

Drawable::~Drawable()
{
    if (auto vaomap = CTX_VAO_MAP.TryGet())
    {
        const auto& its = (*vaomap)->equal_range(this);
        (*vaomap)->erase(its.first, its.second);
    }
}

void Drawable::PrepareMaterial()
{
    MaterialUBO.reset(16 * sizeof(MaterialData));
    BaseMaterial.Albedo = Vec3(0.58, 0.58, 0.58);
    BaseMaterial.Metalness = 0.1f;
    AssignMaterial(&BaseMaterial, 1);
}

void Drawable::AssignMaterial(const PBRMaterial * material, const size_t count) const
{
    vector<byte> data(MaterialUBO->Size());
    size_t pos = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        pos += material->WriteData(&data[pos]);
        if (pos >= MaterialUBO->Size())
            break;
    }
    MaterialUBO->Write(data.data(), pos, oglu::BufferWriteMode::StreamDraw);
}

void Drawable::Draw(Drawcall& drawcall) const
{
    DrawPosition(drawcall)
        .SetUBO(MaterialUBO, "materialBlock")
        .Draw(GetVAO(drawcall.GetProg()));
}

u16string Drawable::GetType() const
{
    return DrawableHelper::GetType(DrawableType);
}

void Drawable::ReleaseAll(const oglu::oglProgram& prog)
{
    if (auto vaomap = CTX_VAO_MAP.TryGet())
    {
        auto& keyPart = (*vaomap)->get<1>();
        const auto its = keyPart.equal_range(prog.weakRef());
        keyPart.erase(its.first, its.second);
    }
}

auto Drawable::DefaultBind(const oglu::oglProgram& prog, oglu::oglVAO& vao, const oglu::oglVBO& vbo) -> decltype(vao->Prepare())
{
    const GLint attrs[3] = { prog->Attr_Vert_Pos, prog->Attr_Vert_Norm, prog->Attr_Vert_Texc };
    return std::move(vao->Prepare().Set(vbo, attrs, 0));
}

Drawable::Drawcall& Drawable::DrawPosition(Drawcall& drawcall) const
{
    Mat3x3 matNormal = Mat3x3::RotateMatXYZ(rotation);
    return drawcall.SetPosition(Mat4x4::TranslateMat(position) * Mat4x4(matNormal * Mat3x3::ScaleMat(scale)), matNormal);
}

void Drawable::SetVAO(const oglu::oglProgram& prog, const oglu::oglVAO& vao) const
{
    auto vaomap = CTX_VAO_MAP.GetOrInsert([](const auto& dummy) { return std::make_shared<VAOMap>(); });
    const auto& it = vaomap->find(std::make_tuple(this, prog.weakRef()));
    if (it == vaomap->cend())
        vaomap->insert({ this, prog.weakRef(),vao });
    else
        vaomap->modify(it, [&](VAOPack& pack) { pack.vao = vao; });
}

const oglu::oglVAO& Drawable::GetVAO(const oglu::oglProgram::weak_type& weakProg) const
{
    if (auto vaomap = CTX_VAO_MAP.TryGet())
    {
        const auto& it = (*vaomap)->find(std::make_tuple(this, weakProg));
        if (it != (*vaomap)->cend())
            return it->vao;
    }
    basLog().error(u"No matching VAO found for [{}]({}), maybe prepareGL not executed.\n", Name, GetType());
    return EmptyVAO;
}


}

