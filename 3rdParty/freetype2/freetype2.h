#pragma once
#include "common/AlignedBuffer.hpp"
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
    FreeTyper(const fs::path& fontpath, const uint16_t pixSize = 128, const uint16_t border = 2);
    BMPair getChBitmap(char32_t ch) const;
};

}