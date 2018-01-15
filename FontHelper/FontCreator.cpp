#include "FontRely.h"
#include "FontCreator.h"
#include "../OpenCLUtil/oclException.h"
#include <cmath>
#include "resource.h"


namespace oglu
{
using namespace oclu;
using xziar::img::Image;
using xziar::img::ImageDataType;

auto FindPlatform(const std::vector<oclPlatform>& platforms)
{
	for (const auto& plt : platforms)
		if (plt->vendor == Vendor::NVIDIA)
			return plt;
	for (const auto& plt : platforms)
		if (plt->vendor == Vendor::AMD)
			return plt;
	for (const auto& plt : platforms)
		if (plt->vendor == Vendor::Intel)
			return plt;
	return oclPlatform();
}

oclu::oclContext createOCLContext()
{
	oclPlatform clPlat = FindPlatform(oclUtil::getPlatforms());
	if (!clPlat)
		return oclContext();
	auto clCtx = clPlat->createContext();
	fntLog().success(L"Created Context in platform {}!\n", clPlat->name);
	clCtx->onMessage = [](const wstring& errtxt)
	{
		fntLog().error(L"Error from context:\t{}\n", errtxt);
	};
	return clCtx;
}

SharedResource<oclu::oclContext> FontCreator::clRes(createOCLContext);


void FontCreator::loadCL(const string& src)
{
	oclProgram clProg(clCtx, src);
	try
	{
		string options = clCtx->vendor == Vendor::NVIDIA ? "-cl-fast-relaxed-math -cl-nv-verbose -DNVIDIA" : "-cl-fast-relaxed-math";
		options += " -DLOC_MEM_SIZE=" + std::to_string(clCtx->devs[0]->LocalMemSize);
		clProg->build(options);
		const auto log = clProg->getBuildLog(clCtx->devs[0]);
		fntLog().debug(L"buildlog:{}\n", log);
	}
	catch (OCLException& cle)
	{
		fntLog().error(L"Fail to build opencl Program:\n{}\n", cle.message);
		COMMON_THROW(BaseException, L"build Program error");
	}
    kerSdf = clProg->getKernel("bmpsdf");
    kerSdfGray = clProg->getKernel("graysdf");
	{
		const auto wgInfo = kerSdfGray->GetWorkGroupInfo(clCtx->devs[0]);
	}
	//prepare LUT
	{
		std::array<float, 256> lut{ 0 };
		sq256lut.reset(clCtx, MemType::ReadOnly, lut.size() * 4);
		for (uint16_t i = 0; i < 256; ++i)
			lut[i] = i * i * 65536.0f;
		sq256lut->write(clQue, lut.data(), lut.size() * sizeof(float));
		kerSdfGray->setArg(0, sq256lut);
	}
}

FontCreator::FontCreator(const fs::path& fontpath) : ft2(fontpath, 124, 2)
{
	clCtx = clRes.get();
	for (const auto& dev : clCtx->devs)
		if (dev->type == DeviceType::GPU)
		{
			clQue.reset(clCtx, dev);
			break;
		}
	if(!clQue)
		COMMON_THROW(BaseException, L"clQueue initialized failed! There may be no GPU device found in platform");
	loadCL(getShaderFromDLL(IDR_SHADER_SDTTEST));
	testTex.reset(TextureType::Tex2D);
	testTex->setProperty(TextureFilterVal::Nearest, TextureWrapVal::Repeat);
}

FontCreator::~FontCreator()
{

}

void FontCreator::reload(const string& src)
{
	loadCL(src);
}

void FontCreator::setChar(char32_t ch, bool custom) const
{
    auto[img, width, height] = ft2.getChBitmap(ch, custom);
	testTex->setData(TextureInnerFormat::R8, TextureDataFormat::R8, width, height, img.GetRawPtr());
}

struct FontInfo
{
	uint32_t offset;
	uint8_t w, h;
};

void FontCreator::clbmpsdf(char32_t ch) const
{
    auto[img, w, h] = ft2.getChBitmap(ch, true);
	oclBuffer input(clCtx, MemType::ReadOnly, img.GetSize());
	oclBuffer output(clCtx, MemType::ReadWrite, img.GetSize() * sizeof(uint16_t));
	oclBuffer wsize(clCtx, MemType::ReadOnly, sizeof(FontInfo));
	FontInfo finfo[] = { { 0,(uint8_t)w,(uint8_t)h } };
	wsize->write(clQue, finfo);
    kerSdf->setArg(0, wsize);
	input->write(clQue, img.GetRawPtr(), img.GetSize());
    kerSdf->setArg(1, input);
    kerSdf->setArg(2, output);
	size_t worksize[] = { 160 };
    kerSdf->run<1>(clQue, worksize, worksize, true);
	vector<uint16_t> distsq;
	output->read(clQue, distsq);
	vector<uint8_t> fin((h * 2) * (w * 2));
	for (uint32_t y = 0; y < h; ++y)
	{
		for (uint32_t x = 0; x < w; ++x)
		{
			bool inside = (img.GetRawPtr<uint8_t>()[y*w + x] != 0);
			fin[y*(w * 2) + x] = inside ? 0 : 255;
			auto rawdist = std::sqrt(distsq[y*w + x]);
			fin[y*(w * 2) + w + x] = (uint8_t)rawdist * 2;
			//limit to +/-8 pix
			auto truedist = std::clamp(rawdist * 16, 0., 127.);
			fin[(h + y)*(w * 2) + x] = (uint8_t)truedist * 2;
			auto trueval = (inside ? 127 - truedist : 127 + truedist);
			fin[(h + y)*(w * 2) + w + x] = (uint8_t)trueval;
		}
	}
	testTex->setData(TextureInnerFormat::R8, TextureDataFormat::R8, w * 2, h * 2, fin);
}

void FontCreator::clbmpsdfgray(char32_t ch) const
{
	SimpleTimer timer;
    auto[img, w, h] = ft2.getChBitmap(ch, false);
	oclBuffer input(clCtx, MemType::ReadOnly, img.GetSize());
	oclBuffer output(clCtx, MemType::ReadWrite, img.GetSize() * sizeof(uint16_t));
	oclBuffer wsize(clCtx, MemType::ReadOnly, sizeof(FontInfo));
	FontInfo finfo[] = { { 0,(uint8_t)w,(uint8_t)h } };
	wsize->write(clQue, finfo);
	kerSdfGray->setArg(1, wsize);
    input->write(clQue, img.GetRawPtr(), img.GetSize());
    kerSdfGray->setArg(2, input);
    kerSdfGray->setArg(3, output);
	size_t worksize[] = { 160 };
	timer.Start();
    kerSdfGray->run<1>(clQue, worksize, worksize, true);
	timer.Stop();
	fntLog().verbose(L"grey OpenCl cost {} us\n", timer.ElapseUs());
	vector<int16_t> distsq;
	output->read(clQue, distsq);
	vector<uint8_t> bdata;
    for (size_t cnt = 0; cnt < img.GetSize(); ++cnt)
        bdata.push_back((uint8_t)img[cnt] > 127 ? 255 : 0);
	input->write(clQue, bdata);
    kerSdf->setArg(0, wsize);
    kerSdf->setArg(1, input);
    kerSdf->setArg(2, output);
    kerSdf->run<1>(clQue, worksize, worksize, true);
	vector<uint16_t> distsq2;
	output->read(clQue, distsq2);
	vector<uint8_t> fin((h * 2) * (w * 2));
	for (uint32_t y = 0; y < h; ++y)
	{
		for (uint32_t x = 0; x < w; ++x)
		{
			fin[y*(w * 2) + x] = (uint8_t)img[y*w + x];
			bool isOutside = (fin[y*(w * 2) + w + x] = bdata[y*w + x]) > 0;
			//limit to +/-8 pix
			auto truedist = std::clamp(((int)distsq[y*w + x] + 8 * 256), 0, 16 * 256 - 1);
			fin[(h + y)*(w * 2) + x] = (uint8_t)(truedist / 16);
			//auto trueval = (inside ? 127 - truedist : 127 + truedist);
			//fin[(h + y)*(w * 2) + w + x] = (uint8_t)trueval;
			auto rawdist2 = std::sqrt(distsq2[y*w + x]);
			auto truedist2 = std::clamp(rawdist2 * 16, 0., 127.);
			fin[(h + y)*(w * 2) + w + x] = isOutside ? (uint8_t)(128 - truedist2) : (uint8_t)(truedist2 + 128);
		}
	}
	testTex->setData(TextureInnerFormat::R8, TextureDataFormat::R8, w * 2, h * 2, fin);
}

void FontCreator::clbmpsdfs(char32_t ch, uint32_t count) const
{
	constexpr size_t fontsizelim = 136, fontcountlim = 64;
	vector<FontInfo> finfos;
	vector<byte> alldata;
	finfos.reserve(fontcountlim * fontcountlim);
	alldata.reserve(fontsizelim * fontsizelim * fontcountlim * fontcountlim);
	oclBuffer input(clCtx, MemType::ReadOnly, fontsizelim * fontsizelim * fontcountlim * fontcountlim);
	oclBuffer output(clCtx, MemType::ReadWrite, fontsizelim * fontsizelim * fontcountlim * fontcountlim * sizeof(int16_t));
	oclBuffer wsize(clCtx, MemType::ReadOnly, sizeof(FontInfo) * fontcountlim * fontcountlim);
	SimpleTimer timer;
	timer.Start();
	fntLog().verbose(L"raster start at {}\n", timer.getCurTimeTxt());
	size_t offset = 0;
	for (uint16_t a = 0; a < count; ++a)
	{
        auto[img, width, height] = ft2.getChBitmap(ch + a, false);
        finfos.push_back(FontInfo{ (uint32_t)offset,(uint8_t)width,(uint8_t)height });
        alldata.insert(alldata.cend(), img.GetRawPtr(), img.GetRawPtr() + img.GetSize());
        offset += img.GetSize();
	}
	timer.Stop();
	fntLog().verbose(L"raster cost {} us\n", timer.ElapseUs());
	timer.Start();
	fntLog().verbose(L"prepare start at {}\n", timer.getCurTimeTxt());
	input->write(clQue, alldata);
	wsize->write(clQue, finfos);
    kerSdfGray->setArg(1, wsize);
    kerSdfGray->setArg(2, input);
    kerSdfGray->setArg(3, output);
	timer.Stop();
	fntLog().verbose(L"prepare cost {} us\n", timer.ElapseUs());
	timer.Start();
	fntLog().verbose(L"OpenCL start at {}\n", timer.getCurTimeTxt());
	size_t localsize[] = { fontsizelim }, worksize[] = { fontsizelim * count };
    kerSdfGray->run<1>(clQue, worksize, localsize, true);
	timer.Stop();
	fntLog().verbose(L"OpenCl cost {} us\n", timer.ElapseUs());
	vector<int16_t> distsq;
	output->read(clQue, distsq);
	vector<uint8_t> fin(fontsizelim * fontsizelim * fontcountlim * fontcountlim, 255);
	uint32_t fidx = 0;
	timer.Start();
	fntLog().verbose(L"post-process start at {}\n", timer.getCurTimeTxt());
	for (auto fi : finfos)
	{
		uint32_t startx = (fidx % fontcountlim) * fontsizelim, starty = (fidx / fontcountlim) * fontsizelim;
		for (uint32_t y = 0; y < fi.h; ++y)
		{
			uint32_t opos = (starty + y) * fontsizelim * fontcountlim + startx;
			uint32_t ipos = fi.offset + (fi.w*y);
			for (uint32_t x = 0; x < fi.w; ++x)
			{
				auto truedist = std::clamp(((int)distsq[ipos + x] + 8 * 256), 0, 16 * 256 - 1);
				fin[opos + x] = (uint8_t)(truedist / 16);
			}
		}
		fidx++;
	}
	timer.Stop();
	fntLog().verbose(L"post-process cost {} us\n", timer.ElapseUs());
	testTex->setData(TextureInnerFormat::R8, TextureDataFormat::R8, fontsizelim * fontcountlim, fontsizelim * fontcountlim, fin);
}


Image FontCreator::clgraysdfs(char32_t ch, uint32_t count) const
{
	constexpr size_t fontsizelim = 128, fontcountlim = 64, newfontsize = 36;
    const auto fontCount = static_cast<uint16_t>(std::ceil(std::sqrt(count)));
	vector<FontInfo> finfos;
	vector<byte> alldata;
	finfos.reserve(fontCount * fontCount);
	alldata.reserve(fontsizelim * fontsizelim * fontCount * fontCount);
	
	SimpleTimer timer;
	fntLog().verbose(L"raster start at {}\n", timer.getCurTimeTxt());
	timer.Start();
	size_t offset = 0;
	for (uint32_t a = 0; a < count; ++a)
	{
		auto[img, width, height] = ft2.getChBitmap(ch + a, false);
		if (width > 128 || height > 128)
		{
			const auto chstr = *(std::wstring*)&str::to_u16string(std::u32string(1, ch + a), str::Charset::UTF32);
			fntLog().warning(L"ch {} has invalid size {} x {}\n", chstr, width, height);
			Image tmpimg(std::move(img), width, height, ImageDataType::GRAY);
			const auto ratio = std::max(width, height) * 4 / 128.0f;
			width = std::max(1u, uint32_t(width / ratio) * 4), height = std::max(1u, uint32_t(height / ratio) * 4);
			tmpimg.Resize(width, height);
			img = common::AlignedBuffer<32>(tmpimg.GetSize());
			width = tmpimg.Width, height = tmpimg.Height;
			memcpy_s(img.GetRawPtr(), img.GetSize(), tmpimg.GetRawPtr(), tmpimg.GetSize());
		}
		finfos.push_back(FontInfo{ (uint32_t)offset,(uint8_t)width,(uint8_t)height });
		alldata.insert(alldata.cend(), img.GetRawPtr(), img.GetRawPtr() + img.GetSize());
		offset += img.GetSize();
	}
	timer.Stop();
	fntLog().verbose(L"raster cost {} us\n", timer.ElapseUs());

	fntLog().verbose(L"prepare start at {}\n", timer.getCurTimeTxt());
	timer.Start();
	oclBuffer input(clCtx, MemType::ReadOnly, alldata.size());
	oclBuffer output(clCtx, MemType::ReadWrite, alldata.size() * sizeof(int16_t));
	oclBuffer wsize(clCtx, MemType::ReadOnly, sizeof(FontInfo) * fontCount * fontCount); 
	input->write(clQue, alldata);
	wsize->write(clQue, finfos);
    kerSdfGray->setArg(1, wsize);
    kerSdfGray->setArg(2, input);
    kerSdfGray->setArg(3, output);
	timer.Stop();
	fntLog().verbose(L"prepare cost {} us\n", timer.ElapseUs());

	fntLog().verbose(L"OpenCL start at {}\n", timer.getCurTimeTxt());
	timer.Start();
	size_t localsize[] = { fontsizelim }, worksize[] = { fontsizelim * count };
    kerSdfGray->run<1>(clQue, worksize, localsize, true);
	timer.Stop();
	fntLog().verbose(L"OpenCl cost {} us\n", timer.ElapseUs());

	vector<int16_t> distsq;
	output->read(clQue, distsq);
    Image fin(ImageDataType::GRAY);
    fin.SetSize(newfontsize * fontCount, newfontsize * fontCount, byte(255));
    const uint32_t rowstep = (uint32_t)fin.RowSize();
    uint8_t * __restrict const finPtr = fin.GetRawPtr<uint8_t>();
	uint32_t fidx = 0;
	fntLog().verbose(L"post-process start at {}\n", timer.getCurTimeTxt());
	timer.Start();
    for (const auto& fi : finfos)
	{
		if (false && (fi.w == 0 || fi.h == 0))
		{
			const auto idx = ((&fi) - finfos.data()) / sizeof(FontInfo);
			fntLog().warning(L"index {} get wrong w/h", idx);
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
	fntLog().verbose(L"post-process cost {} us\n", timer.ElapseUs());
	return fin;
}

}