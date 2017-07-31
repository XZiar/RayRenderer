#pragma once

#include "ImageUtilRely.h"
#include "ImageInternal.h"
#include "common/StringEx.hpp"


namespace xziar::img::png
{

using namespace common;

class PngReader : public _ImgReader
{
private:
	FileObject& ImgFile;
	void *PngStruct = nullptr;
	void *PngInfo = nullptr;

	static std::vector<uint8_t*> GetRowPtrs(Image& image, const size_t offset = 0)
	{
		std::vector<uint8_t*> pointers(image.Height, nullptr);
		uint8_t *rawPtr = image.GetRawPtr();
		size_t lineStep = image.ElementSize * image.Width;
		for (auto& ptr : pointers)
			ptr = rawPtr + offset, rawPtr += lineStep;
		return pointers;
	}

	void ReadThrough(uint8_t passes, Image& image);
	void ReadColorToColorAlpha(uint8_t passes, Image& image);
public:
	PngReader(FileObject& file);
	virtual ~PngReader() override;
	virtual bool Validate() override;
	virtual Image Read(const ImageDataType dataType) override;
};

class PngWriter : public _ImgWriter
{

};

class PngSupport : public _ImgSupport
{
public:
	PngSupport() : _ImgSupport(L"Png") {}
	virtual ImgReader GetReader(FileObject& file) const override { return Wrapper<PngReader>(file).cast_static<_ImgReader>(); }
	virtual ImgWriter GetWriter() const override { return (ImgWriter)Wrapper<PngWriter>().cast_static<_ImgWriter>(); }
	virtual bool MatchExtension(const wstring& ext) const override { return ext == L"PNG"; }
	virtual bool MatchType(const wstring& type) const override { return type == L"PNG"; }
};


}