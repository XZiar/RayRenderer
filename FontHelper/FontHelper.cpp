#include "FontRely.h"
#include "FontInternal.h"
#include "resource.h"
#include "FontHelper.h"
#include "../OpenCLUtil/oclException.h"
#include "../common/ResourceHelper.h"
#include <cmath>


namespace oglu
{

using namespace oclu;

static string getShaderFromDLL(int32_t id)
{
	std::vector<uint8_t> data;
	if (ResourceHelper::getData(data, L"SHADER", id) != ResourceHelper::Result::Success)
		return "";
	data.push_back('\0');
	return string((const char*)data.data());
}


namespace detail
{


FontViewerProgram::FontViewerProgram()
{
	prog.reset();
	auto shaders = oglShader::loadFromExSrc(getShaderFromDLL(IDR_SHADER_PRINTFONT));
	for (auto shader : shaders)
	{
		try
		{
			shader->compile();
			prog->addShader(std::move(shader));
		}
		catch (OGLException& gle)
		{
			fntLog().error(L"OpenGL compile fail:\n{}\n", gle.message);
			COMMON_THROW(BaseException, L"OpenGL compile fail", std::any(shader));
		}
	}
	try
	{
		prog->link();
		prog->registerLocation({ "vertPos","","vertTexc","vertColor" }, { "","","","","" });
	}
	catch (OGLException& gle)
	{
		fntLog().error(L"Fail to link Program:\n{}\n", gle.message);
		COMMON_THROW(BaseException, L"link Program error");
	}
}


}



oclu::oclContext createOCLContext()
{
	oclPlatform clPlat;
	const auto pltfs = oclUtil::getPlatforms();
	for (const auto& plt : pltfs)
	{
		if (plt->isCurrentGL)
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

SharedResource<oclu::oclContext> FontCreater::clRes(createOCLContext);


FontCreater::FontCreater(const fs::path& fontpath) : ft2(fontpath), clCtx(clRes.get())
{
	{
		for (const auto& dev : clCtx->devs)
			if (dev->type == DeviceType::GPU)
			{
				clQue.reset(clCtx, dev);
				break;
			}
		oclProgram clProg(clCtx, getShaderFromDLL(IDR_SHADER_SDTTEST));
		try
		{
			clProg->build();
		}
		catch (OCLException& cle)
		{
			fntLog().error(L"Fail to build opencl Program:\n{}\n", cle.message);
			COMMON_THROW(BaseException, L"build Program error");
		}
		sdfker = clProg->getKernel("bmpsdf");
	}
	testTex.reset(TextureType::Tex2D);
	testTex->setProperty(TextureFilterVal::Nearest, TextureWrapVal::Repeat);
}

FontCreater::~FontCreater()
{

}

void FontCreater::setChar(wchar_t ch, bool custom) const
{
	auto ret = ft2.getChBitmap(ch, custom);
	auto data = std::move(ret.first);
	uint32_t w, h;
	std::tie(w, h) = ret.second;
	vector<uint32_t> img;
	testTex->setData(TextureInnerFormat::R8, TextureDataFormat::R8, w, h, data);
	vector<uint32_t> tst;
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

void FontCreater::stroke() const
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
	slines = { ft::FreeTyper::SLine{0.f,0.f,(float)w,(float)h} };
	for(uint32_t a=0;a<h;a++)
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

void FontCreater::bmpsdf(wchar_t ch) const
{
	auto ret = ft2.getChBitmap(ch, true);
	auto data = std::move(ret.first);
	uint32_t w, h;
	std::tie(w, h) = ret.second;
	vector<uint16_t> distx(data.size(), 100), distsq(data.size(), 128 * 128);

	/*for (uint32_t y = 0, lineidx = 0; y < h; lineidx = (++y)*w)
	{
		int idx = y * w;
		uint8_t dist = 64, adder = 0, curimg = 0;
		for (int x = 0; x < w; ++x, ++idx, dist += adder)
		{
			if (data[idx] != curimg)
				xdist[idx] = dist = 0, adder = 1, curimg = data[idx];
			else
				xdist[idx] = dist;
		}
		dist = xdist[--idx], adder = 0, curimg = data[idx];
		for (int x = w; x--; --idx, dist += adder)
		{
			if (xdist[idx] != curimg)
				dist = 0, adder = 1, curimg = data[idx];
			else
				xdist[idx] = min(xdist[idx], dist);
		}
	}*/
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
	
	vector<uint8_t> fin;
	for (uint32_t y = 0; y < h; ++y)
	{
		fin.insert(fin.end(), &data[y*w], &data[y*w] +w);
		for (uint32_t x = 0; x < w; ++x)
			//fin.push_back(std::clamp(img[y*w + x] * 16, 0, 255));
			fin.push_back((uint8_t)std::clamp(std::sqrt(distsq[y*w + x]) * 16, 0., 255.));
	}
	testTex->setData(TextureInnerFormat::R8, TextureDataFormat::R8, w*2, h, fin);
}

struct FontInfo 
{
	uint32_t offset;
	uint8_t w, h;
};

void FontCreater::clbmpsdf(wchar_t ch) const
{
	auto ret = ft2.getChBitmap(ch, true);
	auto data = std::move(ret.first);
	uint32_t w, h;
	std::tie(w, h) = ret.second;
	oclBuffer input(clCtx, MemType::ReadOnly, data.size());
	oclBuffer output(clCtx, MemType::ReadWrite, data.size() * sizeof(uint16_t));
	oclBuffer wsize(clCtx, MemType::ReadOnly, sizeof(FontInfo));
	FontInfo finfo[] = { {0,w,h} };
	wsize->write(clQue, finfo);
	sdfker->setArg(0, wsize);
	input->write(clQue, data);
	sdfker->setArg(1, input);
	sdfker->setArg(2, output);
	size_t worksize[] = { 160 };
	sdfker->run<1>(clQue, worksize, worksize, true);
	vector<uint16_t> distsq;
	output->read(clQue, distsq);
	vector<uint8_t> fin((h * 2) * (w * 2));
	for (uint32_t y = 0; y < h; ++y)
	{
		for (uint32_t x = 0; x < w; ++x)
		{
			bool inside = (data[y*w + x] != 0);
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

void FontCreater::clbmpsdfs(wchar_t ch, uint16_t count) const
{
	constexpr auto fontsizelim = 132, fontcountlim = 2;
	vector<FontInfo> finfos;
	vector<uint8_t> alldata;
	finfos.reserve(fontcountlim * fontcountlim);
	alldata.reserve(fontsizelim * fontsizelim * fontcountlim * fontcountlim);
	oclBuffer input(clCtx, MemType::ReadOnly, fontsizelim * fontsizelim * fontcountlim * fontcountlim);
	oclBuffer output(clCtx, MemType::ReadWrite, fontsizelim * fontsizelim * fontcountlim * fontcountlim * sizeof(uint16_t));
	oclBuffer wsize(clCtx, MemType::ReadOnly, sizeof(FontInfo) * fontcountlim * fontcountlim);
	SimpleTimer timer;
	timer.Start();
	size_t offset = 0;
	for (uint16_t a = 0; a < count; ++a)
	{
		auto ret = ft2.getChBitmap(ch + a, true);
		auto data = std::move(ret.first);
		uint32_t w, h;
		std::tie(w, h) = ret.second;
		finfos.push_back(FontInfo{ (uint32_t)offset,(uint8_t)w,(uint8_t)h });
		alldata.insert(alldata.end(), data.cbegin(), data.cend());
		//input->write(clQue, data, offset);
		offset += data.size();
	}
	input->write(clQue, alldata);
	timer.Stop();
	fntLog().verbose(L"prepare cost {} us\n", timer.ElapseUs());
	timer.Start();
	wsize->write(clQue, finfos);
	sdfker->setArg(0, wsize);
	sdfker->setArg(1, input);
	sdfker->setArg(2, output);
	size_t localsize[] = { fontsizelim }, worksize[] = { fontsizelim * count };
	sdfker->run<1>(clQue, worksize, localsize, true);
	timer.Stop();
	fntLog().verbose(L"OpenCl cost {} us\n", timer.ElapseUs());
	vector<uint16_t> distsq;
	output->read(clQue, distsq);
	vector<uint8_t> fin(fontsizelim * fontsizelim * fontcountlim * fontcountlim, 255);
	uint32_t fidx = 0;
	for (auto fi : finfos)
	{
		uint32_t startx = (fidx % fontcountlim) * fontsizelim, starty = (fidx / fontcountlim) * fontsizelim;
		for (uint32_t y = 0; y < fi.h; ++y)
		{
			uint32_t opos = (starty + y) * fontsizelim * fontcountlim + startx;
			uint32_t ipos = fi.offset + (fi.w*y);
			for (uint32_t x = 0; x < fi.w; ++x)
			{
				//fin[opos + x] = (uint8_t)std::clamp(std::sqrt(distsq[ipos + x]) * 16, 0., 255.);
				bool inside = (alldata[ipos + x] != 0);
				auto rawdist = std::sqrt(distsq[ipos + x]);
				//limit to +/-8 pix
				auto truedist = std::clamp(rawdist * 16, 0., 127.);
				auto trueval = (inside ? 127 - truedist : 127 + truedist);
				fin[opos + x] = (uint8_t)trueval;
			}
		}
		fidx++;
	}
	testTex->setData(TextureInnerFormat::R8, TextureDataFormat::R8, fontsizelim * fontcountlim, fontsizelim * fontcountlim, fin);
}

detail::FontViewerProgram& FontViewer::getProgram()
{
	static detail::FontViewerProgram fvProg;
	return fvProg;
}

FontViewer::FontViewer()
{
	using b3d::Point;
	viewVAO.reset(VAODrawMode::Triangles);
	viewRect.reset(BufferType::Array);
	{
		const Point pa({ -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }),
			pb({ 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }),
			pc({ -1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }),
			pd({ 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f });
		Point DatVert[] = { pa,pb,pc, pd,pc,pb };

		viewRect->write(DatVert, sizeof(DatVert));
		viewVAO->setDrawSize(0, 6);
		viewVAO->prepare().set(viewRect, getProgram().prog->Attr_Vert_Pos, sizeof(Point), 2, 0)
			.set(viewRect, getProgram().prog->Attr_Vert_Color, sizeof(Point), 3, sizeof(Vec3))
			.set(viewRect, getProgram().prog->Attr_Vert_Texc, sizeof(Point), 2, 2 * sizeof(Vec3)).end();
	}
	getProgram().prog->useSubroutine("sdfMid");
}

void FontViewer::draw()
{
	getProgram().prog->draw().draw(viewVAO).end();
}

void FontViewer::bindTexture(const oglTexture& tex)
{
	getProgram().prog->globalState().setTexture(tex, "tex").end();
}

}
