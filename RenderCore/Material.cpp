#include "RenderCoreRely.h"
#include "Material.h"
#include "common/PromiseTaskSTD.hpp"
#include "ThumbnailManager.h"

namespace rayr
{
using b3d::Vec4;
using oglu::oglTex2D;
using oglu::oglTex2DArray;
using oglu::TextureInnerFormat;

oglu::TextureInnerFormat TexHolder::GetInnerFormat() const
{
    switch (index())
    {
    case 1: return std::get<oglTex2D>(*this)->GetInnerFormat();
    case 2: return std::get<FakeTex>(*this)->TexFormat;
    default: return (TextureInnerFormat)GL_INVALID_ENUM;
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
    case 1: return std::get<oglTex2D>(*this).weakRef();
    case 2: return std::weak_ptr<detail::_FakeTex>(std::get<FakeTex>(*this));
    default: return {};
    }
}

uint32_t PBRMaterial::WriteData(std::byte *ptr) const
{
    float *ptrFloat = reinterpret_cast<float*>(ptr);
    Vec4 basic(Albedo, Metalness);
    if (UseDiffuseMap)
        basic.x *= -1.0f;
    if (UseNormalMap)
        basic.y *= -1.0f;
    basic.save(ptrFloat);
    ptrFloat[4] = Roughness;
    ptrFloat[5] = Specular;
    ptrFloat[6] = AO;
    return 8 * sizeof(float);
}


static ejson::JDoc SerializeTex(const TexHolder& holder, SerializeUtil & context)
{
    switch (holder.index())
    {
    case 1: 
    {
        const auto& tex = std::get<oglTex2D>(holder);
        auto jtex = context.NewObject();
        jtex.Add("name", str::to_u8string(tex->Name, Charset::UTF16LE));
        jtex.Add("format", (uint32_t)tex->GetInnerFormat());
        const auto[w, h] = tex->GetSize();
        jtex.Add("width", w);
        jtex.Add("height", h);
        std::vector<uint8_t> data;
        if (tex->IsCompressed())
            data = tex->GetCompressedData().value();
        else
            data = tex->GetData(oglu::TexFormatUtil::DecideFormat(tex->GetInnerFormat()));
        const auto datahandle = context.PutResource(data.data(), data.size());
        jtex.Add("data", datahandle);
        return std::move(jtex);
    }
    case 2: 
    {
        const auto& tex = std::get<FakeTex>(holder);
        auto jtex = context.NewObject();
        jtex.Add("name", str::to_u8string(tex->Name, Charset::UTF16LE));
        jtex.Add("format", (uint32_t)tex->TexFormat);
        jtex.Add("width", tex->Width);
        jtex.Add("height", tex->Height);
        const auto datahandle = context.PutResource(tex->TexData.GetRawPtr(), tex->TexData.GetSize());
        jtex.Add("data", datahandle);
        return std::move(jtex);
    }
    default: 
        return ejson::JNull();
    }
}
ejson::JObject PBRMaterial::Serialize(SerializeUtil & context) const
{
    auto jself = context.NewObject();
    jself.Add("name", str::to_u8string(Name, Charset::UTF16LE));
    jself.Add("albedo", detail::ToJArray(context, Albedo));
    jself.EJOBJECT_ADD(Metalness)
        .EJOBJECT_ADD(Roughness)
        .EJOBJECT_ADD(Specular)
        .EJOBJECT_ADD(AO)
        .EJOBJECT_ADD(UseDiffuseMap)
        .EJOBJECT_ADD(UseNormalMap)
        .EJOBJECT_ADD(UseMetalMap)
        .EJOBJECT_ADD(UseRoughMap)
        .EJOBJECT_ADD(UseAOMap);
    jself.Add("DiffuseMap", SerializeTex(DiffuseMap, context));
    jself.Add("NormalMap", SerializeTex(NormalMap, context));
    jself.Add("MetalMap", SerializeTex(MetalMap, context));
    jself.Add("RoughMap", SerializeTex(RoughMap, context));
    jself.Add("AOMap", SerializeTex(AOMap, context));
    return jself;
}
void PBRMaterial::Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object)
{
    detail::FromJArray(object.GetArray("albedo"), Albedo);
    EJSON_GET_MEMBER(object, Metalness);
    EJSON_GET_MEMBER(object, Roughness);
    EJSON_GET_MEMBER(object, Specular);
    EJSON_GET_MEMBER(object, AO);
    EJSON_GET_MEMBER(object, UseDiffuseMap);
    EJSON_GET_MEMBER(object, UseNormalMap);
    EJSON_GET_MEMBER(object, UseMetalMap);
    EJSON_GET_MEMBER(object, UseRoughMap);
    EJSON_GET_MEMBER(object, UseAOMap);
}
RESPAK_DESERIALIZER(PBRMaterial)
{
    auto ret = new PBRMaterial(str::to_u16string(object.Get<string>("name"), Charset::UTF8));
    ret->Deserialize(context, object);
    return std::unique_ptr<Serializable>(ret);
}
RESPAK_REGIST_DESERIALZER(PBRMaterial)

struct CheckTexCtxConfig : public oglu::CtxResConfig<true, oglu::oglTex2DV>
{
    oglu::oglTex2DV Construct() const 
    { 
        oglu::oglTex2DS chkTex(128, 128, oglu::TextureInnerFormat::SRGBA8);
        std::array<uint32_t, 128 * 128> pixs{};
        for (uint32_t a = 0, idx = 0; a < 128; ++a)
        {
            for (uint32_t b = 0; b < 128; ++b)
            {
                pixs[idx++] = ((a / 32) & 0x1) == ((b / 32) & 0x1) ? 0xff0f0f0fu : 0xffa0a0a0u;
            }
        }
        chkTex->SetData(oglu::TextureDataFormat::RGBA8, pixs.data());
        const auto texv = chkTex->GetTextureView();
        texv->Name = u"Check Image";
        basLog().verbose(u"new CheckTex generated.\n");
        return texv;
    }
};
static CheckTexCtxConfig CHKTEX_CTXCFG;

oglu::oglTex2DV MultiMaterialHolder::GetCheckTex()
{
    return oglu::oglContext::CurrentContext()->GetOrCreate<true>(CHKTEX_CTXCFG);
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
            if (oglu::TexFormatUtil::IsCompressType(fakeTex->TexFormat))
                texarr->SetCompressedTextureLayer(layer, fakeTex->TexData.GetRawPtr(), fakeTex->TexData.GetSize());
            else
                texarr->SetTextureLayer(layer, oglu::TexFormatUtil::DecideFormat(fakeTex->TexFormat), fakeTex->TexData.GetRawPtr());
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
        for (const auto texmap : { &material.DiffuseMap, &material.NormalMap, &material.MetalMap, &material.RoughMap, &material.AOMap })
        {
            if (texmap->index() == 0) // empty
                continue;
            if (newArrange.count(*texmap) > 0 || added.count(*texmap)) // has exist or planned
                continue;
            if (const auto& it = Arrangement.find(*texmap); it != Arrangement.cend()) // old arrangement
                newArrange.insert(Arrangement.extract(it));
            // need new arrangement
            else 
                added[*texmap] = &material;
        }
    }
    //quick return
    if (added.empty()) 
    {
        Arrangement.swap(newArrange);
        return;
    }
    {
        const auto thumbman = ThumbMan.lock();
        if (thumbman)
            thumbman->PrepareThumbnails(common::container::KeySet(added));
    }
    //generate avaliable map
    set<Mapping, common::container::PairLess<detail::TexTag, uint16_t>> avaliableMap;
    for (const auto&[tid, texarr] : Textures)
    {
        const uint16_t layers = (uint16_t)std::get<2>(texarr->GetSize());
        for (uint16_t j = 0; j < layers; ++j)
            avaliableMap.insert(avaliableMap.cend(), std::pair(tid, j));
    }
    for (const auto&[tex, mapping] : newArrange)
        avaliableMap.erase(mapping);
    //process mapping
    map<detail::TexTag, set<TexHolder>> needAdd;
    for (const auto&[holder, material] : added)
    {
        const auto[w, h] = holder.GetSize();
        if (w == 0 || h == 0)
            COMMON_THROW(BaseException, u"binded texture size cannot be 0", material->Name);
        if (!IsPower2(w) || !IsPower2(h))
            COMMON_THROW(BaseException, u"binded texture size should be power of 2", material->Name);
        const detail::TexTag tid(holder.GetInnerFormat(), w, h);
        if (const auto avaSlot = avaliableMap.lower_bound(tid); avaSlot == avaliableMap.cend())
        {
            needAdd[tid].insert(holder);
        }
        else
        {
            const uint16_t objLayer = avaSlot->second;
            avaliableMap.erase(avaSlot);
            InsertLayer(Textures[tid], objLayer, holder);
        }
    }
    // workaround for intel gen7.5
    for (auto&[tid, texs] : needAdd)
    {
        if (oglu::TexFormatUtil::IsCompressType(tid.Info.Format))
        {
            auto& texarr = Textures[tid];
            if (texarr)
            {
                for (const auto&[th, p] : newArrange)
                    if (p.first == tid)
                        texs.insert(th);
                texarr.release(); //release texture, wait for reconstruct
            }
        }
    }
    for (const auto& [tid, texs] : needAdd)
    {
        auto& texarr = Textures[tid];
        uint16_t objLayer = 0;
        if (texarr)
        {
            objLayer = (uint16_t)std::get<2>(texarr->GetSize());
            const auto texname = texarr->Name;
            texarr = oglTex2DArray(texarr, (uint32_t)(texs.size()));
            texarr->Name = texname;
        }
        else
        {
            texarr.reset(tid.Info.Width, tid.Info.Height, (uint16_t)(texs.size()), tid.Info.Format);
            const auto[w, h, l] = texarr->GetSize();
            texarr->Name = fmt::to_string(common::mlog::detail::StrFormater<char16_t>::ToU16Str(u"MatTexArr {}@{}x{}", oglu::TexFormatUtil::GetFormatName(texarr->GetInnerFormat()), w, h));
        }
        texarr->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Repeat);
        for (const auto& tex : texs)
        {
            InsertLayer(texarr, objLayer, tex);
            newArrange.try_emplace(tex, tid, objLayer++);
        }
    }
    Arrangement.swap(newArrange);
    //prepare lookup
    TextureLookup.clear();
    uint8_t i = 0;
    for (const auto&[tid, texarr] : Textures)
    {
        if (texarr)
            TextureLookup[tid] = i++;
    }
}

void MultiMaterialHolder::BindTexture(oglu::detail::ProgDraw& drawcall) const
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

uint32_t MultiMaterialHolder::WriteData(std::byte *ptr) const
{
    uint32_t pos = 0;
    for (const auto& mat : Materials)
    {
        float *ptrFloat = reinterpret_cast<float*>(ptr + pos);
        uint32_t *ptrU32 = reinterpret_cast<uint32_t*>(ptr + pos);
        Vec4 basic(mat.Albedo, mat.Metalness);
        basic.save(ptrFloat);
        ptrFloat[4] = mat.Roughness;
        ptrFloat[5] = mat.Specular;
        ptrFloat[6] = mat.AO;
        ptrU32[7] = PackMapPos(Arrangement, TextureLookup, mat.AOMap, mat.UseAOMap);
        ptrU32[8] = PackMapPos(Arrangement, TextureLookup, mat.DiffuseMap, mat.UseDiffuseMap);
        ptrU32[9] = PackMapPos(Arrangement, TextureLookup, mat.NormalMap, mat.UseNormalMap);
        ptrU32[10] = PackMapPos(Arrangement, TextureLookup, mat.MetalMap, mat.UseMetalMap);
        ptrU32[11] = PackMapPos(Arrangement, TextureLookup, mat.RoughMap, mat.UseRoughMap);
        pos += UnitSize;
    }
    return pos;
}

ejson::JObject MultiMaterialHolder::Serialize(SerializeUtil & context) const
{
    auto jself = context.NewObject();
    auto jmaterials = context.NewArray();
    for (const auto& material : Materials)
        context.AddObject(jmaterials, material);
    jself.Add("pbr", jmaterials);
    return jself;
}
void MultiMaterialHolder::Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object)
{
    Materials.clear();
    for (const auto& material : object.GetArray("pbr"))
        Materials.push_back(*context.Deserialize<PBRMaterial>(ejson::JObjectRef<true>(material)));
}
RESPAK_DESERIALIZER(MultiMaterialHolder)
{
    auto ret = new MultiMaterialHolder();
    ret->Deserialize(context, object);
    return std::unique_ptr<Serializable>(ret);
}
RESPAK_REGIST_DESERIALZER(MultiMaterialHolder)


}
