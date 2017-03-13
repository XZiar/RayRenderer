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

}