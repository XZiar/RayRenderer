#include "RenderCoreRely.h"
#include "RenderElement.h"
#include "OpenGLUtil/PointEnhance.hpp"

namespace rayr
{
using oglu::oglTex2D;
using xziar::respak::SerializeUtil;

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
    const oglu::oglDrawProgram::weak_type prog;
    oglu::oglVAO vao;
};

using VAOKey = boost::multi_index::composite_key<VAOPack,
    boost::multi_index::member<VAOPack, const Drawable*, &VAOPack::drawable>,
    boost::multi_index::member<VAOPack, const oglu::oglDrawProgram::weak_type, &VAOPack::prog>
>;
using ProgKey = boost::multi_index::member<VAOPack, const oglu::oglDrawProgram::weak_type, &VAOPack::prog>;
using VAOKeyComp = boost::multi_index::composite_key_compare<
    std::less<const Drawable*>, std::owner_less<std::weak_ptr<oglu::detail::_oglProgram>>
>;
using VAOMap = boost::multi_index_container<VAOPack, boost::multi_index::indexed_by<
    boost::multi_index::ordered_unique<VAOKey, VAOKeyComp>,
    boost::multi_index::ordered_non_unique<ProgKey, std::owner_less<std::weak_ptr<oglu::detail::_oglProgram>>>
>>;

struct VAOMAPCtxConfig : public oglu::CtxResConfig<true, VAOMap>
{
    VAOMap Construct() const { return {}; }
};
static VAOMAPCtxConfig VAOMAP_CTX_CFG;
static VAOMap& GetVAOMap(const oglu::oglContext& ctx = oglu::oglContext::CurrentContext())
{
    return ctx->GetOrCreate<false>(VAOMAP_CTX_CFG);
}

struct VAOKeyX
{
    const oglu::oglDrawProgram::weak_type prog;
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
    const auto ctx = oglu::oglContext::CurrentContext();
    if (!ctx) return;
    auto& vaomap = GetVAOMap();
    const auto& its = vaomap.equal_range(this);
    vaomap.erase(its.first, its.second);
}

void Drawable::PrepareMaterial(const std::weak_ptr<detail::ThumbnailManager>& thumbman)
{
    MaterialHolder = PrepareMaterial();
    MaterialHolder.ThumbMan = thumbman;
    MaterialUBO.reset(26 * MultiMaterialHolder::UnitSize);
    MaterialBuf.resize(MaterialUBO->Size());
}

void Drawable::AssignMaterial()
{
    MaterialHolder.Refresh();
    const size_t size = MaterialHolder.WriteData(MaterialBuf.data());
    MaterialUBO->Write(MaterialBuf.data(), size, oglu::BufferWriteMode::StreamDraw);
}

void Drawable::Draw(Drawcall& drawcall) const
{
    MaterialHolder.BindTexture(drawcall);
    DrawPosition(drawcall)
        .SetUBO(MaterialUBO, "materialBlock")
        .Draw(GetVAO(drawcall.GetProg()));
}

u16string Drawable::GetType() const
{
    return DrawableHelper::GetType(DrawableType);
}

void Drawable::ReleaseAll(const oglu::oglDrawProgram& prog)
{
    auto& keyPart = GetVAOMap().get<1>();
    const auto its = keyPart.equal_range(prog.weakRef());
    keyPart.erase(its.first, its.second);
}

MultiMaterialHolder Drawable::PrepareMaterial() const
{
    MultiMaterialHolder holder(1);
    holder[0].DiffuseMap = MultiMaterialHolder::GetCheckTex();
    holder[0].UseDiffuseMap = true;
    holder[0].Albedo = Vec3(0.58, 0.58, 0.58);
    holder[0].Metalness = 0.1f;
    return holder;
}


auto Drawable::DefaultBind(const oglu::oglDrawProgram& prog, oglu::oglVAO& vao, const oglu::oglVBO& vbo) -> decltype(vao->Prepare())
{
    using oglu::Point;
    const GLint attrs[3] = { prog->GetLoc("@VertPos"), prog->GetLoc("@VertNorm"), prog->GetLoc("@VertTexc") };
    return std::move(vao->Prepare().SetAttribs<Point>(vbo, 0, attrs).SetDrawId(prog));
}

Drawable::Drawcall& Drawable::DrawPosition(Drawcall& drawcall) const
{
    Mat3x3 matNormal = Mat3x3::RotateMatXYZ(rotation);
    return drawcall.SetPosition(Mat4x4::TranslateMat(position) * Mat4x4(matNormal * Mat3x3::ScaleMat(scale)), matNormal);
}

void Drawable::SetVAO(const oglu::oglDrawProgram& prog, const oglu::oglVAO& vao) const
{
    auto& vaomap = GetVAOMap();
    const auto& it = vaomap.find(std::make_tuple(this, prog.weakRef()));
    if (it == vaomap.cend())
        vaomap.insert({ this, prog.weakRef(),vao });
    else
        vaomap.modify(it, [&](VAOPack& pack) { pack.vao = vao; });
}

const oglu::oglVAO& Drawable::GetVAO(const oglu::oglDrawProgram::weak_type& weakProg) const
{
    auto& vaomap = GetVAOMap();
    const auto& it = vaomap.find(std::make_tuple(this, weakProg));
    if (it != vaomap.cend())
        return it->vao;
    basLog().error(u"No matching VAO found for [{}]({}), maybe prepareGL not executed.\n", Name, GetType());
    return EmptyVAO;
}


ejson::JObject Drawable::Serialize(SerializeUtil & context) const
{
    auto jself = context.NewObject();
    jself.Add("name", str::to_u8string(Name, Charset::UTF16LE));
    jself.Add("position", detail::ToJArray(context, position));
    jself.Add("rotation", detail::ToJArray(context, rotation));
    jself.Add("scale", detail::ToJArray(context, scale));
    context.AddObject(jself, "material", MaterialHolder);
    return jself;
}

void Drawable::Deserialize(DeserializeUtil & context, const ejson::JObjectRef<true>& object)
{
    Name = str::to_u16string(object.Get<string>("name"), Charset::UTF8);
    detail::FromJArray(object.GetArray("position"), position);
    detail::FromJArray(object.GetArray("rotation"), rotation);
    detail::FromJArray(object.GetArray("scale"), scale);
    MaterialHolder.Deserialize(context, object.GetObject("material"));
}

}

