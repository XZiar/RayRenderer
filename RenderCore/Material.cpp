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


static uint16_t PackMapPos(const PBRMaterial::ArrangeMap& mapping, const PBRMaterial::ImageHolder& img, const bool isUse)
{
    if (!isUse) 
        return 0xffff;
    const auto it = mapping.find(img);
    if (it == mapping.cend())
        return 0xffff;
    const auto&[bank, layer] = it->second;
    return uint16_t((bank << 12) | layer);
}

uint32_t PBRMaterial::WriteData(std::byte *ptr, const ArrangeMap& mapping) const
{
    float *ptrFloat = reinterpret_cast<float*>(ptr);
    uint16_t *ptrU16 = reinterpret_cast<uint16_t*>(ptr);
    ptrU16[0] = PackMapPos(mapping, DiffuseMap, UseDiffuseMap);
    ptrU16[1] = PackMapPos(mapping, NormalMap, UseNormalMap);
    ptrU16[2] = PackMapPos(mapping, MetalMap, UseMetalMap);
    ptrU16[3] = PackMapPos(mapping, RoughMap, UseRoughMap);
    ptrU16[4] = PackMapPos(mapping, AOMap, UseAOMap);
    Vec4 basic(Albedo, Metalness);
    memcpy_s(ptrFloat + 4, sizeof(Vec4), &basic, sizeof(Vec4));
    ptrFloat[8] = Roughness;
    ptrFloat[9] = Specular;
    ptrFloat[10] = AO;
    return UnitSize;
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
    auto asyncRet = oglu::oglUtil::invokeAsyncGL([&](const AsyncAgent&)
    {
        CompressData(img, texdata, format);
    }, taskName);
    common::asyexe::AsyncAgent::SafeWait(asyncRet);
    
    oglu::oglTex2DS tex(img.Width, img.Height, format);
    tex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Clamp);
    tex->SetCompressedData(texdata);
    return tex;
}


static constexpr bool IsPower2(const uint32_t num) { return (num & (num - 1)) == 0; }

static std::shared_ptr<xziar::img::Image> RawImgResize(const std::shared_ptr<xziar::img::Image>& img, const u16string& matName, uint8_t& sizepow)
{
    const auto w = img->Width, h = img->Height, pix = w * h;
    if (pix == 0)
        COMMON_THROW(BaseException, L"binded image size cannot be 0", matName);
    std::shared_ptr<xziar::img::Image> newImg;
    if (w != h || !IsPower2(w))
    {
        const auto maxWH = common::max(w, h);
        sizepow = 0;
        for (uint16_t cursize = 4; sizepow < 13; ++sizepow, cursize *= 2)
        {
            const uint32_t curpix = cursize * cursize;
            if (pix >= curpix && pix <= curpix * 2)
                break;
        }
        const auto objWH = 0x1u << sizepow;
        uint32_t newW, newH;
        if (w <= h)
            newH = objWH, newW = objWH * w / h;
        else
            newW = objWH, newH = objWH * h / w;
        basLog().debug(u"decide to resize image[{}*{}] to [{}*{}].\n", w, h, newW, newH);
        auto tmpImg = xziar::img::Image(*img);
        tmpImg.Resize(newW, newH);
        newImg = std::make_shared<xziar::img::Image>(img->DataType);
        newImg->SetSize(objWH, objWH);
        newImg->PlaceImage(tmpImg, 0, 0, 0, 0);
    }
    else
    {
        newImg = img;
        sizepow = (uint8_t)std::log2(w);
    }
    return newImg;
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
        basLog().verbose(u"new CheckTex generated.\n");
        return chkTex->GetTextureView();
    });
}

void MultiMaterialHolder::Refresh()
{
    ArrangeMap newArrange;
    map<PBRMaterial::ImageHolder, const PBRMaterial*> added;
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
            else // need new arrangement
                added[*texmap] = &material;
        }
    }
    if (added.empty()) // quick return
    {
        Arrangement.swap(newArrange);
        return;
    }
    //generate avaliable map
    set<uint16_t> avaliable[13];
    for (uint8_t i = 0; i < 13; ++i)
    {
        if (!Textures[i]) continue;
        const uint16_t layers = (uint16_t)std::get<2>(Textures[i]->GetSize());
        auto objset = avaliable[i];
        for (uint16_t j = 0; j < layers; ++j)
            objset.insert(objset.end(), j);
    }
    for (const auto& p : newArrange)
        avaliable[p.second.first].erase(p.second.second);
    //process mapping
    set<oglTex2D> needAdd[15];
    for (const auto&[holder, material] : added)
    {
        oglTex2D objtex;
        uint8_t sizepow = 0;
        if (holder.index() == 2) // oglTexture
        {
            objtex = std::get<2>(holder);
            const auto[w, h] = objtex->GetSize();
            if (w != h)
                COMMON_THROW(BaseException, L"binded texture not square", material->Name);
            if (w == 0)
                COMMON_THROW(BaseException, L"binded texture size cannot be 0", material->Name);
            if (!IsPower2(w))
                COMMON_THROW(BaseException, L"binded texture size should be power of 2", material->Name);
            sizepow = (uint8_t)(std::log2(w) - 2);
        }
        else // raw image
        {
            const auto img = RawImgResize(std::get<1>(holder), material->Name, sizepow);
            objtex = GenTexture(*img);
        }
        if (auto& it = avaliable[sizepow]; it.empty())
        {
            needAdd[sizepow].insert(objtex);
        }
        else
        {
            const uint16_t objLayer = *it.begin();
            it.erase(it.begin());
            Textures[sizepow]->SetTextureLayer(objLayer, objtex);
        }
    }
    for (uint8_t i = 0; i < 13; ++i)
    {
        const auto& objAdd = needAdd[i];
        if (objAdd.empty())
            continue;
        auto& oldTex = Textures[i];
        uint32_t objLayer = 0;
        oglTex2DArray newTex;
        if (oldTex)
        {
            const auto[w, h, l] = oldTex->GetSize();
            newTex.reset(w, h, (uint16_t)(l + objAdd.size()), oglu::TextureInnerFormat::RGBA8);
            newTex->SetTextureLayers(0, oldTex, 0, l);
            objLayer = l;
        }
        else
        {
            newTex.reset(4 << i, 4 << i, (uint16_t)(objAdd.size()), oglu::TextureInnerFormat::RGBA8);
        }
        for (const auto& tex : objAdd)
        {
            newTex->SetTextureLayer(objLayer, tex);
            newArrange[tex] = { i,objLayer++ };
        }
        oldTex = newTex;
    }
    Arrangement.swap(newArrange);
}

void MultiMaterialHolder::BindTexture(oglu::detail::ProgDraw& drawcall) const
{
    for (uint8_t i = 0; i < 13; ++i)
    {
        if (Textures[i])
            drawcall.SetTexture(Textures[i], "texs", i);
    }
}

uint32_t MultiMaterialHolder::WriteData(std::byte *ptr) const
{
    uint32_t pos = 0;
    for (const auto& mat : Materials)
        pos += mat.WriteData(ptr + pos, Arrangement);
    return pos;
}


}
