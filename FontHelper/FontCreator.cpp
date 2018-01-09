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

oclu::oclContext createOCLContext()
{
	oclPlatform clPlat;
	const auto pltfs = oclUtil::getPlatforms();
	for (const auto& plt : pltfs)
	{
		if (plt->isNVIDIA)
		{
			clPlat = plt;
			auto clCtx = plt->createContext();
			fntLog().success(L"Created Context in platform {}!\n", plt->name);
			clCtx->onMessage = [](wstring errtxt)
			{
				fntLog().error(L"Error from context:\t{}\n", errtxt);
			};
			return clCtx;
		}
	}
	return oclContext();
}

SharedResource<oclu::oclContext> FontCreator::clRes(createOCLContext);


void FontCreator::loadCL(const string& src)
{
	oclProgram clProg(clCtx, src);
	try
	{
		clProg->build("-cl-fast-relaxed-math -cl-mad-enable -cl-nv-verbose");
		auto log = clProg->getBuildLog(clCtx->devs[0]);
		fntLog().debug(L"nv-buildlog:{}\n", log);
	}
	catch (OCLException& cle)
	{
		fntLog().error(L"Fail to build opencl Program:\n{}\n", cle.message);
		COMMON_THROW(BaseException, L"build Program error");
	}
    kerSdf = clProg->getKernel("bmpsdf");
    kerSdfGray = clProg->getKernel("graysdf");
}

FontCreator::FontCreator(const fs::path& fontpath) : ft2(fontpath), clCtx(clRes.get())
{
	for (const auto& dev : clCtx->devs)
		if (dev->type == DeviceType::GPU)
		{
			clQue.reset(clCtx, dev);
			break;
		}
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

void FontCreator::setChar(wchar_t ch, bool custom) const
{
    auto[img, width, height] = ft2.getChBitmap(ch, custom);
	testTex->setData(TextureInnerFormat::R8, TextureDataFormat::R8, width, height, img.GetRawPtr());
}

int solveCubic(float a, float b, float c, float* r)
{
	float p = b - a*a / 3;
	float q = a * (2 * a*a - 9 * b) / 27 + c;
	float p3 = p*p*p;
	float d = q*q + 4 * p3 / 27;
	float offset = -a / 3;
	if (d >= 0)
	{ // Single solution
		float z = sqrtf(d);
		float u = (-q + z) / 2;
		float v = (-q - z) / 2;
		u = cbrt(u);
		v = cbrt(v);
		r[0] = offset + u + v;
		return 1;
	}
	float u = sqrtf(-p / 3);
	float v = acos(-sqrtf(-27 / p3) * q / 2) / 3;
	float m = cos(v), n = sin(v)*1.732050808f;
	r[0] = offset + u * (m + m);
	r[1] = offset - u * (n + m);
	r[2] = offset + u * (n - m);
	return 3;
}


inline float dot(float x1, float y1, float x2, float y2)
{
	return x1*x2 + y1*y2;
}

float SignedDistanceSquared(float x, float y, ft::FreeTyper::QBLine s, float minDis = 1e20f)
{
	float res[3];

	{
		float d0x = x - s.p0x, d0y = y - s.p0y;
		float d2x = x - s.p2x, d2y = y - s.p2y;
		float d0 = d0x*d0x + d0y*d0y;
		float d2 = d2x*d2x + d2y*d2y;
		minDis = min(minDis, min(d0, d2));
	}

	float Mx = s.p0x - x, My = s.p0y = y;
	float Ax = s.p1x - s.p0x, Ay = s.p1y - s.p0y;
	float Bx = s.p2x - s.p1x - Ax, By = s.p2y - s.p1y - Ay;

	float a = 1.0f / (Bx*Bx + By*By);// 1/dot(B,B);
	float b = 3 * (Ax * Bx + Ay * By);// 3dot(A,B);
	float c = 2 * (Ax * Ax + Ay * Ay) + Mx * Bx + My * By;// 2dot(A,A)+dot(M,B)
	float d = Mx*Ax + My*Ay;// dot(M,A)

	int n = solveCubic(b*a, c*a, d*a, res);

	for (int j = 0; j < n; j++)
	{
		float t = res[j];
		if (t >= 0 && t <= 1)
		{
			float dx = (1 - t)*(1 - t)*s.p0x + 2 * t*(1 - t)*s.p1x + t*t*s.p2x - x;
			float dy = (1 - t)*(1 - t)*s.p0y + 2 * t*(1 - t)*s.p1y + t*t*s.p2y - y;
			minDis = min(minDis, dx*dx + dy*dy);
		}
	}

	return minDis;
}

float LineDistanceSquared(float x, float y, ft::FreeTyper::SLine l)
{
	float cross = (l.p1x - l.p0x) * (x - l.p0x) + (l.p1y - l.p0y) * (y - l.p0y);
	if (cross <= 0)
		return (x - l.p0x) * (x - l.p0x) + (y - l.p0y) * (y - l.p0y);

	float d2 = (l.p1x - l.p0x) * (l.p1x - l.p0x) + (l.p1y - l.p0y) * (l.p1y - l.p0y);
	if (cross >= d2)
		return (x - l.p1x) * (x - l.p1x) + (y - l.p1y) * (y - l.p1y);

	float r = cross / d2;
	float px = l.p0x + (l.p1x - l.p0x) * r;
	float py = l.p0y + (l.p1y - l.p0y) * r;
	return (x - px) * (x - px) + (py - l.p0y) * (py - l.p0y);
}

void FontCreator::stroke() const
{
	auto ret = ft2.TryStroke();
	auto w = ret.second.first, h = ret.second.second;
	w = ((w + 3) / 4) * 4;
	vector<uint8_t> data(w*h);
	auto qlines = ret.first.first;
	auto slines = ret.first.second;
	//qlines = { ft::FreeTyper::QBLine{0.f,0.f,(float)w,0.f,0.5f*w,.5f*h} };
	//slines.clear();
	qlines.clear();
	slines = { ft::FreeTyper::SLine{ 0.f,0.f,(float)w,(float)h } };
	for (uint32_t a = 0; a<h; a++)
		for (uint32_t b = 0; b < w; b++)
		{
			float minDist = 1e20f;
			for (auto& qline : qlines)
			{
				minDist = SignedDistanceSquared(b, a, qline, minDist);
			}
			for (auto& sline : slines)
			{
				auto newDist = LineDistanceSquared(b, a, sline);
				minDist = min(minDist, newDist);
			}
			//data[a*w + b] = minDist < 1.0f ? 0 : 255;
			auto dist = sqrt(minDist);
			//data[a*w + b] = dist < 1.6f ? 0 : 255;
			data[a*w + b] = (uint8_t)std::clamp(dist * 4, 0.f, 255.f);
		}
	testTex->setData(TextureInnerFormat::R8, TextureDataFormat::R8, w, h, data);
}

void FontCreator::bmpsdf(wchar_t ch) const
{
    auto [img, w, h] = ft2.getChBitmap(ch, true);
	vector<uint16_t> distx(img.GetSize(), 100), distsq(img.GetSize(), 128 * 128);

	for (uint32_t x = 0, lineidx = 0; x < w; lineidx = ++x)
	{
		distsq[lineidx] = distx[lineidx] * distx[lineidx];
		for (uint32_t y = 1; y < h; ++y)
		{
			auto testidx = lineidx;
			lineidx += w;
			auto& obj = distsq[lineidx];
			auto xd = distx[lineidx];
			if (!xd)//0dist,insede
			{
				obj = 0; continue;
			}
			obj = xd*xd;
			for (uint32_t dy = 1, maxdy = min((uint32_t)std::ceil(sqrt(obj)), y + 1); dy < maxdy; ++dy, testidx -= w)
			{
				auto oxd = distx[testidx];
				if (!oxd)
				{
					obj = dy*dy;
					//further won't be shorter
					break;
				}
				auto dist = oxd*oxd + dy*dy;
				if (dist < obj)
				{
					obj = dist;
					maxdy = min((uint32_t)std::ceil(sqrt(dist)), maxdy);
				}
			}
		}
		for (uint32_t y = h - 1; y--;)
		{
			auto testidx = lineidx;
			lineidx -= w;
			auto& obj = distsq[lineidx];
			auto xd = distx[lineidx];
			if (!xd)//0dist,insede
				continue;
			for (uint32_t dy = 1, maxdy = min((uint32_t)std::ceil(sqrt(obj)), h - y - 1); dy < maxdy; ++dy, testidx += w)
			{
				auto oxd = distx[testidx];
				if (!oxd)
				{
					obj = dy*dy;
					//further won't be shorter
					break;
				}
				auto dist = oxd*oxd + dy*dy;
				if (dist < obj)
				{
					obj = dist;
					maxdy = min((uint32_t)std::ceil(sqrt(dist)), maxdy);
				}
			}
		}
	}

	vector<byte> fin;
	for (uint32_t y = 0; y < h; ++y)
	{
		fin.insert(fin.end(), &img.GetRawPtr()[y*w], &img.GetRawPtr()[y*w] + w);
		for (uint32_t x = 0; x < w; ++x)
			//fin.push_back(std::clamp(img[y*w + x] * 16, 0, 255));
			fin.push_back(byte((uint8_t)std::clamp(std::sqrt(distsq[y*w + x]) * 16, 0., 255.)));
	}
	testTex->setData(TextureInnerFormat::R8, TextureDataFormat::R8, w * 2, h, fin);
}

struct FontInfo
{
	uint32_t offset;
	uint8_t w, h;
};

void FontCreator::clbmpsdf(wchar_t ch) const
{
    auto[img, w, h] = ft2.getChBitmap(ch, true);
	oclBuffer input(clCtx, MemType::ReadOnly, img.GetSize());
	oclBuffer output(clCtx, MemType::ReadWrite, img.GetSize() * sizeof(uint16_t));
	oclBuffer wsize(clCtx, MemType::ReadOnly, sizeof(FontInfo));
	FontInfo finfo[] = { { 0,w,h } };
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

void FontCreator::clbmpsdfgray(wchar_t ch) const
{
	SimpleTimer timer;
    auto[img, w, h] = ft2.getChBitmap(ch, false);
	oclBuffer input(clCtx, MemType::ReadOnly, img.GetSize());
	oclBuffer output(clCtx, MemType::ReadWrite, img.GetSize() * sizeof(uint16_t));
	oclBuffer wsize(clCtx, MemType::ReadOnly, sizeof(FontInfo));
	FontInfo finfo[] = { { 0,w,h } };
	wsize->write(clQue, finfo);
    kerSdfGray->setArg(0, wsize);
    input->write(clQue, img.GetRawPtr(), img.GetSize());
    kerSdfGray->setArg(1, input);
    kerSdfGray->setArg(2, output);
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

void FontCreator::clbmpsdfs(wchar_t ch, uint16_t count) const
{
	constexpr auto fontsizelim = 136, fontcountlim = 64;
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
    kerSdfGray->setArg(0, wsize);
    kerSdfGray->setArg(1, input);
    kerSdfGray->setArg(2, output);
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


Image FontCreator::clgraysdfs(wchar_t ch, uint16_t count) const
{
	constexpr auto fontsizelim = 136, fontcountlim = 64, newfontsize = 36;
    const auto fontCount = static_cast<uint16_t>(std::ceil(std::sqrt(count)));
	vector<FontInfo> finfos;
	vector<byte> alldata;
	finfos.reserve(fontCount * fontCount);
	alldata.reserve(fontsizelim * fontsizelim * fontCount * fontCount);
	oclBuffer input(clCtx, MemType::ReadOnly, fontsizelim * fontsizelim * fontCount * fontCount);
	oclBuffer output(clCtx, MemType::ReadWrite, fontsizelim * fontsizelim * fontCount * fontCount * sizeof(int16_t));
	oclBuffer wsize(clCtx, MemType::ReadOnly, sizeof(FontInfo) * fontCount * fontCount);
	SimpleTimer timer;
	timer.Start();
	fntLog().verbose(L"raster start at {}\n", timer.getCurTimeTxt());
	size_t offset = 0;
	for (uint16_t a = 0; a < count; ++a)
	{
		auto [img, width, height] = ft2.getChBitmap(ch + a, false);
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
    kerSdfGray->setArg(0, wsize);
    kerSdfGray->setArg(1, input);
    kerSdfGray->setArg(2, output);
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
    Image fin(ImageDataType::GRAY);
    fin.SetSize(newfontsize * fontCount, newfontsize * fontCount, byte(255));
    const auto rowstep = fin.RowSize();
    uint8_t * __restrict const finPtr = fin.GetRawPtr<uint8_t>();
	uint32_t fidx = 0;
	timer.Start();
	fntLog().verbose(L"post-process start at {}\n", timer.getCurTimeTxt());
    int32_t distsum[fontsizelim];
    for (auto fi : finfos)
	{
		uint32_t startx = (fidx % fontCount) * newfontsize + 1, starty = (fidx / fontCount) * newfontsize + 1;
        for (uint32_t y = 0, ylim = fi.h/4; y < ylim; y++)
		{
            uint32_t opos = (starty + y) * rowstep + startx;
            size_t ipos = fi.offset + (fi.w * y * 4);
            for (uint32_t x = 0, xlim = fi.w / 4; x < xlim; ipos += 4)
                distsum[x++] = distsq[ipos + fi.w + 1] * 3 + distsq[ipos + fi.w + 2] * 3 + distsq[ipos + fi.w * 2 + 1] * 3 + distsq[ipos + fi.w * 2 + 2] * 3
                - distsq[ipos] - distsq[ipos + 3] - distsq[ipos + fi.w * 3] - distsq[ipos + fi.w * 3 + 3];
            for (uint32_t i = 0, ilim = fi.w / 4; i < ilim; opos++)
                finPtr[opos] = std::clamp(distsum[i++] / 128 + 128, 0, 255);
		}
		fidx++;
	}
	timer.Stop();
	fntLog().verbose(L"post-process cost {} us\n", timer.ElapseUs());
    return fin;
}

}