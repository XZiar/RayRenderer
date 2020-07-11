#include "RenderCorePch.h"
#include "RenderElement.h"
#include "OpenGLUtil/PointEnhance.hpp"

#include <boost/uuid/uuid_generators.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace rayr
{
using std::map;
using common::str::Charset;
using oglu::oglTex2D;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;
using namespace std::string_view_literals;
using namespace b3d;


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
            dizzLog().debug(u"Regist Drawable [{}]\n", name);
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
    static boost::uuids::uuid GenerateUUID()
    {
        static boost::uuids::random_generator_mt19937 Generator;
        static common::SpinLocker Lock;
        const auto lock = Lock.LockScope();
        return Generator();
    }
    static boost::uuids::uuid GenerateUUID(const string_view& str)
    {
        static const boost::uuids::string_generator Generator;
        return Generator(str.cbegin(), str.cend());
    }
};


struct VAOPack
{
    const Drawable *drawable;
    const oglu::oglDrawProgram::weak_type prog;
    oglu::oglVAO vao;
};

using VAOKey = boost::multi_index::key<&VAOPack::drawable, &VAOPack::prog>;
using ProgWeakComp = std::owner_less<std::weak_ptr<oglu::oglProgram_>>;
using VAOKeyComp = boost::multi_index::composite_key_compare<std::less<const Drawable*>, ProgWeakComp>;
using VAOMap = boost::multi_index_container<VAOPack, boost::multi_index::indexed_by<
    boost::multi_index::ordered_unique<VAOKey, VAOKeyComp>,
    boost::multi_index::ordered_non_unique<boost::multi_index::key<&VAOPack::prog>, ProgWeakComp>
>>;

struct VAOMAPCtxConfig : public oglu::CtxResConfig<true, VAOMap>
{
    VAOMap Construct() const { return {}; }
};
static VAOMAPCtxConfig VAOMAP_CTX_CFG;
static VAOMap& GetVAOMap(const oglu::oglContext& ctx = oglu::oglContext_::CurrentContext())
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

Drawable::Drawable(const std::type_index type, const u16string& typeName) : Uid(DrawableHelper::GenerateUUID()), DrawableType(type)
{
    DrawableHelper::Regist(DrawableType, typeName);
    RegistControllable();
}

Drawable::~Drawable()
{
    const auto ctx = oglu::oglContext_::CurrentContext();
    if (!ctx) return;
    auto& vaomap = GetVAOMap();
    const auto& its = vaomap.equal_range(this);
    vaomap.erase(its.first, its.second);
}

void Drawable::PrepareMaterial()
{
    MaterialHolder = OnPrepareMaterial();
    MaterialUBO = oglu::oglUniformBuffer_::Create(32 * MultiMaterialHolder::WriteSize);
    MaterialUBO->SetName(u"MaterialData");
}

void Drawable::AssignMaterial()
{
    MaterialHolder.Refresh();
    MaterialHolder.WriteData(MaterialUBO->GetPersistentPtr());
}

void Drawable::Draw(Drawcall& drawcall) const
{
    MaterialHolder.BindTexture(drawcall.Drawer);
    DrawPosition(drawcall)
        .SetUBO(MaterialUBO, "materialBlock")
        .Draw(GetVAO(drawcall.Drawer.GetProg()));
}

u16string Drawable::GetType() const
{
    return DrawableHelper::GetType(DrawableType);
}

void Drawable::ReleaseAll(const oglu::oglDrawProgram& prog)
{
    auto& keyPart = GetVAOMap().get<1>();
    const auto its = keyPart.equal_range(std::weak_ptr(prog));
    keyPart.erase(its.first, its.second);
}

MultiMaterialHolder Drawable::OnPrepareMaterial() const
{
    MultiMaterialHolder holder(1);
    holder[0]->DiffuseMap = MultiMaterialHolder::GetCheckTex();
    holder[0]->UseDiffuseMap = true;
    holder[0]->Albedo = Vec3(0.58, 0.58, 0.58);
    holder[0]->Metalness = 0.1f;
    return holder;
}


auto Drawable::DefaultBind(const oglu::oglDrawProgram& prog, oglu::oglVAO& vao, const oglu::oglVBO& vbo) -> decltype(vao->Prepare(prog))
{
    return std::move(vao->Prepare(prog)
        .SetAttribs<oglu::Point>(vbo, 0, { "@VertPos"sv, "@VertNorm"sv, "@VertTexc"sv })
        .SetDrawId());
}

oglu::ProgDraw& Drawable::DrawPosition(Drawcall& drawcall) const
{
    const auto normMat = Mat3x3::RotateMatXYZ(Rotation);
    const auto modelMat = Mat4x4::TranslateMat(Position) * Mat4x4(normMat * Mat3x3::ScaleMat(Scale));
    const auto mvpMat = drawcall.PVMat * modelMat;
    return drawcall.Drawer
        .SetMat("@MVPMat", mvpMat)
        .SetMat("@ModelMat", modelMat)
        .SetMat("@MVPNormMat", normMat);
}

void Drawable::SetVAO(const oglu::oglDrawProgram& prog, const oglu::oglVAO& vao) const
{
    auto& vaomap = GetVAOMap();
    const auto& it = vaomap.find(std::make_tuple(this, std::weak_ptr(prog)));
    if (it == vaomap.cend())
        vaomap.insert({ this, std::weak_ptr(prog),vao });
    else
        vaomap.modify(it, [&](VAOPack& pack) { pack.vao = vao; });
}

const oglu::oglVAO& Drawable::GetVAO(const oglu::oglDrawProgram::weak_type& weakProg) const
{
    auto& vaomap = GetVAOMap();
    const auto& it = vaomap.find(std::make_tuple(this, weakProg));
    if (it != vaomap.cend())
        return it->vao;
    dizzLog().error(u"No matching VAO found for [{}]({}), maybe prepareGL not executed.\n", Name, GetType());
    return EmptyVAO;
}


void Drawable::RegistControllable()
{
    RegistItem<u16string>("Name", "", u"名称", ArgType::RawValue, {}, u"物体名称")
        .RegistMember(&Drawable::Name);
    RegistItem<bool>("ShouldRender", "", u"启用", ArgType::RawValue, {}, u"是否渲染该物体")
        .RegistMember(&Drawable::ShouldRender);
    RegistItem<miniBLAS::Vec3>("Position", "", u"位置", ArgType::RawValue, {}, u"物体位置")
        .RegistMember<false>(&Drawable::Position);
    RegistItem<miniBLAS::Vec3>("Rotation", "", u"旋转", ArgType::RawValue, {}, u"物体旋转方向")
        .RegistMember<false>(&Drawable::Rotation);
    RegistItem<miniBLAS::Vec3>("Scale", "", u"缩放", ArgType::RawValue, {}, u"物体缩放")
        .RegistMember<false>(&Drawable::Scale);
}

void Drawable::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    using detail::JsonConv;
    jself.Add("Name", common::str::to_u8string(Name));
    jself.Add("Uid", boost::uuids::to_string(Uid));
    jself.Add<JsonConv>(EJ_FIELD(Position));
    jself.Add<JsonConv>(EJ_FIELD(Rotation));
    jself.Add<JsonConv>(EJ_FIELD(Scale));
    context.AddObject(jself, "material", MaterialHolder);
}

void Drawable::Deserialize(DeserializeUtil & context, const xziar::ejson::JObjectRef<true>& object)
{
    using detail::JsonConv;
    Name = common::str::to_u16string(object.Get<string>("Name"), Charset::UTF8);
    Uid = DrawableHelper::GenerateUUID(object.Get<string_view>("Uid"));
    object.TryGet<JsonConv>(EJ_FIELD(Position));
    object.TryGet<JsonConv>(EJ_FIELD(Rotation));
    object.TryGet<JsonConv>(EJ_FIELD(Scale));
    MaterialHolder.Deserialize(context, object.GetObject("material"));
}

}

