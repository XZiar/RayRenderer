#pragma once
#include <cstdint>
#include <vector>
#include <tuple>
#include <filesystem>
#include "../../common/AlignedContainer.hpp"

struct FT_LibraryRec_;

namespace ft
{
namespace fs = std::experimental::filesystem;
using std::vector;
using std::pair;
class FreeTyper
{
private:
	static void* createLibrary();
	static FT_LibraryRec_& getLibrary();
	void *face;
	uint32_t getGlyphIndex(wchar_t ch) const;
public:
	struct PerLine
	{
		int16_t x, y;
		uint16_t len;
	};
	struct PerStroke
	{
		char type;
		int32_t x, y, xa, ya;
	};
	struct QBLine
	{
		float p0x, p0y;
		float p1x, p1y;
		float p2x, p2y;
	};
	struct SLine
	{
		float p0x, p0y;
		float p1x, p1y;
	};
	FreeTyper(const fs::path& fontpath);
	common::Image<common::ImageType::GREY> getChBitmap(wchar_t ch, bool custom = false) const;
	pair<vector<PerLine>, pair<uint32_t, uint32_t>> TryRenderLine(void *outline = nullptr) const;
	pair<pair<vector<QBLine>, vector<SLine>>, pair<uint32_t, uint32_t>> TryStroke(void *outline) const;
	pair<pair<vector<QBLine>, vector<SLine>>, pair<uint32_t, uint32_t>> TryStroke() const;
};

}