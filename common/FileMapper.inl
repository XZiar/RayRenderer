#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include "FileMapper.h"

namespace common
{


FileMapper_::FileMapper_(const std::wstring& mname, const std::wstring& fname, const OpenMode openm, const uint8_t access, const uint8_t share)
	:mName(mname), fName(fname), fileAccess(access), shareMode(share)
{
	DWORD flagRW = (access&(uint8_t)FileAccess::Read ? GENERIC_READ : 0)
		| (access&(uint8_t)FileAccess::Write ? GENERIC_WRITE : 0)
		| (access&(uint8_t)FileAccess::Execute ? GENERIC_EXECUTE : 0);
	DWORD flagSM = (share&(uint16_t)ShareMode::Read ? FILE_SHARE_READ : 0)
		| (share&(uint16_t)ShareMode::Write ? FILE_SHARE_WRITE : 0)
		| (share&(uint16_t)ShareMode::Delete ? FILE_SHARE_DELETE : 0);
	HANDLE hFile = CreateFile(fname.c_str(), flagRW, flagSM, NULL, (uint8_t)openm, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		throw std::exception(("can not CreateFile --- " + std::to_string(GetLastError())).c_str());
	file = hFile;
}

FileMapper_::~FileMapper_()
{
	close();
}

void FileMapper_::close()
{
	if (mapper != nullptr)
	{
		CloseHandle(mapper);
		mapper = nullptr;
	}
	if (file != nullptr)
	{
		CloseHandle(file);
		file = nullptr;
	}
}


uint64_t FileMapper_::getSize()
{
	DWORD dwFileSizeHigh;
	uint64_t size = GetFileSize(file, &dwFileSizeHigh);
	size += ((uint64_t)dwFileSizeHigh) << 32;
	return size;
}

bool FileMapper_::mapping(const uint64_t size)
{
	DWORD flagpt = 0;
	if (fileAccess&(uint8_t)FileAccess::Read)
	{
		if (fileAccess&(uint8_t)FileAccess::Write)
			flagpt = PAGE_READWRITE;
		else
			flagpt = PAGE_READONLY;
	}
	if (fileAccess&(uint8_t)FileAccess::Execute)
		flagpt <<= 4;
	DWORD slow = static_cast<DWORD>(size&UINT32_MAX), shigh = static_cast<DWORD>(size >> 32);
	HANDLE hMapping = CreateFileMapping((HANDLE)file, NULL, flagpt, shigh, slow, mName.c_str());
	if (hMapping == nullptr)
		return false;
	mapper = hMapping;
	return true;
}

bool FileMapper_::write(const void *dat, const uint64_t size, const uint64_t offset)
{
	DWORD olow = static_cast<DWORD>(offset&UINT32_MAX), ohigh = static_cast<DWORD>(offset >> 32);
	auto pBuf = MapViewOfFile((HANDLE)mapper, FILE_MAP_WRITE, ohigh, olow, static_cast<SIZE_T>(size));
	if (pBuf == nullptr)
		return false;
	memcpy(pBuf, dat, static_cast<size_t>(size));
	UnmapViewOfFile(pBuf);
	return true;
}

bool FileMapper_::read(void *dat, const uint64_t size, const uint64_t offset)
{
	DWORD olow = static_cast<DWORD>(offset&UINT32_MAX), ohigh = static_cast<DWORD>(offset >> 32);
	auto pBuf = MapViewOfFile((HANDLE)mapper, FILE_MAP_WRITE, ohigh, olow, static_cast<SIZE_T>(size));
	if (pBuf == nullptr)
		return false;
	memcpy(dat, pBuf, static_cast<size_t>(size));
	UnmapViewOfFile(pBuf);
	return true;
}

bool FileMapper_::deleteFile()
{
	close();
	return DeleteFile(fName.c_str()) != 0;
}


std::map<std::wstring, Wrapper<FileMapper_, false>>& FileMapper::getMappingMap()
{
	static std::map<std::wstring, Wrapper<FileMapper_, false>> mappingMap;
	return mappingMap;
}

Wrapper<FileMapper_, false> FileMapper::getMapping(const std::wstring& mname)
{
	auto& mmap = getMappingMap();
	auto it = mmap.find(mname);
	if (it == mmap.end())
		return Wrapper<FileMapper_, false>();
	else
		return it->second;
}

bool FileMapper::releaseMapping(const std::wstring& mname)
{
	auto& mmap = getMappingMap();
	auto it = mmap.find(mname);
	if (it == mmap.end())
		return false;
	if (it->second.refCount() == 1)
	{
		mmap.erase(it);
		return true;
	}
	else
		return false;
}

}