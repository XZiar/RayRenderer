#include "RenderCoreRely.h"
#include "Material.h"

namespace rayr
{
using common::asyexe::AsyncAgent;
using b3d::Vec4;
using oglu::oglTex2D;
using oglu::oglTex2DArray;


uint32_t PBRMaterial::WriteData(std::byte *ptr) const
{
    float *ptrFloat = reinterpret_cast<float*>(ptr);
    Vec4 basic(Albedo, Metalness);
    if (UseDiffuseMap)
        basic.x *= -1.0f;
    if (UseNormalMap)
        basic.y *= -1.0f;
    memcpy_s(ptrFloat, sizeof(Vec4), &basic, sizeof(Vec4));
    ptrFloat[4] = Roughness;
    ptrFloat[5] = Specular;
    ptrFloat[6] = AO;
    return 8 * sizeof(float);
}



oglu::oglTex2DS GenTexture(const xziar::img::Image& img, const oglu::TextureInnerFormat format)
{
    oglu::oglTex2DS tex(img.Width, img.Height, format);
    tex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Clamp);
    tex->SetData(img);
    return tex;
}

static void CompressData(const xziar::img::Image& img, vector<uint8_t>& output, const oglu::TextureInnerFormat format)
{
    auto tex = GenTexture(img, format);
    if (auto dat = tex->GetCompressedData())
        output = std::move(*dat);
}

oglu::oglTex2DS GenTextureAsync(const xziar::img::Image& img, const oglu::TextureInnerFormat format, const u16string& taskName)
{
    vector<uint8_t> texdata;
    const auto asyncRet = oglu::oglUtil::invokeAsyncGL([&](const AsyncAgent&)
    {
        CompressData(img, texdata, format);
    }, taskName);
    oglu::oglTex2DS tex(img.Width, img.Height, format);
    tex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Clamp);

    common::asyexe::AsyncAgent::SafeWait(asyncRet);
    tex->SetCompressedData(texdata);
    return tex;
}


static constexpr forceinline bool IsPower2(const uint32_t num) { return (num & (num - 1)) == 0; }

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
        basLog().verbose(u"new CheckTex generated.\n");
        return chkTex->GetTextureView();
    });
}

oglu::oglTex2DS MultiMaterialHolder::LoadImgToTex(const xziar::img::Image& img, const oglu::TextureInnerFormat format)
{
    const auto w = img.Width, h = img.Height;
    if (w <= 4 || h <= 4)
        COMMON_THROW(BaseException, L"image size to small");
    if (!IsPower2(w) || !IsPower2(h))
    {
        const auto newW = 1 << uint32_t(std::round(std::log2(w)));
        const auto newH = 1 << uint32_t(std::round(std::log2(h)));
        basLog().debug(u"decide to resize image[{}*{}] to [{}*{}].\n", w, h, newW, newH);
        auto newImg = xziar::img::Image(img);
        newImg.Resize(newW, newH);
        return LoadImgToTex(newImg);
    }
    return GenTextureAsync(img);
}

void MultiMaterialHolder::Refresh()
{
    ArrangeMap newArrange;
    map<oglTex2D, const PBRMaterial*> added;
    for (const auto& material : Materials)
    {
        for (const auto texmap : { &material.DiffuseMap, &material.NormalMap, &material.MetalMap, &material.RoughMap, &material.AOMap })
        {
            if (!*texmap) // empty
                continue;
            if (newArrange.count(*texmap) > 0 || added.count(*texmap)) // has exist or planned
                continue;
            if (const auto& it = Arrangement.find(*texmap); it != Arrangement.cend()) // old arrangement
                newArrange.insert(Arrangement.extract(it));
            else // need new arrangement
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
    set<uint16_t> avaliableMap[13 * 13];
    for(const auto idx : AllocatedTexture)
    {
        auto avaSet = avaliableMap[idx];
        const uint16_t layers = (uint16_t)std::get<2>(Textures[idx]->GetSize());
        for (uint16_t j = 0; j < layers; ++j)
            avaSet.insert(avaSet.end(), j);
    }
    for (const auto&[tex, mapping] : newArrange)
        avaliableMap[mapping.first].erase(mapping.second);
    //process mapping
    map<uint8_t, set<oglTex2D>> needAdd;
    for (const auto&[tex, material] : added)
    {
        const auto[w, h] = tex->GetSize();
        if (w == 0 || h == 0)
            COMMON_THROW(BaseException, L"binded texture size cannot be 0", material->Name);
        if (!IsPower2(w) || !IsPower2(h))
            COMMON_THROW(BaseException, L"binded texture size should be power of 2", material->Name);
        const uint8_t widthpow = (uint8_t)(std::log2(w) - 2), heightpow = (uint8_t)(std::log2(h) - 2);
        const uint8_t sizepow = uint8_t(widthpow + heightpow * 13);
        if (auto& avaSet = avaliableMap[sizepow]; avaSet.empty())
        {
            needAdd[sizepow].insert(tex);
        }
        else
        {
            const uint16_t objLayer = *avaSet.begin();
            avaSet.erase(avaSet.begin());
            Textures[sizepow]->SetTextureLayer(objLayer, tex);
        }
    }
    for (const auto& [sizepow, texs] : needAdd)
    {
        auto& texarr = Textures[sizepow];
        uint32_t objLayer = 0;
        if (texarr)
        {
            objLayer = std::get<2>(texarr->GetSize());
            texarr = oglTex2DArray(texarr, (uint32_t)(texs.size()));
        }
        else
        {
            const auto widthpow = sizepow % 13, heightpow = sizepow / 13;
            texarr.reset(4 << widthpow, 4 << heightpow, (uint16_t)(texs.size()), oglu::TextureInnerFormat::RGBA8);
        }
        for (const auto& tex : texs)
        {
            texarr->SetTextureLayer(objLayer, tex);
            newArrange[tex] = { sizepow,objLayer++ };
        }
    }
    Arrangement.swap(newArrange);
    //prepare lookup
    AllocatedTexture.clear();
    uint32_t i = 0;
    for (const auto& texarr : Textures)
    {
        if (texarr)
        {
            TextureLookup[i] = static_cast<uint8_t>(AllocatedTexture.size());
            AllocatedTexture.push_back(i);
        }
        else
            TextureLookup[i] = 0xff;
        ++i;
    }
}

void MultiMaterialHolder::BindTexture(oglu::detail::ProgDraw& drawcall) const
{
    uint8_t idx = 0;
    for (const auto texIdx : AllocatedTexture)
    {
        drawcall.SetTexture(Textures[texIdx], "texs", idx++);
    }
}

static forceinline uint32_t PackMapPos(const PBRMaterial::ArrangeMap& mapping, const uint8_t(&lookup)[13 * 13], const oglTex2D& tex, const bool isUse)
{
    if (!isUse)
        return 0xffff;
    const auto it = mapping.find(tex);
    if (it == mapping.cend())
        return 0xffff;
    const auto&[idx, layer] = it->second;
    const auto bank = lookup[idx];
    if (bank == 0xff)
        return 0xffff;
    return uint32_t((bank << 16) | layer);
}

uint32_t MultiMaterialHolder::WriteData(std::byte *ptr) const
{
    uint32_t pos = 0;
    for (const auto& mat : Materials)
    {
        float *ptrFloat = reinterpret_cast<float*>(ptr + pos);
        uint32_t *ptrU32 = reinterpret_cast<uint32_t*>(ptr + pos);
        Vec4 basic(mat.Albedo, mat.Metalness);
        memcpy_s(ptrFloat, sizeof(Vec4), &basic, sizeof(Vec4));
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
