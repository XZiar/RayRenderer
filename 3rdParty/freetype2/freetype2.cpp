#include "freetype2.h"
#include "ftException.h"
#include "ft2build.h"
#include FT_FREETYPE_H
#include <freetype/ftoutln.h>

#pragma message("Compile FreeType2 with FreeType2[" STRINGIZE(FREETYPE2_VERSION) "]")


namespace ft
{
using namespace common;

static void* CreateLibrary()
{
    FT_Library library;
    auto error = FT_Init_FreeType(&library);
    if (error)
        COMMON_THROW(BaseException, u"cannot initialize freetype library");
    return library;
}

static FT_LibraryRec_& GetLibrary()
{
    static FT_Library lib = (FT_Library)CreateLibrary();
    return *lib;
}

uint32_t FreeTyper::getGlyphIndex(char32_t ch) const
{
    return FT_Get_Char_Index((FT_Face)face, ch);
}

FreeTyper::FreeTyper(const fs::path& fontpath, const uint16_t pixSize, const uint16_t border) : PixSize(pixSize), Border(border)
{
    auto ret = FT_New_Face(&GetLibrary(), fontpath.string().c_str(), 0, (FT_Face*)&face);
    switch (ret)
    {
    case FT_Err_Ok:
        break;
    case FT_Err_Unknown_File_Format:
        COMMON_THROW(FileException, FileException::Reason::WrongFormat, fontpath, u"freetype does not support this kinds of format");
    case FT_Err_Cannot_Open_Resource:
        COMMON_THROW(FileException, FileException::Reason::OpenFail, fontpath, u"freetype cannot open file");
    default:
        COMMON_THROW(FTException, u"unknown exception while load freetype face");
    }
    FT_Set_Pixel_Sizes((FT_Face)face, PixSize, PixSize);
    //FT_Set_Char_Size((FT_Face)face, 0, height * 64, 96, 96);
}

BMPair FreeTyper::getChBitmap(char32_t ch) const
{
    auto f = (FT_Face)face;
    auto idx = getGlyphIndex(ch);
    FT_GlyphSlot glyph = f->glyph;
    auto ret = FT_Load_Glyph(f, idx, FT_LOAD_DEFAULT);
    if (ret)
        COMMON_THROW(FTException, u"unknown exception while load freetype Glyph");
    FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);
    auto bmp = glyph->bitmap;
    //middle alignment fix and x-px border
    const auto w = ((bmp.width + Border * 2 + 3) / 4) * 4, h = ((bmp.rows + Border * 2 + 3) / 4) * 4;
    const auto offx = (w - bmp.width) / 2, offy = (h - bmp.rows) / 2;
    AlignedBuffer output(w*h, byte(0));
    byte* __restrict outPtr = output.GetRawPtr() + offy * w + offx;
    const auto* __restrict inPtr = bmp.buffer;
    for (uint32_t a = bmp.rows; a--; inPtr += bmp.width, outPtr += w)
        memcpy_s(outPtr, bmp.width, inPtr, bmp.width);
    return { output,w,h };
}


}