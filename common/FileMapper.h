#pragma once
#error FileMapper not correctly implemented

#include "CommonRely.hpp"
#include "Wrapper.hpp"
#include <cstdint>
#include <string>
#include <map>

namespace common
{

enum class FileAccess : uint8_t { None = 0x0, Read = 0x1, Write = 0x2, Execute = 0x4 };
enum class ShareMode : uint8_t { None = 0x0, Read = 0x1, Write = 0x2, Delete = 0x4 };
/*CREATE_NEW			1
*CREATE_ALWAYS			2
*OPEN_EXISTING			3
*OPEN_ALWAYS			4
*TRUNCATE_EXISTING		5
**/
enum class OpenMode : uint8_t { CreateNew = 1, CreateAlways = 2, OpenExisting = 3, OpenAlways = 4, TruncateExisting = 5 };


class FileMapper_ : public NonCopyable
{
private:
	friend class FileMapper;
	const std::wstring mName, fName;
	void *file = nullptr;
	void *mapper = nullptr;
	const uint8_t fileAccess, shareMode;
	void close();
public:
	FileMapper_(const std::wstring& mname, const std::wstring& fname, const OpenMode openm, const uint8_t access = 0x3, const uint8_t share = 0x7);
	~FileMapper_();
	uint64_t getSize();
	bool mapping(const uint64_t size);
	bool write(const void *dat, const uint64_t size, const uint64_t offset);
	bool read(void *dat, const uint64_t size, const uint64_t offset);
	bool deleteFile();
};

class FileMapper
{
private:
	static std::map<std::wstring, Wrapper<FileMapper_>>& getMappingMap();
public:
	static Wrapper<FileMapper_> getMapping(const std::wstring& mname);
	template<class... ARGS>
	static Wrapper<FileMapper_> addMapping(const std::wstring& mname, ARGS... args)
	{
		auto& mmap = getMappingMap();
		if (mmap.find(mname) != mmap.end())
			return Wrapper<FileMapper_>();
		auto m = Wrapper<FileMapper_>(mname, args...);
		mmap.insert({ mname, m });
		return m;
	}
	static bool releaseMapping(const std::wstring& mname);
};



}