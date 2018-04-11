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


oglu::oglTex2D GenTexture(const xziar::img::Image& img, const oglu::TextureInnerFormat format)
{
    oglu::oglTex2D tex(std::in_place);
    tex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Clamp);
    tex->SetData(format, img);
    return tex;
}

static void CompressData(const xziar::img::Image& img, vector<uint8_t>& output, const oglu::TextureInnerFormat format)
{
    auto tex = GenTexture(img, format);
    if (auto dat = tex->GetCompressedData())
        output = std::move(*dat);
}

oglu::oglTex2D GenTextureAsync(const xziar::img::Image& img, const oglu::TextureInnerFormat format, const u16string& taskName)
{
    vector<uint8_t> texdata;
    auto asyncRet = oglu::oglUtil::invokeAsyncGL([&](const AsyncAgent&)
    {
        CompressData(img, texdata, format);
    }, taskName);
    common::asyexe::AsyncAgent::SafeWait(asyncRet);
    
    oglu::oglTex2D tex(std::in_place);
    tex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Clamp);
    tex->SetCompressedData(format, img.Width, img.Height, texdata);
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
        for (uint16_t cursize = 1; sizepow < 15; ++sizepow, cursize *= 2)
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
    set<uint16_t> avaliable[15];
    for (uint8_t i = 0; i < 15; ++i)
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
            sizepow = (uint8_t)std::log2(w);
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
    for (uint8_t i = 0; i < 15; ++i)
    {
        const auto& objAdd = needAdd[i];
        if (objAdd.empty())
            continue;
        const auto& oldTex = Textures[i];
        const auto[w, h, l] = oldTex->GetSize();
        oglTex2DArray newTex(w, h, (uint16_t)(l + objAdd.size()), oglu::TextureDataFormat::RGBA8);
        newTex->SetTextureLayers(0, oldTex, 0, l);
        uint32_t objLayer = l;
        for (const auto& tex : objAdd)
            newTex->SetTextureLayer(objLayer, tex);
        Textures[i] = newTex;
    }
    Arrangement.swap(newArrange);
}



}
