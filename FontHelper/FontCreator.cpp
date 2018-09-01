#include "FontRely.h"
#include "FontCreator.h"
#include "freetype2/freetype2.h"
#include "OpenCLUtil/oclException.h"
#include <cmath>
#include "resource.h"
#pragma warning(disable : 4996)
#include "fmt/time.h"
#pragma warning(default : 4996)

namespace oglu
{
using namespace oclu;
using xziar::img::Image;
using xziar::img::ImageDataType;


struct FontInfo
{
    uint32_t offset;
    uint8_t w, h;
};


auto FindPlatform(const std::vector<oclPlatform>& platforms, const oclu::Vendor vendor)
{
    for (const auto& plt : platforms)
        if (plt->PlatVendor == vendor)
            return plt;
    for (const auto& plt : platforms)
        if (plt->PlatVendor == Vendor::NVIDIA)
            return plt;
    for (const auto& plt : platforms)
        if (plt->PlatVendor == Vendor::AMD)
            return plt;
    for (const auto& plt : platforms)
        if (plt->PlatVendor == Vendor::Intel)
            return plt;
    return oclPlatform();
}

oclu::oclContext createOCLContext(const oclu::Vendor vendor)
{
    oclPlatform clPlat = FindPlatform(oclUtil::getPlatforms(), vendor);
    if (!clPlat)
        return oclContext();
    auto clCtx = clPlat->CreateContext();
    fntLog().success(u"Created Context in platform {}!\n", clPlat->Name);
    clCtx->onMessage = [](const u16string& errtxt)
    {
        fntLog().error(u"Error from context:\t{}\n", errtxt);
    };
    return clCtx;
}

static SharedResource<oclu::oclContext, const oclu::Vendor> clRes(createOCLContext);


void FontCreator::loadCL(const string& src)
{
    oclProgram clProg(clCtx, src);
    try
    {
        string options = clCtx->vendor == Vendor::NVIDIA ? "-cl-kernel-arg-info -cl-fast-relaxed-math -cl-nv-verbose -DNVIDIA" : "-cl-fast-relaxed-math";
        options += " -DLOC_MEM_SIZE=" + std::to_string(clCtx->Devices[0]->LocalMemSize);
        clProg->Build(options);
    }
    catch (OCLException& cle)
    {
        fntLog().error(u"Fail to build opencl Program:\n{}\n", cle.message);
        COMMON_THROW(BaseException, u"build Program error");
    }
    kerSdf = clProg->GetKernel("bmpsdf");
    kerSdfGray = clProg->GetKernel("graysdf");
    kerSdfGray4 = clProg->GetKernel("graysdf4");
    {
        const auto wgInfo = kerSdfGray->GetWorkGroupInfo(clCtx->Devices[0]);
        //preset arg
        kerSdfGray->SetArg(0, infoBuf);
        kerSdfGray->SetArg(1, inputBuf);
        kerSdfGray->SetArg(2, middleBuf);

        kerSdfGray4->SetArg(0, infoBuf);
        kerSdfGray4->SetArg(1, inputBuf);
        kerSdfGray4->SetArg(2, middleBuf);
    }
}

void FontCreator::loadDownSampler(const string& src)
{
    oclProgram clProg(clCtx, src);
    try
    {
        string options = clCtx->vendor == Vendor::NVIDIA ? "-cl-kernel-arg-info -cl-fast-relaxed-math -cl-nv-verbose -DNVIDIA" : "-cl-fast-relaxed-math";
        options += " -DLOC_MEM_SIZE=" + std::to_string(clCtx->Devices[0]->LocalMemSize);
        clProg->Build(options);
    }
    catch (OCLException& cle)
    {
        fntLog().error(u"Fail to build opencl Program:\n{}\n", cle.message);
        COMMON_THROW(BaseException, u"build Program error");
    }
    kerDownSamp = clProg->GetKernel("avg16");// "downsample4");
    kerDownSamp->SetArg(0, infoBuf);
    kerDownSamp->SetArg(1, middleBuf);
    kerDownSamp->SetArg(2, outputBuf);
}

FontCreator::FontCreator(const oclu::Vendor preferredVendor)
{
    clCtx = createOCLContext(preferredVendor);
    for (const auto& dev : clCtx->Devices)
        if (dev->Type == DeviceType::GPU)
        {
            clQue.reset(clCtx, dev);
            break;
        }
    if(!clQue)
        COMMON_THROW(BaseException, u"clQueue initialized failed! There may be no GPU device found in platform");

    //prepare LUT
    {
        std::array<float, 256> lut;
        //sq256lut.reset(clCtx, MemType::ReadOnly | MemType::HostWriteOnly, lut.size() * 4);
        for (uint16_t i = 0; i < 256; ++i)
            lut[i] = i * i * 65536.0f;
        constexpr auto expectBufSize = 4096 * 128 * 128 * sizeof(uint16_t);
        infoBuf.reset(clCtx, MemFlag::ReadOnly | MemFlag::HostWriteOnly, 4096 * sizeof(FontInfo));
        inputBuf.reset(clCtx, MemFlag::ReadOnly | MemFlag::HostWriteOnly, 4096 * 128 * 128 * sizeof(uint8_t));
        middleBuf.reset(clCtx, MemFlag::ReadWrite | MemFlag::HostReadOnly, 4096 * 128 * 128 * sizeof(uint16_t));
        outputBuf.reset(clCtx, MemFlag::WriteOnly | MemFlag::HostReadOnly, 4096 * 32 * 32 * sizeof(uint8_t));
    }
    
    loadCL(getShaderFromDLL(IDR_SHADER_SDFTEST));
    loadDownSampler(getShaderFromDLL(IDR_SHADER_DWNSAMP));
    testTex.reset();
    testTex->SetProperty(TextureFilterVal::Nearest, TextureWrapVal::Repeat);
}

FontCreator::~FontCreator()
{

}

void FontCreator::reloadFont(const fs::path& fontpath)
{
    ft2.reset(fontpath, 124, 2);
}


void FontCreator::reload(const string& src)
{
    kerSdf.release();
    kerSdfGray.release();
    kerDownSamp.release();
    loadCL(src);
    loadDownSampler(getShaderFromDLL(IDR_SHADER_DWNSAMP));
}

void FontCreator::setChar(char32_t ch, bool custom) const
{
    auto[img, width, height] = ft2->getChBitmap(ch, custom);
    testTex->SetData(TextureInnerFormat::R8, TextureDataFormat::R8, width, height, img.GetRawPtr());
}

Image FontCreator::clgraysdfs(char32_t ch, uint32_t count) const
{
    constexpr uint32_t fontsizelim = 128, fontcountlim = 64, newfontsize = 36;
    const auto fontCount = static_cast<uint16_t>(std::ceil(std::sqrt(count)));
    vector<FontInfo> finfos;
    vector<byte> alldata;
    finfos.reserve(fontCount * fontCount);
    alldata.reserve(fontsizelim * fontsizelim * fontCount * fontCount);
    
    SimpleTimer timer;
    fntLog().verbose(u"raster start at {:%H:%H:%S}\n", SimpleTimer::getCurLocalTime());
    timer.Start();
    size_t offset = 0;
    for (uint32_t a = 0; a < count; ++a)
    {
        auto[img, width, height] = ft2->getChBitmap(ch + a, false);
        if (width > fontsizelim || height > fontsizelim)
        {
            const auto chstr = str::to_u16string(std::u32string(1, ch + a), str::Charset::UTF32);
            fntLog().warning(u"ch {} has invalid size {} x {}\n", chstr, width, height);
            Image tmpimg(std::move(img), width, height, ImageDataType::GRAY);
            const auto ratio = std::max(width, height) * 4 / 128.0f;
            width = std::max(1u, uint32_t(width / ratio) * 4), height = std::max(1u, uint32_t(height / ratio) * 4);
            tmpimg.Resize(width, height);
            img = common::AlignedBuffer<32>(tmpimg.GetSize());
            width = tmpimg.GetWidth(), height = tmpimg.GetHeight();
            memcpy_s(img.GetRawPtr(), img.GetSize(), tmpimg.GetRawPtr(), tmpimg.GetSize());
        }
        if (offset % 16)
            COMMON_THROW(BaseException, u"offset wrong");
        if (width % 4)
            COMMON_THROW(BaseException, u"width wrong");
        if (height % 4)
            COMMON_THROW(BaseException, u"height wrong");
        finfos.push_back(FontInfo{ (uint32_t)offset,(uint8_t)width,(uint8_t)height });
        alldata.insert(alldata.cend(), img.GetRawPtr(), img.GetRawPtr() + img.GetSize());
        offset += img.GetSize();
    }
    timer.Stop();
    fntLog().verbose(u"raster cost {} us\n", timer.ElapseUs());

    fntLog().verbose(u"prepare start at {:%H:%H:%S}\n", SimpleTimer::getCurLocalTime());
    timer.Start();
    inputBuf->Write(clQue, alldata);
    infoBuf->Write(clQue, finfos);
    timer.Stop();
    fntLog().verbose(u"prepare cost {} us\n", timer.ElapseUs());
    if (true)
    {
        fntLog().verbose(u"OpenCL start at {:%H:%H:%S}\n", SimpleTimer::getCurLocalTime());
        timer.Start();
        size_t localsize[] = { fontsizelim / 4 }, worksize[] = { fontsizelim / 4 * count };

        auto pms = kerSdfGray4->Run<1>(clQue, worksize, localsize, false);
        pms->wait();
        timer.Stop();
        fntLog().verbose(u"OpenCl [sdfGray4] cost {}us ({}us by OCL)\n", timer.ElapseUs(), pms->ElapseNs() / 1000);

        vector<uint8_t> clImg;
        middleBuf->Read(clQue, clImg, alldata.size() / 16);

        fntLog().verbose(u"post-merging start at {:%H:%H:%S}\n", SimpleTimer::getCurLocalTime());
        timer.Start();
        Image fin(ImageDataType::GRAY);
        fin.SetSize(newfontsize * fontCount, newfontsize * fontCount, byte(255));
        const uint32_t rowstep = (uint32_t)fin.RowSize();
        uint32_t fidx = 0;
        for (const auto& fi : finfos)
        {
            const uint32_t neww = fi.w / 4, newh = fi.h / 4;
            const auto offsetx = (newfontsize - neww) / 2, offsety = (newfontsize - newh) / 2;
            uint32_t startx = (fidx % fontCount) * newfontsize + offsetx, starty = (fidx / fontCount) * newfontsize + offsety;
            uint8_t * __restrict finPtr = fin.GetRawPtr<uint8_t>(starty, startx);
            const uint8_t * __restrict inPtr = &clImg[fi.offset / 16];
            for (uint32_t row = 0; row < newh; ++row)
            {
                memcpy_s(finPtr, neww, inPtr, neww);
                finPtr += rowstep, inPtr += neww;
            }
            fidx++;
        }
        timer.Stop();
        fntLog().verbose(u"post-process cost {} us\n", timer.ElapseUs());
        return fin;
    }
    else
    {
        fntLog().verbose(u"OpenCL start at {:%H:%H:%S}\n", SimpleTimer::getCurLocalTime());
        timer.Start();
        size_t localsize[] = { fontsizelim }, worksize[] = { fontsizelim * count };

        kerSdfGray->Run<1>(clQue, worksize, localsize, true);
        timer.Stop();
        fntLog().verbose(u"OpenCl cost {} us\n", timer.ElapseUs());
        if (false)
        {
            fntLog().verbose(u"clDownSampler start at {:%H:%H:%S}\n", SimpleTimer::getCurLocalTime());
            timer.Start();
            localsize[0] /= 4, worksize[0] /= 4;
            kerDownSamp->Run<1>(clQue, worksize, localsize, true);
            timer.Stop();
            fntLog().verbose(u"OpenCl[clDownSampler] cost {} us\n", timer.ElapseUs());
            vector<uint8_t> clImg;
            outputBuf->Read(clQue, clImg, alldata.size() / 16);

            fntLog().verbose(u"post-merging start at {:%H:%H:%S}\n", SimpleTimer::getCurLocalTime());
            timer.Start();
            Image fin(ImageDataType::GRAY);
            fin.SetSize(newfontsize * fontCount, newfontsize * fontCount, byte(255));
            const uint32_t rowstep = (uint32_t)fin.RowSize();
            uint32_t fidx = 0;
            for (const auto& fi : finfos)
            {
                const uint32_t neww = fi.w / 4, newh = fi.h / 4;
                const auto offsetx = (newfontsize - neww) / 2, offsety = (newfontsize - newh) / 2;
                uint32_t startx = (fidx % fontCount) * newfontsize + offsetx, starty = (fidx / fontCount) * newfontsize + offsety;
                uint8_t * __restrict finPtr = fin.GetRawPtr<uint8_t>(starty, startx);
                const uint8_t * __restrict inPtr = &clImg[fi.offset / 16];
                for (uint32_t row = 0; row < newh; ++row)
                {
                    memcpy_s(finPtr, neww, inPtr, neww);
                    finPtr += rowstep, inPtr += neww;
                }
                fidx++;
            }
            timer.Stop();
            fntLog().verbose(u"post-process cost {} us\n", timer.ElapseUs());
            return fin;
        }
        else
        {
            vector<int16_t> distsq;
            middleBuf->Read(clQue, distsq, alldata.size());
            fntLog().verbose(u"post-process start at {:%H:%H:%S}\n", SimpleTimer::getCurLocalTime());
            timer.Start();
            Image fin(ImageDataType::GRAY);
            fin.SetSize(newfontsize * fontCount, newfontsize * fontCount, byte(255));
            const uint32_t rowstep = (uint32_t)fin.RowSize();
            uint8_t * __restrict const finPtr = fin.GetRawPtr<uint8_t>();
            uint32_t fidx = 0;
            for (const auto& fi : finfos)
            {
                if (false && (fi.w == 0 || fi.h == 0))
                {
                    const auto idx = ((&fi) - finfos.data()) / sizeof(FontInfo);
                    fntLog().warning(u"index {} get wrong w/h", idx);
                }
                const auto offsetx = (newfontsize - fi.w / 4) / 2, offsety = (newfontsize - fi.h / 4) / 2;
                uint32_t startx = (fidx % fontCount) * newfontsize + offsetx, starty = (fidx / fontCount) * newfontsize + offsety;
                for (uint32_t y = 0, ylim = fi.h / 4; y < ylim; y++)
                {
                    uint32_t opos = (starty + y) * rowstep + startx;
                    size_t ipos = fi.offset + (fi.w * y * 4);
                    for (uint32_t xlim = fi.w / 4; xlim--; ipos += 4, ++opos)
                    {
                        const uint64_t data64[4] = { *(uint64_t*)&distsq[ipos],*(uint64_t*)&distsq[ipos + fi.w] ,*(uint64_t*)&distsq[ipos + 2 * fi.w] ,*(uint64_t*)&distsq[ipos + 3 * fi.w] };
                        const int16_t * __restrict const data16 = (int16_t*)data64;
                        const auto insideCount = (data16[5] < 0 ? 1 : 0) + (data16[6] < 0 ? 1 : 0) + (data16[9] < 0 ? 1 : 0) + (data16[10] < 0 ? 1 : 0);
                        int32_t dists[8];
                        dists[0] = data16[5] * 3 - data16[0], dists[1] = data16[6] * 3 - data16[3], dists[2] = data16[9] * 3 - data16[12], dists[3] = data16[10] * 3 - data16[15];
                        dists[4] = ((data16[5] + data16[9]) * 3 - (data16[4] + data16[8])) / 2, dists[5] = ((data16[6] + data16[10]) * 3 - (data16[7] + data16[11])) / 2;
                        dists[6] = ((data16[5] + data16[6]) * 3 - (data16[1] + data16[2])) / 2, dists[7] = ((data16[9] + data16[10]) * 3 - (data16[13] + data16[14])) / 2;
                        int32_t avg4 = (data16[5] + data16[6] + data16[9] + data16[10]) / 2;
                        int32_t negsum = 0, possum = 0, negcnt = 0, poscnt = 0;
                        for (const auto dist : dists)
                        {
                            if (dist <= 0)
                                negsum += dist, negcnt++;
                            else
                                possum += dist, poscnt++;
                        }
                        if (negcnt == 0)
                            finPtr[opos] = std::clamp(possum / (poscnt * 32) + 128, 0, 255);
                        else if (poscnt == 0)
                            finPtr[opos] = std::clamp(negsum * 3 / (negcnt * 64) + 128, 0, 255);
                        else
                        {
                            const bool isInside = avg4 < 0;
                            const auto distsum = isInside ? ((negsum + avg4) * 4 / (negcnt + 1)) : ((possum + avg4) * 2 / (poscnt + 1));
                            finPtr[opos] = std::clamp(distsum / 64 + 128, 0, 255);
                        }
                    }
                }
                fidx++;
            }
            timer.Stop();
            fntLog().verbose(u"post-process cost {} us\n", timer.ElapseUs());
            return fin;
        }
    }
}

}