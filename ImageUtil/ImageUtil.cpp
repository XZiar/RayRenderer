#include "ImageUtilRely.h"
#include "ImageUtil.h"
#include "ImagePNG.h"
#include "ImageTGA.h"
#include "ImageJPEG.h"
#include "ImageBMP.h"


namespace xziar::img
{
using std::vector;
using namespace common;

static vector<Wrapper<ImgSupport>> SUPPORT_MAP
{
	Wrapper<png::PngSupport>(std::in_place).cast_static<ImgSupport>(),
	Wrapper<tga::TgaSupport>(std::in_place).cast_static<ImgSupport>(),
	Wrapper<jpeg::JpegSupport>(std::in_place).cast_static<ImgSupport>(),
	Wrapper<bmp::BmpSupport>(std::in_place).cast_static<ImgSupport>(),
};

static vector<Wrapper<ImgSupport>> GenerateSupportList(const u16string& ext, const bool allowDisMatch)
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
	ImgLog().debug(u"Read Image {}\n", path.u16string());
    const auto ext = str::ToUpperEng(path.extension().u16string(), common::str::Charset::UTF16LE);
	auto testList = GenerateSupportList(ext, true);
	for (auto& support : testList)
	{
		try
		{
			auto reader = support->GetReader(imgFile);
			if (!reader->Validate())
			{
				reader->Release();
				continue;
			}
			ImgLog().debug(u"Using [{}]\n", support->Name);
			const auto img = reader->Read(dataType);
			reader->Release();
			return img;
		}
		catch (BaseException& be)
		{
			ImgLog().warning(u"Read Image using {} receive error {}\n", support->Name, be.message);
		}
	}
	return Image();
}

void WriteImage(const Image& image, const fs::path & path)
{
	auto imgFile = file::FileObject::OpenThrow(path, file::OpenFlag::WRITE | file::OpenFlag::CREATE | file::OpenFlag::BINARY);
	ImgLog().debug(u"Write Image {}\n", path.u16string());
	const auto ext = str::ToUpperEng(path.extension().u16string(), common::str::Charset::UTF16LE);
	auto testList = GenerateSupportList(ext, false);
	for (auto& support : testList)
	{
		try
		{
			auto writer = support->GetWriter(imgFile);
			ImgLog().debug(u"Using [{}]\n", support->Name);
			return writer->Write(image);
		}
		catch (BaseException& be)
		{
			ImgLog().warning(u"Write Image using {} receive error {}\n", support->Name, be.message);
		}
	}
	return;
}

}

