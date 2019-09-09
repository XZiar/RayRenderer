#include "stblib.h"
#include "common/Exceptions.hpp"
#include "common/SIMD.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"


namespace stb
{

using namespace common;
using common::file::FileException;

static int ReadFile(void *user, char *data, int size)
{
    auto& stream = *static_cast<common::file::FileInputStream*>(user);
    return (int)stream.ReadMany(size, 1, data);
}
static void SkipFile(void *user, int n)
{
    auto& stream = *static_cast<common::file::FileInputStream*>(user);
    stream.Skip(n);
}
static int IsEof(void *user)
{
    auto& stream = *static_cast<common::file::FileInputStream*>(user);
    return stream.IsEnd() ? 1 : 0;
}

static constexpr stbi_io_callbacks FileCallback{ ReadFile, SkipFile, IsEof };

std::tuple<int32_t, int32_t> loadImage(const common::fs::path& fpath, std::vector<uint32_t>& data)
{
    auto stream = file::FileInputStream(file::FileObject::OpenThrow(fpath, file::OpenFlag::READ | file::OpenFlag::BINARY));
    int width, height, comp;
    auto ret = stbi_load_from_callbacks(&FileCallback, &stream, &width, &height, &comp, 4);
    if (ret == nullptr)
        COMMON_THROW(FileException, FileException::Reason::ReadFail, fpath, u"cannot parse image");

    data.resize(width*height);
    memcpy_s(data.data(), data.size() * sizeof(uint32_t), ret, width*height * 4);
    stbi_image_free(ret);
    return{ width,height };
}


std::vector<uint32_t> resizeImage(const std::vector<uint32_t>& input, const uint32_t inw, const uint32_t inh, const uint32_t width, const uint32_t height)
{
    std::vector<uint32_t> output(width*height);
    stbir_resize(input.data(), inw, inh, 0, output.data(), width, height, 0,
        STBIR_TYPE_UINT8, 4, 3, 0,
        STBIR_EDGE_REFLECT, STBIR_EDGE_REFLECT, STBIR_FILTER_TRIANGLE, STBIR_FILTER_TRIANGLE, STBIR_COLORSPACE_LINEAR, nullptr);
    return output;
}


void writeToFile(void *context, void *data, int size)
{
    auto& stream = *static_cast<common::file::FileOutputStream*>(context);
    stream.Write(size, data);
}


void saveImage(const common::fs::path& fpath, const void *data, const uint32_t width, const uint32_t height, const uint8_t compCount)
{
    auto stream = file::FileOutputStream(file::FileObject::OpenThrow(fpath, file::OpenFlag::CreatNewBinary));
    const auto ret = stbi_write_png_to_func(&writeToFile, &stream, (int)width, (int)height, compCount, data, 0);
    if (ret == 0)
        COMMON_THROW(FileException, FileException::Reason::WriteFail, fpath, u"cannot parse image");
}


static std::vector<uint32_t> buildBlock4x4(const std::vector<uint32_t>& input, const uint32_t w, const uint32_t h, uint32_t& idx)
{
    std::vector<uint32_t> ret(input.size() + 64 + 32);
    intptr_t addr = (intptr_t)ret.data();
    uint32_t padding = 32 - (addr % 32);
    idx = 64 + padding;
    const uint32_t *row1 = input.data(), *row2 = &input[w], *row3 = &input[w * 2], *row4 = &input[w * 3];
#if COMMON_SIMD_LV >= 200
    __m256i *dest = (__m256i *)&ret[idx];
#elif COMMON_SIMD_LV >= 20
    __m128i *dest = (__m128i *)&ret[idx];
#else
    uint32_t *dest = &ret[idx];
#endif
    for (uint32_t y = h; y > 0; y -= 4)
    {
        for (uint32_t x = w; x > 0; x -= 4)
        {
        #if COMMON_SIMD_LV >= 200
            _mm256_stream_si256(dest++, _mm256_loadu2_m128i((const __m128i*)row1, (const __m128i*)row2));
            _mm256_stream_si256(dest++, _mm256_loadu2_m128i((const __m128i*)row3, (const __m128i*)row4));
        #elif COMMON_SIMD_LV >= 20
            _mm_stream_si128(dest++, _mm_loadu_si128((const __m128i*)row1));
            _mm_stream_si128(dest++, _mm_loadu_si128((const __m128i*)row2));
            _mm_stream_si128(dest++, _mm_loadu_si128((const __m128i*)row3));
            _mm_stream_si128(dest++, _mm_loadu_si128((const __m128i*)row4));
        #else
            memmove(dest, row1, 16);
            memmove(dest + 4, row2, 16);
            memmove(dest + 8, row3, 16);
            memmove(dest + 12, row4, 16);
            dest += 64;
        #endif
            row1 += 4, row2 += 4, row3 += 4, row4 += 4;
        }
        row1 += 3 * w, row2 += 3 * w, row3 += 3 * w, row4 += 3 * w;
    }
    return ret;
}

std::vector<uint32_t> compressTextureBC1(const std::vector<uint32_t>& input, uint32_t w, uint32_t h)
{
    uint32_t inidx = 0, outidx = 0;
    auto ret = buildBlock4x4(input, w, h, inidx);
    for (; h > 0; h -= 4)
    {
        for (uint32_t px = w; px > 0; inidx += 16, outidx += 4, px -= 4)
        {
            stb_compress_dxt_block((unsigned char*)&ret[outidx], (unsigned char*)&ret[inidx], 0, STB_DXT_HIGHQUAL);
        }
    }
    ret.resize(outidx);
    return ret;
}

std::vector<uint32_t> compressTextureBC2(const std::vector<uint32_t>& input, uint32_t w, uint32_t h)
{
    uint32_t inidx = 0, outidx = 0;
    auto ret = buildBlock4x4(input, w, h, inidx);
    for (; h > 0; h -= 4)
    {
        for (uint32_t px = w; px > 0; inidx += 16, outidx += 4, px -= 4)
        {
            stb_compress_dxt_block((unsigned char*)&ret[outidx], (unsigned char*)&ret[inidx], 1, STB_DXT_HIGHQUAL);
        }
    }
    ret.resize(outidx);
    return ret;
}

}