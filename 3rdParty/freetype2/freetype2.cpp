#include "freetype2.h"
#include "../../common/Exceptions.hpp"
#include "ft2build.h"
#include FT_FREETYPE_H

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

FreeTyper::FreeTyper(const fs::path& fontpath)
{

}

}