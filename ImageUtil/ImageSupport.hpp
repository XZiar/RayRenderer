#pragma once

#include "ImageUtilRely.h"
#include "ImageCore.h"


namespace xziar::img
{
using common::file::BufferedFileReader;
using common::file::FileObject;
using common::Wrapper;


class IMGUTILAPI ImgReader
{
protected:
public:
	virtual ~ImgReader() {}
	virtual bool Validate() = 0;
	virtual Image Read(const ImageDataType dataType) = 0;
	virtual void Release() {}
};

class IMGUTILAPI ImgWriter
{
public:
	virtual ~ImgWriter() {};
	virtual void Write(const Image& image) = 0;
};

class IMGUTILAPI ImgSupport
{
protected:
	ImgSupport(const wstring& name) : Name(name) {}
public:
	const wstring Name;
	virtual Wrapper<ImgReader> GetReader(FileObject& file) const = 0;
	virtual Wrapper<ImgWriter> GetWriter(FileObject& file) const = 0;
	virtual bool MatchExtension(const wstring& ext) const = 0;
	virtual bool MatchType(const wstring& type) const = 0;
};


}