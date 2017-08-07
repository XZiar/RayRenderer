#include "ImageUtilRely.h"
#include "ImageUtil.h"
#include "ImagePNG.h"
#include "ImageTGA.h"
#include "ImageJPEG.h"


namespace xziar::img
{
using std::vector;
using namespace common;

static vector<Wrapper<ImgSupport>> SUPPORT_MAP
{
	Wrapper<png::PngSupport>(std::in_place).cast_static<ImgSupport>(),
	Wrapper<tga::TgaSupport>(std::in_place).cast_static<ImgSupport>(),
	Wrapper<jpeg::JpegSupport>(std::in_place).cast_static<ImgSupport>(),
};

static vector<Wrapper<ImgSupport>> GenerateSupportList(const wstring& ext, const bool allowDisMatch)
{
	vector<Wrapper<ImgSupport>> ret;
	ret.reserve(SUPPORT_MAP.size());
	for (auto& support : SUPPORT_MAP)
	{
		//make supportor with proper extension first
		if (support->MatchExtension(ext))
			ret.insert(ret.cbegin(), support);
		else if (allowDisMatch)
			ret.push_back(support);
	}
	return ret;
}

Image ReadImage(const fs::path& path, const ImageDataType dataType)
{
	auto imgFile = file::FileObject::OpenThrow(path, file::OpenFlag::READ | file::OpenFlag::BINARY);
	ImgLog().debug(L"Read Image {}\n", path.wstring());
	const auto ext = str::ToUpper(path.extension().wstring());
	auto testList = GenerateSupportList(ext, true);
	for (auto& support : testList)
	{
		try
		{
			auto reader = support->GetReader(imgFile);
			if(!reader->Validate())
				continue;
			ImgLog().debug(L"Using [{}]\n", support->Name);
			return reader->Read(dataType);
		}
		catch (BaseException& be)
		{
			ImgLog().warning(L"Read Image using {} receive error {}\n", support->Name, be.message);
		}
	}
	return Image();
}

void WriteImage(const Image& image, const fs::path & path)
{
	auto imgFile = file::FileObject::OpenThrow(path, file::OpenFlag::WRITE | file::OpenFlag::CREATE | file::OpenFlag::BINARY);
	ImgLog().debug(L"Write Image {}\n", path.wstring());
	const auto ext = str::ToUpper(path.extension().wstring());
	auto testList = GenerateSupportList(ext, false);
	for (auto& support : testList)
	{
		try
		{
			auto reader = support->GetWriter(imgFile);
			ImgLog().debug(L"Using [{}]\n", support->Name);
			return reader->Write(image);
		}
		catch (BaseException& be)
		{
			ImgLog().warning(L"Write Image using {} receive error {}\n", support->Name, be.message);
		}
	}
	return;
}

}

