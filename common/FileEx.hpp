#pragma once

#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <string>

#include "Exceptions.hpp"

namespace common::file
{

namespace fs = std::experimental::filesystem;

template<class T>
inline void readAll(FILE *fp, T& output)
{
	static_assert(sizeof(T::value_type) == 1, "element's size should be 1 byte");
	fseek(fp, 0, SEEK_END);
	auto flen = ftell(fp);
	output.resize(flen);
	fseek(fp, 0, SEEK_SET);
	fread(output.data(), flen, 1, fp);
}
inline std::vector<uint8_t> readAll(FILE *fp)
{
	std::vector<uint8_t> fdata;
	readAll(fp, fdata);
	return fdata;
}
inline std::vector<uint8_t> readAll(const fs::path& fpath)
{
	FILE *fp = nullptr;
	_wfopen_s(&fp, fpath.c_str(), L"rb");
	if (fp == nullptr)
		COMMON_THROW(FileException, FileException::Reason::OpenFail, fpath, L"cannot open target file");
	auto fdata = readAll(fp);
	fclose(fp);
	return fdata;
}
inline std::string readAllTxt(FILE *fp)
{
	auto fdata = readAll(fp);
	fdata.push_back('\0');
	return std::string((char*)fdata.data());
}
inline std::string readAllTxt(const fs::path& fpath)
{
	FILE *fp = nullptr;
	_wfopen_s(&fp, fpath.c_str(), L"r");
	if (fp == nullptr)
		COMMON_THROW(FileException, FileException::Reason::OpenFail, fpath, L"cannot open target file");
	auto txt = readAllTxt(fp);
	fclose(fp);
	return txt;
}

}
