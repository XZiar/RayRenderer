#include "ImageUtilRely.h"
#include "ImageUtil.h"
#include "ImageInternal.h"
#include "ImagePNG.hpp"
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

static const std::map<ImageType, ImgSupport> SUPPORT_MAP{ { ImageType::PNG, png::PngSupport(NoArg()).cast_static<_ImgSupport>() } };

static std::vector<ImgSupport> GenerateSupportList(const wstring& ext)
{
	std::vector<ImgSupport> ret;
	ret.reserve(SUPPORT_MAP.size());
	for (auto& pair : SUPPORT_MAP)
	{
		if (pair.second->MatchExtension(ext))
			ret.insert(ret.cbegin(), pair.second);
		else
			ret.push_back(pair.second);
	}
	return ret;
}

Image ReadImage(const fs::path& path)
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
			return reader->Read();
		}
		catch (BaseException& be)
		{
			ImgLog().warning(L"ReadImage from {} receive error {}\n", support->Name, be.message);
		}
	}
	return Image();
}

}

