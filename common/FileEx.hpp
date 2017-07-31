#pragma once

#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>
#include <optional>

#include "CommonMacro.hpp"
#include "StringEx.hpp"
#include "Exceptions.hpp"

#if defined(USING_CHARDET) && !defined(UCHARDETLIB_H_)
namespace uchdet
{
	common::Charset detectEncoding(const std::string& str);
}
#endif

namespace common::file
{

namespace fs = std::experimental::filesystem;
using std::string;
using std::wstring;

#if defined(USING_CHARDET) || defined(UCHARDETLIB_H_)
#   define GET_ENCODING(str, chset) chset = uchdet::detectEncoding(str)
#else
#   define GET_ENCODING(str, chset) COMMON_THROW(BaseException, L"UnSuppoted, lacks uchardet");
#endif


enum class OpenFlag : uint8_t { READ = 0b1, WRITE = 0b10, CREATE = 0b100, APPEND = 0b1110, TRUNC = 0b0110, TEXT = 0b00000, BINARY = 0b10000 };
MAKE_ENUM_BITFIELD(OpenFlag)

class FileObject : public NonCopyable
{
private:
	FILE *fp;

	static const wchar_t* ParseFlag(const OpenFlag flag)
	{
		switch ((uint8_t)flag)
		{
		case 0b00001: return L"r";
		case 0b00011: return L"r+";
		case 0b00110: return L"w";
		case 0b00111: return L"w+";
		case 0b01110: return L"a";
		case 0b01111: return L"a+";
		case 0b10001: return L"rb";
		case 0b10011: return L"r+b";
		case 0b10110: return L"wb";
		case 0b10111: return L"w+b";
		case 0b11110: return L"ab";
		case 0b11111: return L"a+b";
		default: return nullptr;
		}
	}

	constexpr FileObject(const fs::path& path, FILE *fp) : filePath(path), fp(fp) {}
public:
	const fs::path filePath;
	FileObject(FileObject&& rhs) = default;
	FileObject& operator= (FileObject&& rhs) = default;
	~FileObject() { if (fp != nullptr) fclose(fp); }
	wstring extName() const { return filePath.extension().wstring(); }

	FILE* Raw() { return fp; }

	void Rewind(const size_t offset = 0) { fseek(fp, (long)offset, SEEK_SET); }
	size_t CurrentPos() const { return ftell(fp); }

	size_t GetSize()
	{
		const auto cur = CurrentPos();
		fseek(fp, 0, SEEK_END);
		const auto flen = CurrentPos();
		Rewind(cur);
		return flen;
	}

	bool Read(const size_t len, void *ptr)
	{
		return fread(ptr, len, 1, fp) != 0;
	}

	bool Write(const size_t len, const void *ptr)
	{
		return fwrite(ptr, len, 1, fp) != 0;
	}

	template<class T, size_t N>
	size_t Read(T(&output)[N], size_t count = N)
	{
		const size_t elementSize = sizeof(T);
		const auto cur = CurrentPos();
		fseek(fp, 0, SEEK_END);
		const auto flen = CurrentPos();
		Rewind(cur);
		count = std::min(count, N);
		count = std::min((flen - cur) / elementSize, N);
		auto ret = Read(count * elementSize, output);
		return ret ? count : 0;
	}

	template<class T, size_t N>
	size_t Write(T(&input)[N], size_t count = N)
	{
		const size_t elementSize = sizeof(T);
		count = std::min(count, N);
		auto ret = Write(count * elementSize, output);
		return ret ? count : 0;
	}

	template<class T, typename = typename std::enable_if<std::is_class<T>::value>::type>
	size_t Read(size_t count, T& output)
	{
		const size_t elementSize = sizeof(T::value_type);
		const auto cur = CurrentPos();
		fseek(fp, 0, SEEK_END);
		const auto flen = CurrentPos();
		Rewind(cur);
		count = std::min((flen - cur) / elementSize, count);
		output.resize(count);
		return Read(count * elementSize, output.data()) ? count : 0;
	}

	template<class T, typename = typename std::enable_if<std::is_class<T>::value>::type>
	size_t Write(size_t count, const T& input)
	{
		const size_t elementSize = sizeof(T::value_type);
		count = std::min(input.size(), count);
		return Write(count * elementSize, output.data()) ? count : 0;
	}

	template<class T, typename = typename std::enable_if<std::is_class<T>::value>::type>
	void ReadAll(T& output)
	{
		static_assert(sizeof(T::value_type) == 1, "element's size should be 1 byte");
		auto flen = GetSize();
		Rewind();
		Read(flen, output);
	}

	std::vector<uint8_t> ReadAll()
	{
		std::vector<uint8_t> fdata;
		ReadAll(fdata);
		return fdata;
	}

	std::string ReadAllText()
	{
		std::string text;
		ReadAll(text);
		text.push_back('\0');
		return text;
	}

	std::string ReadAllText(Charset& chset)
	{
		auto text = ReadAllText();
		GET_ENCODING(text, chset);
		return text;
	}

	std::string ReadAllTextUTF8(const Charset chset)
	{
		Charset rawChar;
		auto text = ReadAllText(rawChar);
		if(rawChar == Charset::UTF8)
			return text;
	}

	static std::optional<FileObject> OpenFile(const fs::path& path, const OpenFlag flag)
	{
		if (!fs::exists(path))
			return {};
		FILE *fp;
		if (_wfopen_s(&fp, path.wstring().c_str(), ParseFlag(flag)) != 0)
			return {};
		return std::optional<FileObject>(std::in_place, FileObject(path, fp));
	}

	static FileObject OpenThrow(const fs::path& path, const OpenFlag flag)
	{
		if (!fs::exists(path) && !HAS_FIELD(flag, OpenFlag::CREATE))
			COMMON_THROW(FileException, FileException::Reason::NotExist, path, L"target file not exist");
		FILE *fp;
		if (_wfopen_s(&fp, path.wstring().c_str(), ParseFlag(flag)) != 0)
			COMMON_THROW(FileException, FileException::Reason::OpenFail, path, L"cannot open target file");
		return FileObject(path, fp);
	}

};

template<typename T>
inline void ReadAll(const fs::path& fpath, T& output)
{
	FileObject::OpenThrow(fpath, OpenFlag::BINARY | OpenFlag::READ)
		.ReadAll(output);
}

inline string ReadAllText(const fs::path& fpath)
{
	return FileObject::OpenThrow(fpath, OpenFlag::BINARY | OpenFlag::READ)
		.ReadAllText();
}

/*
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
*/

}
