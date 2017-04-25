#include "freetype2.h"
#include "ftException.h"
#include "ft2build.h"
#include FT_FREETYPE_H
#include <freetype/ftoutln.h>

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
		COMMON_THROW(BaseException, L"unknown exception while load freetype face");
	}
	int height = 128;
	FT_Set_Pixel_Sizes((FT_Face)face, 128, 128);
	//FT_Set_Char_Size((FT_Face)face, 0, height * 64, 96, 96);
}

pair<vector<uint8_t>, pair<uint32_t, uint32_t>> FreeTyper::getChBitmap(wchar_t ch, bool custom) const
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
		//width alignment fix
		auto w = ((bmp.width + 3) / 4) * 4;
		vector<uint8_t> dat(w*bmp.rows, 0);
		for (uint32_t a = 0; a < bmp.rows; a++)
			memmove(dat.data() + a*w, bmp.buffer + a*bmp.width, bmp.width);
		return { dat,{w,bmp.rows} };
	}
	else
	{
		//auto stks = TryStroke(&glyph->outline);
		auto ret = TryRenderLine(&glyph->outline);
		auto w = ret.second.first, h = ret.second.second;
		w = ((w + 3) / 4) * 4;
		vector<uint8_t> dat(w*h, 0);
		auto lines = ret.first;
		for(const auto& l : lines)
		{
			auto pdat = &dat[l.y*w + l.x];
			for (auto a = l.len; a--;)
				*pdat++ = 255;
		}
		return { dat,{ w,h } };
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
	return { lines,{maxw - minw,maxh - minh} };
}

static int moveto(const FT_Vector *to, void *user)
{
	auto& strokes = *(vector<FreeTyper::PerStroke>*)user;
	strokes.push_back({ 'M',to->x >> 6,to->y >> 6,0,0 });
	return 0;
}

static int lineto(const FT_Vector *to, void *user)
{
	auto& strokes = *(vector<FreeTyper::PerStroke>*)user;
	strokes.push_back({ 'L',to->x >> 6,to->y >> 6,0,0 });
	return 0;
}

static int conicto(const FT_Vector *ctrl, const FT_Vector *to, void *user)
{
	auto& strokes = *(vector<FreeTyper::PerStroke>*)user;
	strokes.push_back({ 'Q',to->x >> 6,to->y >> 6,ctrl->x >> 6,ctrl->y >> 6 });
	return 0;
}

pair<vector<FreeTyper::PerStroke>, pair<uint32_t, uint32_t>> FreeTyper::TryStroke(void *outline) const
{
	vector<FreeTyper::PerStroke> strokes;
	FT_Outline_Funcs cb;
	cb.shift = 0, cb.delta = 0;
	cb.move_to = &moveto;
	cb.line_to = &lineto;
	cb.conic_to = &conicto;
	cb.cubic_to = nullptr;
	FT_Outline_Decompose((FT_Outline*)outline, &cb, &strokes);
	int32_t maxw = 0, maxh = 0, minw = 65536, minh = 65536;
	for (auto& ps : strokes)
		maxw = max(maxw, ps.x), maxh = max(maxh, ps.y), minw = min(minw, ps.x), minh = min(minh, ps.y);
	for (auto& ps : strokes)
		ps.x -= minw, ps.y -= minh, ps.xa -= minw, ps.ya -= minh;
	return { strokes,{maxw - minw,maxh - minh} };
}

pair<vector<FreeTyper::PerStroke>, pair<uint32_t, uint32_t>> FreeTyper::TryStroke() const
{
	return TryStroke(&((FT_Face)face)->glyph->outline);
}

}