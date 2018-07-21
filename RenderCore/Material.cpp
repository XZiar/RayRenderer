#include "RenderCoreRely.h"
#include "Material.h"

namespace rayr
{
using common::asyexe::AsyncAgent;
using b3d::Vec4;
using oglu::oglTex2D;
using oglu::oglTex2DArray;
using oglu::TextureInnerFormat;
using std::multimap;

oglu::TextureInnerFormat PBRMaterial::GetInnerFormat(const TexHolder& holder)
{
    switch (holder.index())
    {
    case 1: return std::get<oglTex2D>(holder)->GetInnerFormat();
    case 2: return std::get<FakeTex>(holder)->TexFormat;
    default: return (TextureInnerFormat)GL_INVALID_ENUM;
    }

}

u16string PBRMaterial::GetName(const TexHolder& holder)
{
    switch (holder.index())
    {
    case 1: return std::get<oglTex2D>(holder)->Name;
    case 2: return std::get<FakeTex>(holder)->Name;
    default: return u"";
    }
}

std::pair<uint32_t, uint32_t> PBRMaterial::GetSize(const TexHolder& holder)
{
    switch (holder.index())
    {
    case 1: return std::get<oglTex2D>(holder)->GetSize();
    case 2: {const auto& ft = std::get<FakeTex>(holder); return { ft->Width, ft->Height }; };
    default: return { 0,0 };
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


static oglu::detail::ContextResource<oglu::oglTex2DV, true> CTX_CHECK_TEX;
constexpr auto GenerateCheckImg()
{
    std::array<uint32_t, 128 * 128> pixs{};
    for (uint32_t a = 0, idx = 0; a < 128; ++a)
    {
        for (uint32_t b = 0; b < 128; ++b)
        {
            pixs[idx++] = ((a / 32) & 0x1) == ((b / 32) & 0x1) ? 0xff0f0f0fu : 0xffa0a0a0u;
        }
    }
    return pixs;
}
static constexpr auto CHECK_IMG_DATA = GenerateCheckImg();
oglu::oglTex2DV MultiMaterialHolder::GetCheckTex()
{
    return CTX_CHECK_TEX.GetOrInsert([](const auto&)
    {
        oglu::oglTex2DS chkTex(128, 128, oglu::TextureInnerFormat::RGBA8);
        chkTex->SetData(oglu::TextureDataFormat::RGBA8, CHECK_IMG_DATA.data());
        const auto texv = chkTex->GetTextureView();
        texv->Name = u"Check Image";
        basLog().verbose(u"new CheckTex generated.\n");
        return texv;
    });
}


static void InsertLayer(const oglTex2DArray& texarr, const uint32_t layer, const PBRMaterial::TexHolder& holder)
{
    switch (holder.index())
    {
    case 1: 
        texarr->SetTextureLayer(layer, std::get<oglTex2D>(holder));
        break;
    case 2:
        {
            const auto& texDat = std::get<FakeTex>(holder)->TexData;
            texarr->SetCompressedTextureLayer(layer, texDat.GetRawPtr(), texDat.GetSize());
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
    for (const auto&[tex, material] : added)
    {
        const auto[w, h] = PBRMaterial::GetSize(tex);
        if (w == 0 || h == 0)
            COMMON_THROW(BaseException, L"binded texture size cannot be 0", material->Name);
        if (!IsPower2(w) || !IsPower2(h))
            COMMON_THROW(BaseException, L"binded texture size should be power of 2", material->Name);
        const detail::TexTag tid(PBRMaterial::GetInnerFormat(tex), w, h);
        if (const auto avaSlot = avaliableMap.lower_bound(tid); avaSlot == avaliableMap.cend())
        {
            needAdd[tid].insert(tex);
        }
        else
        {
            const uint16_t objLayer = avaSlot->second;
            avaliableMap.erase(avaSlot);
            InsertLayer(Textures[tid], objLayer, tex);
        }
    }
    // workaround for intel gen7.5
    for (auto&[tid, texs] : needAdd)
    {
        if (oglu::detail::_oglTexBase::IsCompressType(tid.Info.Format))
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
        uint32_t objLayer = 0;
        if (texarr)
        {
            objLayer = std::get<2>(texarr->GetSize());
            const auto texname = texarr->Name;
            texarr = oglTex2DArray(texarr, (uint32_t)(texs.size()));
            texarr->Name = texname;
        }
        else
        {
            texarr.reset(tid.Info.Width, tid.Info.Height, (uint16_t)(texs.size()), tid.Info.Format);
            const auto[w, h, l] = texarr->GetSize();
            texarr->Name = fmt::to_string(common::mlog::detail::StrFormater<char16_t>::ToU16Str(u"MatTexArr {}@{}x{}", oglu::detail::_oglTexBase::GetFormatName(texarr->GetInnerFormat()), w, h));
            texarr->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Repeat);
            
            static_assert(std::is_convertible_v<decltype(std::declval<u16string>().data()), const char16_t*>);
        }
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

static forceinline uint32_t PackMapPos(const MultiMaterialHolder::ArrangeMap& mapping, const map<detail::TexTag, uint8_t>& lookup, const PBRMaterial::TexHolder& tex, const bool isUse)
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


}
