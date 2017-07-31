#include "ImageUtilRely.h"
#include "ImageUtil.h"
#include "ImageInternal.h"
#include "ImagePNG.h"
#include "common/CommonUtil.hpp"

#include <map>
#include <algorithm>


namespace xziar::img
{
using namespace common::mlog;
logger& ImgLog()
{
	static logger imglog(L"ImageUtil", nullptr, nullptr, LogOutput::Console, LogLevel::Debug);
	return imglog;
}


using namespace common;

static std::vector<ImgSupport> SUPPORT_MAP{ Wrapper<png::PngSupport>(NoArg()).cast_static<_ImgSupport>() };

static std::vector<ImgSupport> GenerateSupportList(const wstring& ext)
{
	std::vector<ImgSupport> ret;
	ret.reserve(SUPPORT_MAP.size());
	for (auto& support : SUPPORT_MAP)
	{
		//make supportor with proper extension first
		if (support->MatchExtension(ext))
			ret.insert(ret.cbegin(), support);
		else
			ret.push_back(support);
	}
	return ret;
}

Image ReadImage(const fs::path& path, const ImageDataType dataType)
{
	auto imgFile = file::FileObject::OpenThrow(path, file::OpenFlag::READ | file::OpenFlag::BINARY);
	ImgLog().debug(L"Read Image {}\n", path.wstring());
	const auto ext = str::ToUpper(path.extension().wstring().substr(1));
	auto testList = GenerateSupportList(ext);
	for (auto& support : testList)
	{
		try
		{
			auto reader = support->GetReader(imgFile);
			if(!reader->Validate())
				continue;
			ImgLog().debug(L"Try [{}]\n", support->Name);
			return reader->Read(dataType);
		}
		catch (BaseException& be)
		{
			ImgLog().warning(L"ReadImage from {} receive error {}\n", support->Name, be.message);
		}
	}
	return Image();
}

}

