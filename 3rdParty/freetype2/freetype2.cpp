#include "freetype2.h"
#include "ftException.h"
#include "ft2build.h"
#include FT_FREETYPE_H
#include <freetype/ftoutln.h>

#pragma message("Compiler FreeType2 with FreeType2[" STRINGIZE(FREETYPE2_VERSION) "]")


namespace ft
{
using namespace common;

void* FreeTyper::createLibrary()
{
	FT_Library library;
	auto error = FT_Init_FreeType(&library);
	if (error)
		COMMON_THROW(BaseException, L"cannot initialize freetype library");
	return library;
}

FT_LibraryRec_& FreeTyper::getLibrary()
{
	static FT_Library lib = (FT_Library)createLibrary();
	return *lib;
}

uint32_t FreeTyper::getGlyphIndex(wchar_t ch) const
{
	return FT_Get_Char_Index((FT_Face)face, ch);
}

FreeTyper::FreeTyper(const fs::path& fontpath)
{
	auto ret = FT_New_Face(&getLibrary(), fontpath.string().c_str(), 0, (FT_Face*)&face);
	switch (ret)
	{
	case FT_Err_Ok:
		break;
	case FT_Err_Unknown_File_Format:
		COMMON_THROW(FileException, FileException::Reason::WrongFormat, fontpath, L"freetype does not support this kinds of format");
	case FT_Err_Cannot_Open_Resource:
		COMMON_THROW(FileException, FileException::Reason::OpenFail, fontpath, L"freetype cannot open file");
	default:
		COMMON_THROW(FTException, L"unknown exception while load freetype face");
	}
	int height = 128;
	FT_Set_Pixel_Sizes((FT_Face)face, 128, 128);
	//FT_Set_Char_Size((FT_Face)face, 0, height * 64, 96, 96);
}

BMPair FreeTyper::getChBitmap(wchar_t ch, bool custom) const
{
	auto f = (FT_Face)face;
	auto idx = getGlyphIndex(ch);
	FT_GlyphSlot glyph = f->glyph;
	auto ret = FT_Load_Glyph(f, idx, FT_LOAD_DEFAULT);
	if (ret)
		COMMON_THROW(FTException, L"unknown exception while load freetype Glyph");
	if (!custom)
	{
		FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);
		auto bmp = glyph->bitmap;
		//width alignment fix and 1px border
		const auto w = ((bmp.width + 2 + 3) / 4) * 4, h = ((bmp.rows + 2 + 3) / 4) * 4;
        const auto offx = (w - bmp.width) / 2, offy = (h - bmp.rows) / 2;
        AlignedBuffer<32> output(w*h, byte(0));
        byte* __restrict outPtr = output.GetRawPtr() + offy * w + offx;
        const auto* __restrict inPtr = bmp.buffer;
        for (uint32_t a = bmp.rows; a--; inPtr += bmp.width, outPtr += w)
            memcpy_s(outPtr, bmp.width, inPtr, bmp.width);
		return { output,w,h };
	}
	else
	{
		//auto stks = TryStroke(&glyph->outline);
		auto ret = TryRenderLine(&glyph->outline);
		auto w = ret.second.first, h = ret.second.second;
		w = ((w + 2 + 3) / 4) * 4, h = h + 2;
        AlignedBuffer<32> output(w*h, byte(0));
		auto lines = ret.first;
		for(const auto& l : lines)
		{
            auto pdat = &output.GetRawPtr()[(l.y + 1)*w + (l.x + 1)];
			for (auto a = l.len; a--;)
				*pdat++ = std::byte(0xff);
		}
        return { output,w,h };
	}
}

static void greyspan(int y, int count, const FT_Span *spans, void *user)
{
	intptr_t *ptr = (intptr_t*)user;
	auto& lines = *(vector<FreeTyper::PerLine>*)(ptr[0]);
	auto& maxw = *(int32_t*)(ptr[1]);
	auto& maxh = *(int32_t*)(ptr[2]);
	auto& minw = *(int32_t*)(ptr[3]);
	auto& minh = *(int32_t*)(ptr[4]);
	for (int a = 0; a < count; ++a)
	{
		auto s = spans[a];
		if (s.coverage < 128)
			continue;
		lines.push_back({ s.x,(int16_t)y,s.len });
		minw = min(minw, (int32_t)s.x);
		maxw = max(maxw, (int32_t)(s.x + s.len));
	}
	minh = min(minh, y);
	maxh = max(maxh, y);
}

pair<vector<FreeTyper::PerLine>, pair<uint32_t, uint32_t>> FreeTyper::TryRenderLine(void *outline) const
{
	if (outline == nullptr)
		outline = &((FT_Face)face)->glyph->outline;
	FT_Raster_Params params;
	memset(&params, 0, sizeof(params));
	params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
	params.gray_spans = &greyspan;
	vector<FreeTyper::PerLine> lines;
	int32_t maxw = 0, maxh = 0, minw = 255, minh = 255;
	intptr_t ptr[] = { (intptr_t)&lines,(intptr_t)&maxw,(intptr_t)&maxh,(intptr_t)&minw,(intptr_t)&minh };
	auto t = &ptr;
	params.user = &ptr;
	FT_Outline_Render(&getLibrary(), (FT_Outline*)outline, &params);
	for (auto& l : lines)
		l.x -= minw, l.y = maxh - l.y;
	return { lines,{maxw - minw + 1,maxh - minh + 1} };
}


class Stroker
{
private:
	float curx, cury;
	float minx = 255, miny = 255, maxx = 0, maxy = 0;
public:
	vector<FreeTyper::QBLine> qlines;
	vector<FreeTyper::SLine> slines;
	uint32_t w, h;
	void moveto(const FT_Vector *to)
	{
		curx = to->x / 64.0f, cury = to->y / 64.0f;
		minx = min(minx, curx), miny = min(miny, cury);
		maxx = max(maxx, curx), maxy = max(maxy, cury);
	}
	void lineto(const FT_Vector *to)
	{
		FreeTyper::SLine line{ curx,cury };
		line.p1x = curx = to->x / 64.0f;
		line.p1y = cury = to->y / 64.0f;
		minx = min(minx, curx), miny = min(miny, cury);
		maxx = max(maxx, curx), maxy = max(maxy, cury);
		slines.push_back(line);
	}
	void conicto(const FT_Vector *ctrl, const FT_Vector *to)
	{
		FreeTyper::QBLine line{ curx,cury,ctrl->x / 64.0f,ctrl->y / 64.0f };
		line.p2x = curx = to->x / 64.0f;
		line.p2y = cury = to->y / 64.0f;
		minx = min(minx, curx), miny = min(miny, cury);
		maxx = max(maxx, curx), maxy = max(maxy, cury);
		qlines.push_back(line);
	}
	void relocate()
	{
		for (auto& line : qlines)
			line.p0x -= minx, line.p1x -= minx, line.p2x -= minx, line.p0y -= miny, line.p1y -= miny, line.p2y -= miny;
		for (auto& line : slines)
			line.p0x -= minx, line.p1x -= minx, line.p0y -= miny, line.p1y -= miny;
		w = (uint32_t)(maxx - minx + 1), h = (uint32_t)(maxy - miny + 1);
	}
};

static int moveto(const FT_Vector *to, void *user)
{
	((Stroker*)user)->moveto(to);
	return 0;
}

static int lineto(const FT_Vector *to, void *user)
{
	((Stroker*)user)->lineto(to);
	return 0;
}

static int conicto(const FT_Vector *ctrl, const FT_Vector *to, void *user)
{
	((Stroker*)user)->conicto(ctrl, to);
	return 0;
}

pair<pair<vector<FreeTyper::QBLine>, vector<FreeTyper::SLine>>, pair<uint32_t, uint32_t>> FreeTyper::TryStroke(void *outline) const
{
	Stroker stroker;
	FT_Outline_Funcs cb;
	cb.shift = 0, cb.delta = 0;
	cb.move_to = &moveto;
	cb.line_to = &lineto;
	cb.conic_to = &conicto;
	cb.cubic_to = nullptr;
	FT_Outline_Decompose((FT_Outline*)outline, &cb, &stroker);
	stroker.relocate();
	return { {stroker.qlines,stroker.slines},{stroker.w,stroker.h} };
}

pair<pair<vector<FreeTyper::QBLine>, vector<FreeTyper::SLine>>, pair<uint32_t, uint32_t>> FreeTyper::TryStroke() const
{
	return TryStroke(&((FT_Face)face)->glyph->outline);
}

}