#pragma once
#include "common/AlignedBuffer.hpp"
#include "common/FileBase.hpp"
#include "common/Exceptions.hpp"
#include <cstddef>
#include <cstdint>
#include <tuple>


#define FREETYPE2_VERSION PPCAT(PPCAT(FREETYPE_MAJOR,PPCAT(.,FREETYPE_MINOR)),PPCAT(.,FREETYPE_PATCH))



namespace ft
{

class [[nodiscard]] FTException : public common::BaseException
{
    PREPARE_EXCEPTION(FTException, BaseException,
        ExceptionInfo(const std::u16string_view msg) : TPInfo(TYPENAME, msg) { }
    );
    FTException(const std::u16string_view msg) : BaseException(T_<ExceptionInfo>{}, msg) {}
};


using BMPair = std::tuple<common::AlignedBuffer, uint32_t, uint32_t>;

class FreeTyper
{
private:
    void *face;
    const uint16_t PixSize, Border;
    uint32_t getGlyphIndex(char32_t ch) const;
public:
    FreeTyper(const common::fs::path& fontpath, const uint16_t pixSize = 128, const uint16_t border = 2);
    BMPair getChBitmap(char32_t ch) const;
};

}