#if defined(__AVX2__) || defined(__AVX__) || defined(__SSE4_2__) || (_M_IX86_FP == 2)
#   define __SSE2__ 1
#endif
#include "stblib.h"

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


std::tuple<int32_t, int32_t> loadImage(const std::wstring& fname, std::vector<uint32_t>& data)
{
	FILE *fp = nullptr;
	if (_wfopen_s(&fp, fname.c_str(), L"rb") != 0)
		throw std::ios_base::failure("cannot open file");
	int width, height, comp;
	auto ret = stbi_load_from_file(fp, &width, &height, &comp, 4);
	if (ret == nullptr)
	{
		fclose(fp);
		throw std::ios_base::failure("cannot open file");
	}
	data.resize(width*height);
	memmove(data.data(), ret, width*height * 4);
	stbi_image_free(ret);
	fclose(fp);
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
	FILE *fp = (FILE*)context;
	fwrite(data, size, 1, fp);
}

void saveImage(const std::wstring& fname, const std::vector<uint32_t>& data, const uint32_t width, const uint32_t height)
{
	FILE *fp = nullptr;
	if (_wfopen_s(&fp, fname.c_str(), L"wb") != 0)
		throw std::ios_base::failure("cannot open file");
	const auto ret = stbi_write_png_to_func(&writeToFile, fp, (int)width, (int)height, 4, data.data(), 0);
	fclose(fp);
	if (ret == 0)
		throw std::ios_base::failure("write failure");
}


static std::vector<uint32_t> buildBlock4x4(const std::vector<uint32_t>& input, const uint32_t w, const uint32_t h, uint32_t& idx)
{
	std::vector<uint32_t> ret(input.size() + 64 + 32);
	intptr_t addr = (intptr_t)ret.data();
	uint32_t padding = 32 - (addr % 32);
	idx = 64 + padding;
	const uint32_t *row1 = input.data(), *row2 = &input[w], *row3 = &input[w * 2], *row4 = &input[w * 3];
#if defined(__AVX2__)
	__m256i *dest = (__m256i *)&ret[idx];
#elif defined(__SSE2__)
	__m128i *dest = (__m128i *)&ret[idx];
#else
	uint32_t *dest = &ret[idx];
#endif
	for (uint32_t y = h; y > 0; y -= 4)
	{
		for (uint32_t x = w; x > 0; x -= 4)
		{
		#if defined(__AVX2__)
			_mm256_stream_si256(dest++, _mm256_loadu2_m128i((const __m128i*)row1, (const __m128i*)row2));
			_mm256_stream_si256(dest++, _mm256_loadu2_m128i((const __m128i*)row3, (const __m128i*)row4));
		#elif defined(__SSE2__)
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