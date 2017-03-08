#if defined(__AVX2__) || defined(__AVX__) || defined(__SSE4_2__) || (_M_IX86_FP == 2)
#   define __SSE2__ 1
#endif
#include "stblib.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


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



}