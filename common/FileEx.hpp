#pragma once

#if defined(_WIN32)
#   define FSeek64(fp, offset, whence) _fseeki64(fp, offset, whence)
#   define FTell64(fp) _ftelli64(fp)
#else
//#   define _FILE_OFFSET_BITS 64
#   ifndef _LARGEFILE_SOURCE
#       define _LARGEFILE_SOURCE 1
#   endif
#   include <unistd.h>
#   include <cerrno>
#   define FSeek64(fp, offset, whence) fseeko64(fp, offset, whence)
#   define FTell64(fp) ftello64(fp)
#endif

#include "CommonRely.hpp"
#include "StrCharset.hpp"
#include "Exceptions.hpp"
#include "AlignedContainer.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <string>
#include <optional>


#if defined(USING_CHARDET) && !defined(UCHARDETLIB_H_)
namespace uchdet
{
template<typename T>
common::str::Charset detectEncoding(const std::basic_string<T>& str);
}
#endif

namespace common::file
{


using std::string;
using std::u16string;
using std::byte;
using common::str::Charset;

#if defined(USING_CHARDET) || defined(UCHARDETLIB_H_)
#   define GET_ENCODING(str, chset) chset = uchdet::detectEncoding(str)
#else
#   define GET_ENCODING(str, chset) COMMON_THROW(BaseException, u"UnSuppoted, lacks uchardet");
#endif


enum class OpenFlag : uint8_t 
{ 
    READ = 0b1, WRITE = 0b10, CREATE = 0b100, TEXT = 0b00000, BINARY = 0b10000,
    APPEND = 0b1110, TRUNC = 0b0110, 
    CreatNewBinary = CREATE|WRITE|BINARY, CreatNewText = CREATE | WRITE | TEXT,
};
MAKE_ENUM_BITFIELD(OpenFlag)


template<class Base>
class Readable
{
private:
    forceinline bool Read_(const size_t len, void * const ptr) { return static_cast<Base*>(this)->Read(len, ptr); }
    forceinline void Rewind_(const size_t offset = 0) { return static_cast<Base*>(this)->Rewind(offset); }
    forceinline size_t CurrentPos_() const { return static_cast<const Base*>(this)->CurrentPos(); }
    forceinline size_t GetSize_() { return static_cast<Base*>(this)->GetSize(); }
public:
    forceinline size_t LeftSpace() { return GetSize_() - CurrentPos_(); }

    template<typename T>
    bool Read(T& output)
    {
        return Read_(sizeof(T), &output);
    }

    template<class T, size_t N>
    size_t Read(T(&output)[N], size_t count = N)
    {
        const size_t elementSize = sizeof(T);
        const auto left = LeftSpace();
        count = std::min(count, N);
        count = std::min(left / elementSize, N);
        auto ret = Read_(count * elementSize, output);
        return ret ? count : 0;
    }

    template<class T, typename = typename std::enable_if<std::is_class<T>::value>::type>
    size_t Read(size_t count, T& output)
    {
        const size_t elementSize = sizeof(typename T::value_type);
        const auto left = LeftSpace();
        count = std::min(left / elementSize, count);
        output.resize(count);
        return Read_(count * elementSize, output.data()) ? count : 0;
    }

    template<class T, typename = typename std::enable_if_t<std::is_class_v<T>>>
    void ReadAll(T& output)
    {
        static_assert(sizeof(typename T::value_type) == 1, "element's size should be 1 byte");
        const auto flen = GetSize_();
        Rewind_();
        Read(flen, output);
    }

    template<typename T = byte>
    std::vector<T> ReadAll()
    {
        std::vector<T> fdata;
        ReadAll(fdata);
        return fdata;
    }

    std::string ReadAllText()
    {
        std::string text;
        ReadAll(text);
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
        if (rawChar == Charset::UTF8)
            return text;
        else
            return str::to_u8string(text, rawChar);
    }
};

template<class Base>
class Writable
{
private:
    forceinline bool Write_(const size_t len, const void * const ptr) { return static_cast<Base*>(this)->Write(len, ptr); }
public:
    template<typename T>
    bool Write(const T& output)
    {
        return Write_(sizeof(T), &output);
    }

    template<class T, size_t N>
    size_t Write(T(&output)[N], size_t count = N)
    {
        const size_t elementSize = sizeof(T);
        count = std::min(count, N);
        auto ret = Write_(count * elementSize, output);
        return ret ? count : 0;
    }

    template<class T, typename = typename std::enable_if<std::is_class_v<T>>::type>
    size_t Write(size_t count, const T& input)
    {
        const size_t elementSize = sizeof(typename T::value_type);
        count = std::min(input.size(), count);
        return Write_(count * elementSize, input.data()) ? count : 0;
    }
};

class FileObject : public Readable<FileObject>, public Writable<FileObject>, public NonCopyable
{
private:
    fs::path FilePath;
    FILE *fp;
    OpenFlag Flag;

    static constexpr const auto* ParseFlag(const OpenFlag flag)
    {
        switch ((uint8_t)flag)
        {
        case 0b00001: return StrText("r");
        case 0b00011: return StrText("r+");
        case 0b00110: return StrText("w");
        case 0b00111: return StrText("w+");
        case 0b01110: return StrText("a");
        case 0b01111: return StrText("a+");
        case 0b10001: return StrText("rb");
        case 0b10011: return StrText("r+b");
        case 0b10110: return StrText("wb");
        case 0b10111: return StrText("w+b");
        case 0b11110: return StrText("ab");
        case 0b11111: return StrText("a+b");
        default:      return StrText("");
        }
    }

    FileObject(const fs::path& path, FILE *fp, const OpenFlag flag) : FilePath(path), fp(fp), Flag(flag)
    {
        ::std::setvbuf(fp, NULL, _IOFBF, 16384);
    }
public:
    FileObject(FileObject&& rhs) noexcept : FilePath(std::move(rhs.FilePath)), fp(rhs.fp) { rhs.fp = nullptr; }
    FileObject& operator= (FileObject&& rhs) noexcept
    {
        if (fp != nullptr)
            fclose(fp);
        fp = rhs.fp;
        FilePath = std::move(rhs.FilePath);
        rhs.fp = nullptr;
        return *this;
    }
    ~FileObject() { if (fp != nullptr) fclose(fp); }
    const fs::path& Path() const { return FilePath; }
    u16string extName() const { return FilePath.extension().u16string(); }

    FILE* Raw() { return fp; }

    void Flush() { if (HAS_FIELD(Flag, OpenFlag::WRITE)) fflush(fp); }
    void Rewind(const size_t offset = 0) { FSeek64(fp, (int64_t)offset, SEEK_SET); }
    void Skip(const size_t offset = 0) { FSeek64(fp, (int64_t)offset, SEEK_CUR); }
    size_t CurrentPos() const { return FTell64(fp); }
    bool IsEnd() const { return feof(fp) != 0; }
    size_t GetSize()
    {
        const auto cur = CurrentPos();
        FSeek64(fp, 0, SEEK_END);
        const auto flen = CurrentPos();
        Rewind(cur);
        return flen;
    }

    template<typename T = byte>
    T ReadByteNE() noexcept//without checking
    {
        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
        return (T)fgetc(fp);
    }
    template<typename T = byte>
    T ReadByte()
    {
        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
        const auto ret = fgetc(fp);
        if (ret != EOF)
            return static_cast<T>(ret);
        else
            COMMON_THROW(FileException, FileException::Reason::ReadFail, FilePath, u"reach end of file");
    }

    bool Read(const size_t len, void * const ptr)
    {
        return fread(ptr, len, 1, fp) != 0;
    }
    template<typename T>
    size_t ReadMany(const size_t want, T * const ptr)
    {
        return fread(ptr, sizeof(T), want, fp);
    }

    template<typename T = byte>
    bool WriteByteNE(const T data) noexcept//without checking
    {
        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
        return fputc(*reinterpret_cast<const uint8_t*>(&data), fp) != EOF;
    }
    template<typename T = byte>
    bool WriteByte(const T data)
    {
        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
        const auto ret = fputc(*reinterpret_cast<const uint8_t*>(&data), fp);
        if (ret != EOF)
            return true;
        else
            COMMON_THROW(FileException, FileException::Reason::WriteFail, FilePath, u"write byte to file failed");
    }

    bool Write(const size_t len, const void *ptr)
    {
        return fwrite(ptr, len, 1, fp) != 0;
    }
    template<typename T>
    size_t WriteMany(const size_t want, const T * const ptr)
    {
        return fwrite(ptr, sizeof(T), want, fp);
    }

    using Readable<FileObject>::Read;
    using Writable<FileObject>::Write;
    
    static std::optional<FileObject> OpenFile(const fs::path& path, const OpenFlag flag)
    {
        if (!fs::exists(path))
            return {};
        FILE *fp;
    #if defined(_WIN32)
        if (_wfopen_s(&fp, path.wstring().c_str(), ParseFlag(flag)) != 0)
            return {};
    #else
        fp = fopen(path.u8string().c_str(), ParseFlag(flag));
        if (fp == nullptr)
            return {};
    #endif
        return std::optional<FileObject>(std::in_place, FileObject(path, fp, flag));
    }

    static FileObject OpenThrow(const fs::path& path, const OpenFlag flag)
    {
        if (!fs::exists(path) && !HAS_FIELD(flag, OpenFlag::CREATE))
            COMMON_THROW(FileException, FileException::Reason::NotExist, path, u"target file not exist");
        FILE *fp;
    #if defined(_WIN32)
        if (_wfopen_s(&fp, path.wstring().c_str(), ParseFlag(flag)) != 0)
            COMMON_THROW(FileException, FileException::Reason::OpenFail, path, u"cannot open target file");
    #else
        fp = fopen(path.u8string().c_str(), ParseFlag(flag));
        if (fp == nullptr)
            COMMON_THROW(FileException, FileException::Reason::OpenFail, path, u"cannot open target file");
    #endif
        return FileObject(path, fp, flag);
    }

};

class BufferedFileReader : public Readable<BufferedFileReader>, public NonCopyable
{
private:
    AlignedBuffer Buffer;
    FileObject File;
    size_t BufBegin, BufPos = 0, BufLen = 0;
    template<bool IsNext = true>
    void LoadBuffer()
    {
        if(IsNext)
            BufBegin += BufLen;
        BufPos = 0;
        BufLen = File.ReadMany(Buffer.GetSize(), Buffer.GetRawPtr());
    }
public:
    BufferedFileReader(FileObject&& file, const size_t bufSize) : Buffer(bufSize), File(std::move(file))
    {
        BufBegin = File.CurrentPos();
        LoadBuffer();
    }

    void Flush() 
    {
        File.Rewind(BufBegin + BufPos);
        BufLen -= BufPos;
        memmove_s(Buffer.GetRawPtr(), Buffer.GetSize(), Buffer.GetRawPtr() + BufPos, BufLen);
    }
    FileObject Release()
    {
        Flush();
        return std::move(File);
    }

    void Rewind(const size_t offset = 0) 
    {
        if (offset >= BufBegin)
        {
            const auto dist = offset - BufBegin;
            if (dist < BufLen)
            {
                BufPos = dist;
                return;
            }
        }
        //need reload
        File.Rewind(BufBegin = offset);
        LoadBuffer<false>();
    }
    void Skip(const size_t offset = 0) 
    {
        const auto newBufPos = BufPos + offset;
        if (newBufPos < BufLen)
        {
            BufPos = newBufPos;
            return;
        }
        //need reload
        BufBegin += offset;
        File.Rewind(BufBegin);
        LoadBuffer<false>();
    }
    size_t CurrentPos() const { return BufBegin + BufPos; }
    bool IsEnd() const { return (BufPos >= BufLen) && File.IsEnd(); }
    size_t GetSize() { return File.GetSize(); }

    bool Read(const size_t len, void * const ptr)
    {
        //copy as many as possible
        const auto bufBytes = std::min(BufLen - BufPos, len);
        memcpy_s(ptr, len, Buffer.GetRawPtr() + BufPos, bufBytes);
        BufPos += bufBytes;

        const auto stillNeed = len - bufBytes;
        if (stillNeed == 0)
            return true;
        // force bypass buffer
        if (stillNeed >= Buffer.GetSize())
        {
            const auto ret = File.Read(stillNeed, reinterpret_cast<uint8_t*>(ptr) + bufBytes);
            BufBegin = File.CurrentPos();
            LoadBuffer<false>();
            return ret;
        }
        // 0 < stillNeed < BufSize
        LoadBuffer();
        if (stillNeed > BufLen) //simple reject
            return false;
        memcpy_s(reinterpret_cast<uint8_t*>(ptr) + bufBytes, stillNeed, Buffer.GetRawPtr(), stillNeed);
        BufPos += stillNeed;
        return true;
    }

    template<typename T = byte>
    T ReadByteNE()//without checking
    {
        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
        if (BufPos >= BufLen)
            LoadBuffer();
        return (T)Buffer[BufPos++];
    }
    template<typename T = byte>
    T ReadByte()
    {
        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
        if (BufPos >= BufLen)
            LoadBuffer();
        if (BufPos >= BufLen)
            COMMON_THROW(FileException, FileException::Reason::ReadFail, File.Path(), u"reach end of file");
        return (T)Buffer[BufPos++];
    }

    using Readable<BufferedFileReader>::Read;
};

class BufferedFileWriter : public Writable<BufferedFileWriter>, public NonCopyable
{
private:
    AlignedBuffer Buffer;
    FileObject File;
    size_t BufBegin, BufLen = 0;
public:
    BufferedFileWriter(FileObject&& file, const size_t bufSize) : Buffer(bufSize), File(std::move(file))
    {
        BufBegin = File.CurrentPos();
    }
    BufferedFileWriter(BufferedFileWriter&& other) noexcept
        : Buffer(std::move(other.Buffer)), File(std::move(other.File)), BufBegin(other.BufBegin), BufLen(other.BufLen) 
    {
        other.BufLen = 0;
    }
    BufferedFileWriter& operator=(BufferedFileWriter&& other) noexcept
    {
        Flush();
        Buffer = std::move(other.Buffer), File = std::move(other.File), BufBegin = other.BufBegin, BufLen = other.BufLen;
        other.BufLen = 0;
        return *this;
    }
    ~BufferedFileWriter()
    {
        Flush();
    }
    template<bool CheckSuccess = true>
    bool Flush()
    {
        if (BufLen == 0)
            return true;
        const auto writeBytes = File.WriteMany(BufLen, Buffer.GetRawPtr());
        BufLen -= writeBytes, BufBegin += writeBytes;
        if constexpr (CheckSuccess)
        {
            if(BufLen > 0)
                COMMON_THROW(FileException, FileException::Reason::WriteFail, File.Path(), u"fail to write demanded bytes");
        }
        return BufLen == 0;
    }
    FileObject Release()
    {
        Flush();
        return std::move(File);
    }

    void Rewind(const size_t offset = 0)
    {
        Flush();
        File.Rewind(BufBegin = offset);
        BufLen = 0; // assume flush success
    }
    size_t CurrentPos() const { return BufBegin + BufLen; }
    size_t GetSize() { return File.GetSize() + BufLen; }

    bool Write(const size_t len, const void * const ptr)
    {
        //copy as many as possible
        const auto bufBytes = std::min(Buffer.GetSize() - BufLen, len);
        memcpy_s(Buffer.GetRawPtr() + BufLen, bufBytes, ptr, bufBytes);
        BufLen += bufBytes;

        const auto stillNeed = len - bufBytes;
        if (stillNeed == 0)
            return true;
        Flush();
        // force bypass buffer
        if (stillNeed >= Buffer.GetSize())
        {
            const auto ret = File.Write(stillNeed, reinterpret_cast<const uint8_t*>(ptr) + bufBytes);
            BufBegin = File.CurrentPos();
            return ret;
        }
        // 0 < stillNeed < BufSize
        memcpy_s(Buffer.GetRawPtr(), stillNeed, reinterpret_cast<const uint8_t*>(ptr) + bufBytes, stillNeed);
        BufLen += stillNeed;
        return true;
    }

    template<typename T = byte>
    bool WriteByteNE(const T data)//without checking
    {
        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
        if (BufLen >= Buffer.GetSize())
            Flush<false>();
        Buffer.GetRawPtr<T>()[BufLen++] = data;
        return true;
    }
    template<typename T = byte>
    bool WriteByte(const T data)
    {
        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
        if (BufLen >= Buffer.GetSize())
            Flush();
        Buffer.GetRawPtr<T>()[BufLen++] = data;
        return true;
    }

    using Writable<BufferedFileWriter>::Write;
};

template<class T>
inline void ReadAll(const fs::path& fpath, T& output)
{
    static_assert(std::is_class_v<T>, "ReadAll should accept container object");
    FileObject::OpenThrow(fpath, OpenFlag::BINARY | OpenFlag::READ)
        .ReadAll(output);
}

inline string ReadAllText(const fs::path& fpath)
{
    return FileObject::OpenThrow(fpath, OpenFlag::BINARY | OpenFlag::READ)
        .ReadAllText();
}


}
