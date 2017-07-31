#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::png
{

using namespace common;

class IMGUTILAPI PngReader : public ImgReader
{
private:
	FileObject& ImgFile;
	void *PngStruct = nullptr;
	void *PngInfo = nullptr;

	void ReadThrough(uint8_t passes, Image& image);
	void ReadColorToColorAlpha(uint8_t passes, Image& image);
public:
	PngReader(FileObject& file);
	virtual ~PngReader() override;
	virtual bool Validate() override;
	virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI PngWriter : public ImgWriter
{
private:
	FileObject& ImgFile;
	void *PngStruct = nullptr;
	void *PngInfo = nullptr;
public:
	PngWriter(FileObject& file);
	virtual ~PngWriter() override;
	virtual void Write(const Image& image) override;
};

class IMGUTILAPI PngSupport : public ImgSupport
{
public:
	PngSupport() : ImgSupport(L"Png") {}
	virtual Wrapper<ImgReader> GetReader(FileObject& file) const override { return Wrapper<PngReader>(file).cast_static<ImgReader>(); }
	virtual Wrapper<ImgWriter> GetWriter(FileObject& file) const override { return Wrapper<PngWriter>(file).cast_static<ImgWriter>(); }
	virtual bool MatchExtension(const wstring& ext) const override { return ext == L".PNG"; }
	virtual bool MatchType(const wstring& type) const override { return type == L"PNG"; }
};


}