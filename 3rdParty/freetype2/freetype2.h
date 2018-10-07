#pragma once
#include "common/AlignedContainer.hpp"
#include "common/Exceptions.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <tuple>


#define FREETYPE2_VERSION PPCAT(PPCAT(FREETYPE_MAJOR,PPCAT(.,FREETYPE_MINOR)),PPCAT(.,FREETYPE_PATCH))


namespace ft
{
namespace fs = common::fs;
using std::vector;
using std::pair;
using std::tuple;
using std::byte;
using BMPair = tuple<common::AlignedBuffer, uint32_t, uint32_t>;

class FreeTyper
{
private:
    void *face;
    const uint16_t PixSize, Border;
    uint32_t getGlyphIndex(char32_t ch) const;
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
    FreeTyper(const fs::path& fontpath, const uint16_t pixSize = 128, const uint16_t border = 2);
    BMPair getChBitmap(char32_t ch, bool custom = false) const;
    pair<vector<PerLine>, pair<uint32_t, uint32_t>> TryRenderLine(void *outline = nullptr) const;
    pair<pair<vector<QBLine>, vector<SLine>>, pair<uint32_t, uint32_t>> TryStroke(void *outline) const;
    pair<pair<vector<QBLine>, vector<SLine>>, pair<uint32_t, uint32_t>> TryStroke() const;
};

}