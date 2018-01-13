#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"
#include "DataConvertor.hpp"


namespace xziar::img::jpeg
{
using namespace common;
struct JpegHelper;
namespace detail
{

}

class IMGUTILAPI JpegReader : public ImgReader
{
	friend JpegHelper;
private:
	FileObject & ImgFile;
	common::AlignedBuffer<32> Buffer;
	uint32_t Width, Height;
	void *JpegDecompStruct = nullptr;
	void *JpegSource = nullptr;
	void *JpegErrorHandler = nullptr;
public:
	JpegReader(FileObject& file);
	virtual ~JpegReader() override;
	virtual bool Validate() override;
	virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI JpegWriter : public ImgWriter
{
	friend JpegHelper;
private:
	FileObject& ImgFile;
	common::AlignedBuffer<32> Buffer;
	void *JpegCompStruct = nullptr;
	void *JpegDest = nullptr;
	void *JpegErrorHandler = nullptr;
public:
	JpegWriter(FileObject& file);
	virtual ~JpegWriter() override;;
	virtual void Write(const Image& image) override;
};

class IMGUTILAPI JpegSupport : public ImgSupport
{
public:
	JpegSupport() : ImgSupport(L"Jpeg") {}
	virtual Wrapper<ImgReader> GetReader(FileObject& file) const override { return Wrapper<JpegReader>(file).cast_static<ImgReader>(); }
	virtual Wrapper<ImgWriter> GetWriter(FileObject& file) const override { return Wrapper<JpegWriter>(file).cast_static<ImgWriter>(); }
	virtual bool MatchExtension(const wstring& ext) const override { return ext == L".JPEG" || ext == L".JPG"; }
	virtual bool MatchType(const wstring& type) const override { return type == L"JPEG"; }
};

}