#include "RenderCorePch.h"
#include "Material.h"

namespace rayr
{
using std::set;
using std::map;
using std::vector;
using common::str::Charset;
using b3d::Vec4;
using oglu::oglTex2D;
using oglu::oglTex2DArray;
using xziar::img::TextureFormat;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;


namespace detail
{
RESPAK_IMPL_COMP_DESERIALIZE(_FakeTex, std::vector<common::AlignedBuffer>, xziar::img::TextureFormat, uint32_t, uint32_t)
{
    const auto format = (xziar::img::TextureFormat)object.Get<uint16_t>("format");
    const auto width = object.Get<uint32_t>("width");
    const auto height = object.Get<uint32_t>("height");
    //const auto mipmap = object.Get<uint8_t>("mipmap");
    std::vector<common::AlignedBuffer> data;
    
    const auto jdata = object.GetArray("data");
    for (const auto ele : jdata)
    {
        const auto srchandle = ele.AsValue<string>();
        data.push_back(context.GetResource(srchandle));
    }

    return std::tuple(std::move(data), format, width, height);
}
void _FakeTex::Serialize(SerializeUtil&, xziar::ejson::JObject&) const
{
}
void _FakeTex::Deserialize(DeserializeUtil&, const xziar::ejson::JObjectRef<true>& object)
{
    Name = common::strchset::to_u16string(object.Get<string>("name"), Charset::UTF8);
}
}

xziar::img::TextureFormat TexHolder::GetInnerFormat() const
{
    switch (index())
    {
    case 1: return std::get<oglTex2D>(*this)->GetInnerFormat();
    case 2: return std::get<FakeTex>(*this)->TexFormat;
    default: return xziar::img::TextureFormat::ERROR;
    }
}
u16string TexHolder::GetName() const
{
    switch (index())
    {
    case 1: return std::get<oglTex2D>(*this)->Name;
    case 2: return std::get<FakeTex>(*this)->Name;
    default: return u"";
    }
}
std::pair<uint32_t, uint32_t> TexHolder::GetSize() const
{
    switch (index())
    {
    case 1: return std::get<oglTex2D>(*this)->GetSize();
    case 2: {const auto& ft = std::get<FakeTex>(*this); return { ft->Width, ft->Height }; };
    default: return { 0,0 };
    }
}
std::weak_ptr<void> TexHolder::GetWeakRef() const
{
    switch (index())
    {
    case 1: return std::get<oglTex2D>(*this);
    case 2: return std::weak_ptr<detail::_FakeTex>(std::get<FakeTex>(*this));
    default: return {};
    }
}
uint8_t TexHolder::GetMipmapCount() const
{
    switch (index())
    {
    case 1: return std::get<oglTex2D>(*this)->GetMipmapCount();
    case 2: return static_cast<uint8_t>(std::get<FakeTex>(*this)->TexData.size());
    default: return 0;
    }
}
uintptr_t TexHolder::GetRawPtr() const
{
    switch (index())
    {
    case 1: return reinterpret_cast<uintptr_t>(std::get<oglTex2D>(*this).get());
    case 2: return reinterpret_cast<uintptr_t>(std::get<FakeTex>(*this).get());
    default: return 0;
    }
}


PBRMaterial::PBRMaterial(const std::u16string& name) 
    : Albedo(0.58, 0.58, 0.58), Metalness(0.0f), Roughness(0.8f), Specular(0.0f), AO(1.0f), Name(name)
{
    RegistControllable();
}

//uint32_t PBRMaterial::WriteData(std::byte *ptr) const
//{
//    float *ptrFloat = reinterpret_cast<float*>(ptr);
//    Vec4 basic(Albedo, Metalness);
//    if (UseDiffuseMap)
//        basic.x *= -1.0f;
//    if (UseNormalMap)
//        basic.y *= -1.0f;
//    basic.save(ptrFloat);
//    ptrFloat[4] = Roughness;
//    ptrFloat[5] = Specular;
//    ptrFloat[6] = AO;
//    return 8 * sizeof(float);
//}


void PBRMaterial::RegistControllable()
{
    RegistItem<u16string>("Name", "", u"名称", ArgType::RawValue, {}, u"材质名称")
        .RegistMember(&PBRMaterial::Name);
    RegistItem<bool>("UseDiffuseMap", "", u"Albedo贴图", ArgType::RawValue, {}, u"是否启用Albedo贴图")
        .RegistMember(&PBRMaterial::UseDiffuseMap);
    RegistItem<bool>("UseNormalMap", "", u"法线贴图", ArgType::RawValue, {}, u"是否启用法线贴图")
        .RegistMember(&PBRMaterial::UseNormalMap);
    RegistItem<bool>("UseMetalMap", "", u"金属度贴图", ArgType::RawValue, {}, u"是否启用金属度贴图")
        .RegistMember(&PBRMaterial::UseMetalMap);
    RegistItem<bool>("UseRoughMap", "", u"粗糙度贴图", ArgType::RawValue, {}, u"是否启用粗糙度贴图")
        .RegistMember(&PBRMaterial::UseRoughMap);
    RegistItem<bool>("UseAOMap", "", u"AO贴图", ArgType::RawValue, {}, u"是否启用AO贴图")
        .RegistMember(&PBRMaterial::UseAOMap);
    RegistItem<miniBLAS::Vec3>("Color", "", u"颜色", ArgType::Color, {}, u"Albedo颜色")
        .RegistMember(&PBRMaterial::Albedo);
    RegistItem<float>("Metalness", "", u"金属度", ArgType::RawValue, std::pair(0.f, 1.f), u"全局金属度")
        .RegistMember(&PBRMaterial::Metalness);
    RegistItem<float>("Roughness", "", u"粗糙度", ArgType::RawValue, std::pair(0.f, 1.f), u"全局粗糙度")
        .RegistMember(&PBRMaterial::Roughness);
    RegistItem<float>("AO", "", u"环境遮蔽", ArgType::RawValue, std::pair(0.f, 1.f), u"全局环境遮蔽")
        .RegistMember(&PBRMaterial::AO);
}

static std::optional<string> SerializeTex(const TexHolder& holder, SerializeUtil & context)
{
    if (holder.index() != 1 && holder.index() != 2)
        return {};
    auto jtex = context.NewObject();
    jtex.Add("name", common::strchset::to_u8string(holder.GetName(), Charset::UTF16LE));
    jtex.Add("format", (uint16_t)holder.GetInnerFormat());
    const auto[w, h] = holder.GetSize();
    jtex.Add("width", w);
    jtex.Add("height", h);
    jtex.Add("mipmap", holder.GetMipmapCount());
    auto jdataarr = context.NewArray();
    for (uint8_t i = 0; i < holder.GetMipmapCount(); ++i)
    {
        switch (holder.index())
        {
        case 1:
            {
                const auto& tex = std::get<oglTex2D>(holder);
                std::vector<uint8_t> data;
                if (tex->IsCompressed())
                    data = tex->GetCompressedData(i).value();
                else
                    data = tex->GetData(tex->GetInnerFormat(), i);
                const auto datahandle = context.PutResource(data.data(), data.size());
                jdataarr.Push(datahandle);
            } break;
        case 2:
            {
                const auto& tex = std::get<FakeTex>(holder);
                const auto datahandle = context.PutResource(tex->TexData[i].GetRawPtr(), tex->TexData[i].GetSize());
                jdataarr.Push(datahandle);
            } break;
        default: break; // should not enter
        }
    }
    jtex.Add("data", jdataarr);
    jtex.Add("#Type", detail::_FakeTex::SERIALIZE_TYPE);
    return context.AddObject(std::move(jtex), std::to_string(holder.GetRawPtr()));
}
static TexHolder DeserializeTex(DeserializeUtil& context, const string_view& value)
{
    if (value.empty())
        return {};
    return context.DeserializeShare<detail::_FakeTex>(value);
}
void PBRMaterial::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    jself.Add("name", common::strchset::to_u8string(Name, Charset::UTF16LE));
    jself.Add("albedo", detail::ToJArray(context, Albedo));
    jself.Add(EJ_FIELD(Metalness))
         .Add(EJ_FIELD(Roughness))
         .Add(EJ_FIELD(Specular))
         .Add(EJ_FIELD(AO))
         .Add(EJ_FIELD(UseDiffuseMap))
         .Add(EJ_FIELD(UseNormalMap))
         .Add(EJ_FIELD(UseMetalMap))
         .Add(EJ_FIELD(UseRoughMap))
         .Add(EJ_FIELD(UseAOMap));
    jself.Add("DiffuseMap", SerializeTex(DiffuseMap, context));
    jself.Add("NormalMap",  SerializeTex(NormalMap,  context));
    jself.Add("MetalMap",   SerializeTex(MetalMap,   context));
    jself.Add("RoughMap",   SerializeTex(RoughMap,   context));
    jself.Add("AOMap",      SerializeTex(AOMap,      context));
}
void PBRMaterial::Deserialize(DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object)
{
    Name = common::strchset::to_u16string(object.Get<string>("name"), Charset::UTF8);
    detail::FromJArray(object.GetArray("albedo"), Albedo);
    object.TryGet(EJ_FIELD(Metalness));
    object.TryGet(EJ_FIELD(Roughness));
    object.TryGet(EJ_FIELD(Specular));
    object.TryGet(EJ_FIELD(AO));
    object.TryGet(EJ_FIELD(UseDiffuseMap));
    object.TryGet(EJ_FIELD(UseNormalMap));
    object.TryGet(EJ_FIELD(UseMetalMap));
    object.TryGet(EJ_FIELD(UseRoughMap));
    object.TryGet(EJ_FIELD(UseAOMap));
    DiffuseMap = DeserializeTex(context, object.Get<string_view>("DiffuseMap"));
    NormalMap  = DeserializeTex(context, object.Get<string_view>("NormalMap"));
    MetalMap   = DeserializeTex(context, object.Get<string_view>("MetalMap"));
    RoughMap   = DeserializeTex(context, object.Get<string_view>("RoughMap"));
    AOMap      = DeserializeTex(context, object.Get<string_view>("AOMap"));
}
RESPAK_IMPL_SIMP_DESERIALIZE(PBRMaterial)

struct CheckTexCtxConfig : public oglu::CtxResConfig<true, oglu::oglTex2DV>
{
    oglu::oglTex2DV Construct() const 
    { 
        auto chkTex = oglu::oglTex2DStatic_::Create(128, 128, xziar::img::TextureFormat::SRGBA8);
        std::array<uint32_t, 128 * 128> pixs{};
        for (uint32_t a = 0, idx = 0; a < 128; ++a)
        {
            for (uint32_t b = 0; b < 128; ++b)
            {
                pixs[idx++] = ((a / 32) & 0x1) == ((b / 32) & 0x1) ? 0xff0f0f0fu : 0xffa0a0a0u;
            }
        }
        chkTex->SetData(xziar::img::TextureFormat::RGBA8, pixs.data());
        const auto texv = chkTex->GetTextureView();
        texv->Name = u"Check Image";
        dizzLog().verbose(u"new CheckTex generated.\n");
        return texv;
    }
};
static CheckTexCtxConfig CHKTEX_CTXCFG;

oglu::oglTex2DV MultiMaterialHolder::GetCheckTex()
{
    return oglu::oglContext_::CurrentContext()->GetOrCreate<true>(CHKTEX_CTXCFG);
}


MultiMaterialHolder::MultiMaterialHolder(const uint8_t count) : Materials(count)
{
    for (uint8_t i = 0; i < count; ++i)
        Materials[i] = std::make_shared<PBRMaterial>(u"unamed");
}

static void InsertLayer(const oglTex2DArray& texarr, const uint32_t layer, const TexHolder& holder)
{
    switch (holder.index())
    {
    case 1: 
        texarr->SetTextureLayer(layer, std::get<oglTex2D>(holder));
        break;
    case 2:
        {
            const auto& fakeTex = std::get<FakeTex>(holder);
            uint8_t i = 0;
            for (const auto& dat : fakeTex->TexData)
            {
                if (xziar::img::TexFormatUtil::IsCompressType(fakeTex->TexFormat))
                    texarr->SetCompressedTextureLayer(layer, dat.GetRawPtr(), dat.GetSize(), i++);
                else
                    texarr->SetTextureLayer(layer, fakeTex->TexFormat, dat.GetRawPtr(), i++);
            }
        } break;
    default:
        break;
    }
}

void MultiMaterialHolder::Refresh()
{
    ArrangeMap newArrange;
    map<TexHolder, const PBRMaterial*> added;
    for (const auto& material : Materials)
    {
        for (const auto texmap : { &material->DiffuseMap, &material->NormalMap, &material->MetalMap, &material->RoughMap, &material->AOMap })
        {
            if (texmap->index() == 0) // empty
                continue;
            if (newArrange.count(*texmap) > 0 || added.count(*texmap)) // has exist or planned
                continue;
            if (const auto& it = Arrangement.find(*texmap); it != Arrangement.cend()) // old arrangement
                newArrange.insert(Arrangement.extract(it));
            // need new arrangement
            else 
                added[*texmap] = material.get();
        }
    }
    //quick return
    if (added.empty()) 
    {
        Arrangement.swap(newArrange);
        return;
    }
    //generate avaliable map
    set<Mapping, common::container::PairLess> avaliableMap;
    set<detail::TexTag> pendingDelTexs;
    for (const auto&[tid, texarr] : Textures)
    {
        pendingDelTexs.insert(tid);
        const uint16_t layers = (uint16_t)std::get<2>(texarr->GetSize());
        for (uint16_t j = 0; j < layers; ++j)
            avaliableMap.insert(avaliableMap.cend(), std::pair(tid, j));
    }
    for (const auto&[tex, mapping] : newArrange)
    {
        avaliableMap.erase(mapping);
        pendingDelTexs.erase(mapping.first);
    }
    //process mapping
    map<detail::TexTag, set<TexHolder>> needAdd;
    for (const auto&[holder, material] : added)
    {
        const auto[w, h] = holder.GetSize();
        if (w == 0 || h == 0)
            COMMON_THROWEX(BaseException, u"binded texture size cannot be 0", material->Name);
        if (!IsPower2(w) || !IsPower2(h))
            COMMON_THROWEX(BaseException, u"binded texture size should be power of 2", material->Name);
        const detail::TexTag tid(holder.GetInnerFormat(), w, h, holder.GetMipmapCount());
        if (const auto avaSlot = avaliableMap.lower_bound(tid); avaSlot == avaliableMap.cend())
        {
            needAdd[tid].insert(holder);
        }
        else
        {
            const uint16_t objLayer = avaSlot->second;
            pendingDelTexs.erase(avaSlot->first);
            avaliableMap.erase(avaSlot);
            InsertLayer(Textures[tid], objLayer, holder);
            newArrange.try_emplace(holder, tid, objLayer);
        }
    }
    // workaround for intel gen7.5
    for (auto&[tid, texs] : needAdd)
    {
        if (xziar::img::TexFormatUtil::IsCompressType(tid.Info.Format))
        {
            auto& texarr = Textures[tid];
            if (texarr)
            {
                for (const auto&[th, p] : newArrange)
                    if (p.first == tid)
                        texs.insert(th);
                texarr.reset(); //release texture, wait for reconstruct
            }
        }
    }
    for (const auto& [tid, texs] : needAdd)
    {
        auto& texarr = Textures[tid];
        pendingDelTexs.erase(tid);
        uint16_t objLayer = 0;
        if (texarr)
        {
            objLayer = (uint16_t)std::get<2>(texarr->GetSize());
            const auto texname = texarr->Name;
            texarr = oglu::oglTex2DArray_::Create(texarr, (uint32_t)(texs.size()));
            texarr->Name = texname;
        }
        else
        {
            texarr = oglu::oglTex2DArray_::Create(tid.Info.Width, tid.Info.Height, (uint16_t)(texs.size()), tid.Info.Format, tid.Info.Mipmap);
            const auto[w, h, l] = texarr->GetSize();
            texarr->Name = fmt::to_string(common::mlog::detail::StrFormater::ToU16Str(FMT_STRING(u"MatTexArr {}@{}x{}x{}"), xziar::img::TexFormatUtil::GetFormatName(texarr->GetInnerFormat()), w, h, l));
        }
        texarr->SetProperty(oglu::TextureFilterVal::BothLinear, oglu::TextureWrapVal::Repeat);
        for (const auto& tex : texs)
        {
            InsertLayer(texarr, objLayer, tex);
            newArrange.try_emplace(tex, tid, objLayer++);
        }
    }
    Arrangement.swap(newArrange);
    for (const auto& textag : pendingDelTexs)
        Textures.erase(textag);
    //prepare lookup
    TextureLookup.clear();
    uint8_t i = 0;
    for (const auto&[tid, texarr] : Textures)
    {
        if (texarr)
            TextureLookup[tid] = i++;
    }
}

void MultiMaterialHolder::BindTexture(oglu::ProgDraw& drawcall) const
{
    for (const auto&[tid, bank] : TextureLookup)
    {
        drawcall.SetTexture(Textures.find(tid)->second, "texs", bank);
    }
}

static forceinline uint32_t PackMapPos(const MultiMaterialHolder::ArrangeMap& mapping, const map<detail::TexTag, uint8_t>& lookup, const TexHolder& tex, const bool isUse)
{
    if (!isUse)
        return 0xffff;
    const auto it = mapping.find(tex);
    if (it == mapping.cend())
        return 0xffff;
    const auto&[tid, layer] = it->second;
    const auto bank = lookup.find(tid);
    if (bank == lookup.cend())
        return 0xffff;
    return uint32_t((bank->second << 16) | layer);
}

uint32_t MultiMaterialHolder::WriteData(common::span<std::byte> space) const
{
    uint32_t pos = 0;
    for (const auto& mat : Materials)
    {
        const auto subSpace = space.subspan(pos);
        if (subSpace.size() < WriteSize) 
            break;
        float *ptrFloat = reinterpret_cast<float*>(subSpace.data());
        uint32_t *ptrU32 = reinterpret_cast<uint32_t*>(subSpace.data());
        Vec4 basic(mat->Albedo, mat->Metalness);
        basic.save(ptrFloat);
        ptrFloat[4] = mat->Roughness;
        ptrFloat[5] = mat->Specular;
        ptrFloat[6] = mat->AO;
        ptrU32[7] = PackMapPos(Arrangement, TextureLookup, mat->AOMap, mat->UseAOMap);
        ptrU32[8] = PackMapPos(Arrangement, TextureLookup, mat->DiffuseMap, mat->UseDiffuseMap);
        ptrU32[9] = PackMapPos(Arrangement, TextureLookup, mat->NormalMap, mat->UseNormalMap);
        ptrU32[10] = PackMapPos(Arrangement, TextureLookup, mat->MetalMap, mat->UseMetalMap);
        ptrU32[11] = PackMapPos(Arrangement, TextureLookup, mat->RoughMap, mat->UseRoughMap);
        pos += WriteSize;
    }
    return pos;
}

void MultiMaterialHolder::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    auto jmaterials = context.NewArray();
    for (const auto& material : Materials)
        context.AddObject(jmaterials, *material);
    jself.Add("pbr", jmaterials);
}
void MultiMaterialHolder::Deserialize(DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object)
{
    Materials.clear();
    for (const auto& material : object.GetArray("pbr"))
        Materials.push_back(context.DeserializeShare<PBRMaterial>(xziar::ejson::JObjectRef<true>(material)));
}
RESPAK_IMPL_SIMP_DESERIALIZE(MultiMaterialHolder)


}
