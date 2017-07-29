#pragma once

#include "ImageUtilRely.h"

#include "common/CommonRely.hpp"
#include "common/FileEx.hpp"
#include "common/Wrapper.hpp"
#include "common/miniLogger/miniLogger.h"

namespace xziar::img
{
common::mlog::logger& ImgLog();

using namespace common::file;
using common::Wrapper;

class _ImgReader;
class _ImgWriter;
class _ImgSupport;

class _ImgReader
{
protected:
public:
	virtual ~_ImgReader() {};
	virtual bool Validate() = 0;
	virtual Image Read() = 0;
};

class _ImgWriter
{
public:
	virtual ~_ImgWriter() {};
};

class _ImgSupport
{
protected:
	_ImgSupport(const wstring& name) : Name(name) {}
public:
	const wstring Name;
	virtual Wrapper<_ImgReader> GetReader(FileObject& file) const = 0;
	virtual Wrapper<_ImgWriter> GetWriter() const = 0;
	virtual bool MatchExtension(const wstring& ext) const = 0;
};

using ImgReader = Wrapper<_ImgReader>;
using ImgWriter = Wrapper<_ImgWriter>;
using ImgSupport = Wrapper<_ImgSupport>;


}