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

struct QBLine
{
	float p0x, p0y;
	float p2x, p2y;
	float p1x, p1y;
};

struct SLine
{
	float p0x, p0y;
	float p1x, p1y;
};

inline float dot(float x1, float y1, float x2, float y2)
{
	return x1*x2 + y1*y2;
}

float SignedDistanceSquared(float x, float y, QBLine s, float minDis = 1e20f)
{
	float res[3];

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
		float t = std::clamp(res[j], 0.f, 1.f);
		float dx = (1 - t*t)*s.p0x + 2 * t*(1 - t)*s.p1x + t*t*s.p2x - x;
		float dy = (1 - t*t)*s.p0y + 2 * t*(1 - t)*s.p1y + t*t*s.p2y - y;
		minDis = min(minDis, dx*dx + dy*dy);
	}

	return minDis;
}

float LineDistanceSquared(float x, float y, SLine l)
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

static pair<vector<QBLine>, vector<SLine>> translateQBLine(const vector<ft::FreeTyper::PerStroke>& strokes)
{
	vector<QBLine> qblines;
	vector<SLine> slines;
	int32_t curX = 0, curY = 0;
	for (auto& s : strokes)
	{
		switch (s.type)
		{
		case 'M':
			break;
		case 'L':
			if (curX != s.x || curY != s.y)
				slines.push_back({ (float)curX,(float)curY,(float)s.x,(float)s.y });
			break;
		case 'Q':
			qblines.push_back(QBLine{ (float)curX,(float)curY,(float)s.x,(float)s.y,(float)s.xa,(float)s.ya });
			break;
		}
		curX = s.x, curY = s.y;
	}
	return { qblines,slines };
}

void FontCreater::stroke() const
{
	auto ret = ft2.TryStroke();
	auto w = ret.second.first, h = ret.second.second;
	w = ((w + 3) / 4) * 4;
	vector<uint8_t> data(w*h);
	auto lines = translateQBLine(ret.first);
	lines.first = { QBLine{0.f,0.f,(float)w,(float)h,0.5f*w,.5f*h} };
	lines.second.clear();
	for(uint32_t a=0;a<h;a++)
		for (uint32_t b = 0; b < w; b++)
		{
			float minDist = 1e20f;
			for (auto& qline : lines.first)
			{
				minDist = SignedDistanceSquared(b, a, qline, minDist);
			}
			for (auto& sline : lines.second)
			{
				auto newDist = LineDistanceSquared(b, a, sline);
				minDist = min(minDist, newDist);
			}
			//data[a*w + b] = minDist < 1.0f ? 0 : 255;
			auto dist = sqrt(minDist);

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

	for (uint32_t y = 0, lineidx = 0; y < h; lineidx = (++y)*w)
	{
		uint16_t dist = 100;
		for (uint32_t x = 0; x < w; ++x, ++lineidx, ++dist)
		{
			if (data[lineidx])
				distx[lineidx] = dist = 0;
			else
				distx[lineidx] = dist;
		}
		dist = distx[--lineidx];
		for (uint32_t x = w; x--; --lineidx, ++dist)
		{
			if (data[lineidx])
				dist = 0;
			else
				distx[lineidx] = min(distx[lineidx], dist);
		}
	}
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
	/*for (int32_t x = 0, lineidx = 0; x < w; lineidx = ++x)
	{
		distsq[lineidx] = distx[lineidx] * distx[lineidx];
		int32_t lastdx = distx[lineidx], lastdy = 1;//checkpoint for last nearest point
		lineidx += w;
		for (int32_t y = 1; y < h; ++y, lineidx += w)
		{
			auto& obj = distsq[lineidx];
			int32_t xd = distx[lineidx];
			if (!xd)//0dist,insede
			{
				obj = lastdx = 0; lastdy = 1; continue;
			}
			obj = lastdy*lastdy + lastdx*lastdx;
			auto curxD = (uint16_t)(xd*xd);
			auto dxD = (int32_t)obj - (int32_t)curxD;
			if (dxD < 0)//lastxD < curxD
			{
				if(dxD + 2*(lastdy - xd) < 0)//at least lastline is still better for next line
					lastdy++;
				else//for later lines, curx will be lesser or at least equal, so update
					lastdx = xd, lastdy = 1;
			}
			else//curxD <= lastxD
			{
				obj = curxD;
				//update(for next line,(dy+1)^2+dx^2 = lastd^2+1+2dy > lastd^2+1^2)
				//curline's dx^2 smaller, ofcourse update
				lastdx = xd, lastdy = 1;
			}
		}
		lastdx = distx[(lineidx -= w)], lastdy = 1;//checkpoint for last nearest point
		lineidx -= w;
		for (int32_t y = h-2; y >= 0; --y, lineidx -= w)
		{
			auto& obj = distsq[lineidx];
			int32_t xd = distx[lineidx];
			if (!xd)//0dist,insede
			{
				lastdx = 0; lastdy = 1; continue;
			}
			uint16_t lastxD = lastdy*lastdy + lastdx*lastdx;
			auto curxD = (uint16_t)(xd*xd);

			auto dxD = (int32_t)lastxD - (int32_t)curxD;
			if (dxD < 0)//lastxD < curxD
			{
				if (dxD + 2 * (lastdy - xd) < 0)//at least lastline is still better for next line
					lastdy++;
				else//for later lines, curx will be lesser or at least equal, so update
					lastdx = xd, lastdy = 1;
			}
			else//curxD <= lastxD
			{
				//update(for next line,(dy+1)^2+dx^2 = lastd^2+1+2dy > lastd^2+1^2)
				//curline's dx^2 smaller, ofcourse update
				lastdx = xd, lastdy = 1;
			}
			obj = min(obj, lastxD);
		}
	}*/
	/*for (uint32_t x = 0, lineidx = 0; x < w; lineidx = ++x)
	{
		for (uint32_t y = 0, dist = img[lineidx]; y < h; ++y, lineidx += w, ++dist)
		{
			if (data[lineidx])
				dist = 0;
			else
				dist = img[lineidx] = min((uint32_t)img[lineidx], dist);
		}
		for (uint32_t y = h, dist = img[lineidx -= w]; y--; lineidx -= w, ++dist)
		{
			if (data[lineidx])
				dist = 0;
			else
				dist = img[lineidx] = min((uint32_t)img[lineidx], dist);
		}
	}*/
	
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
	vector<uint8_t> fin;
	for (uint32_t y = 0; y < h; ++y)
	{
		fin.insert(fin.end(), &data[y*w], &data[y*w] + w);
		for (uint32_t x = 0; x < w; ++x)
			fin.push_back((uint8_t)std::clamp(std::sqrt(distsq[y*w + x]) * 16, 0., 255.));
	}
	testTex->setData(TextureInnerFormat::R8, TextureDataFormat::R8, w * 2, h, fin);
}

void FontCreater::clbmpsdfs(wchar_t ch, uint16_t count) const
{
	constexpr auto fontsizelim = 132, fontcountlim = 16;
	vector<FontInfo> finfos;
	finfos.reserve(fontcountlim * fontcountlim);
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
		input->write(clQue, data, offset);
		offset += data.size();
	}
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
				fin[opos + x] = (uint8_t)std::clamp(std::sqrt(distsq[ipos + x]) * 16, 0., 255.);
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
