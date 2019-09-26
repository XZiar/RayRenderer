#pragma once


#include "CommonRely.hpp"
#include "EnumEx.hpp"
#include "Exceptions.hpp"
#include "Stream.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <string>
#include <optional>


#if defined(__cpp_lib_filesystem)
#   include <filesystem>
#else
#   include <experimental/filesystem>
#endif


#if !defined(_WIN32)
#   include <unistd.h>
#   include <cerrno>
#endif

namespace common
{
#if defined(__cpp_lib_filesystem)
namespace fs = std::filesystem;
#else
namespace fs = std::experimental::filesystem;
#endif
}

namespace common::file
{

using std::string;
using std::u16string;
using std::byte;


class FileException : public BaseException
{
public:
    enum class Reason { NotExist, WrongFormat, OpenFail, ReadFail, WriteFail, CloseFail };
public:
    fs::path filepath;
public:
    EXCEPTION_CLONE_EX(FileException);
    const Reason reason;
    FileException(const Reason why, const fs::path& file, const std::u16string_view& msg, const std::any& data_ = std::any())
        : BaseException(TYPENAME, msg, data_), filepath(file), reason(why)
    { }
    ~FileException() override {}
};


enum class OpenFlag : uint8_t 
{ 
    READ = 0b1, WRITE = 0b10, CREATE = 0b100, TEXT = 0b00000, BINARY = 0b10000,
    APPEND = 0b1110, TRUNC = 0b0110, 
    CreatNewBinary = CREATE|WRITE|BINARY, CreatNewText = CREATE | WRITE | TEXT,
};
MAKE_ENUM_BITFIELD(OpenFlag)


class FileObject : public std::enable_shared_from_this<FileObject>, public NonCopyable, public NonMovable
{
    friend class FileStream;
private:
    static constexpr const auto* ParseFlag(const OpenFlag flag)
    {
#if defined(_WIN32)
#   define StrText(x) L ##x
#else
#   define StrText(x) x
#endif
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
#undef StrText
    }

private:
    fs::path FilePath;
    FILE* fp;
    OpenFlag Flag;

    FileObject(const fs::path& path, FILE* fp, const OpenFlag flag) : FilePath(path), fp(fp), Flag(flag)
    {
        ::std::setvbuf(fp, NULL, _IOFBF, 16384);
    }
public:
    ~FileObject()
    {
        if (fp != nullptr)
            fclose(fp);
    }

    const fs::path& Path() const { return FilePath; }
    u16string ExtName() const { return FilePath.extension().u16string(); }
    FILE* Raw() { return fp; }

    //==========Open=========//

    static std::shared_ptr<FileObject> OpenFile(const fs::path& path, const OpenFlag flag)
    {
        if (!fs::exists(path))
            return {};
        FILE* fp;
#if defined(_WIN32)
        if (_wfopen_s(&fp, path.wstring().c_str(), ParseFlag(flag)) != 0)
            return {};
#else
        fp = fopen(path.u8string().c_str(), ParseFlag(flag));
        if (fp == nullptr)
            return {};
#endif
        return std::shared_ptr<FileObject>(new FileObject(path, fp, flag));
    }

    static std::shared_ptr<FileObject> OpenThrow(const fs::path& path, const OpenFlag flag)
    {
        if (!fs::exists(path) && !HAS_FIELD(flag, OpenFlag::CREATE))
            COMMON_THROW(FileException, FileException::Reason::NotExist, path, u"target file not exist");
        FILE* fp;
#if defined(_WIN32)
        if (_wfopen_s(&fp, path.wstring().c_str(), ParseFlag(flag)) != 0)
            COMMON_THROW(FileException, FileException::Reason::OpenFail, path, u"cannot open target file");
#else
        fp = fopen(path.u8string().c_str(), ParseFlag(flag));
        if (fp == nullptr)
            COMMON_THROW(FileException, FileException::Reason::OpenFail, path, u"cannot open target file");
#endif
        return std::shared_ptr<FileObject>(new FileObject(path, fp, flag));
    }

};

class FileStream
{
protected:
    std::shared_ptr<FileObject> File;

    FileStream(std::shared_ptr<FileObject>&& file) noexcept : File(std::move(file)) {}

#if defined(_WIN32)
    forceinline bool FSeek(const size_t offset, int32_t whence)
    {
        return _fseeki64(GetFP(), offset, whence) == 0;
    }
    forceinline size_t FTell() const
    {
        return _ftelli64(GetFP());
    }
#else
    forceinline bool FSeek(const size_t offset, int32_t whence)
    {
        return fseeko64(GetFP(), offset, whence) == 0;
    }
    forceinline size_t FTell() const
    {
        return ftello64(GetFP());
    }
#endif

    forceinline FILE* GetFP() const { return File->fp; }
    void WriteCheck() const
    {
        if (!HAS_FIELD(File->Flag, OpenFlag::WRITE))
            COMMON_THROW(FileException, FileException::Reason::WriteFail, File->FilePath, u"not opened for write");
    }
    void ReadCheck() const
    {
        if (!HAS_FIELD(File->Flag, OpenFlag::READ))
            COMMON_THROW(FileException, FileException::Reason::ReadFail, File->FilePath, u"not opened for read");
    }
    void CheckError(const FileException::Reason reason)
    {
        if (feof(GetFP()) != 0)
            COMMON_THROW(FileException, reason, File->FilePath, u"reach end of file");
        if (ferror(GetFP()) != 0)
            COMMON_THROW(FileException, reason, File->FilePath, u"access file failed");
        COMMON_THROW(FileException, reason, File->FilePath, u"unknown error");
    }
    size_t LeftSpace()
    {
        const auto cur = FTell();
        FSeek(0, SEEK_END);
        const auto flen = FTell();
        FSeek((int64_t)cur, SEEK_SET);
        return flen - cur;
    }

    //==========RandomStream=========//
    size_t GetSize()
    {
        const auto cur = FTell();
        FSeek(0, SEEK_END);
        const auto flen = FTell();
        FSeek((int64_t)cur, SEEK_SET);
        return flen;
    }
    size_t CurrentPos() const
    {
        return FTell();
    }
    bool SetPos(const size_t offset)
    {
        return FSeek((int64_t)offset, SEEK_SET);
    }

};


class FileInputStream : private FileStream, public io::RandomInputStream
{
    friend class FileObject;
public:
    FileInputStream(std::shared_ptr<FileObject> file) : FileStream(std::move(file)) { ReadCheck(); }
    FileInputStream(FileInputStream&& stream) noexcept : FileStream(std::move(stream.File)) { }
    virtual ~FileInputStream() override {}

    //==========InputStream=========//
    virtual size_t AvaliableSpace() override
    {
        return LeftSpace();
    };
    virtual bool Read(const size_t len, void* ptr) override
    {
        return fread(ptr, len, 1, GetFP()) != 0;
    }
    virtual size_t ReadMany(const size_t want, const size_t perSize, void* ptr) override
    {
        return fread(ptr, perSize, want, GetFP());
    }
    virtual bool Skip(const size_t len) override
    {
        return FSeek((int64_t)len, SEEK_CUR);
    }
    virtual bool IsEnd() override { return feof(GetFP()) != 0; }
    forceinline virtual std::byte ReadByteNE(bool& isSuccess) override
    {
        const auto ret = fgetc(GetFP());
        isSuccess = ret != EOF;
        return std::byte(ret);
    }
    forceinline virtual std::byte ReadByteME() override
    {
        const auto ret = fgetc(GetFP());
        if (ret == EOF)
            CheckError(FileException::Reason::ReadFail);
        return std::byte(ret);
    }

    //==========RandomStream=========//
    virtual size_t GetSize() override
    {
        return FileStream::GetSize();
    }
    virtual size_t CurrentPos() const override
    {
        return FileStream::CurrentPos();
    }
    virtual bool SetPos(const size_t offset) override
    {
        return FileStream::SetPos(offset);
    }
};


class FileOutputStream : private FileStream, public io::RandomOutputStream
{
    friend class FileObject;
public:
    FileOutputStream(std::shared_ptr<FileObject> file) : FileStream(std::move(file)) { WriteCheck(); }
    FileOutputStream(FileOutputStream&& stream) noexcept : FileStream(std::move(stream.File)) { }
    virtual ~FileOutputStream() override { Flush(); }
    
    //==========OutputStream=========//
    virtual size_t AcceptableSpace() override
    {
        return SIZE_MAX;
    };
    virtual bool Write(const size_t len, const void* ptr) override
    {
        return fwrite(ptr, len, 1, GetFP()) != 0;
    }
    virtual size_t WriteMany(const size_t want, const size_t perSize, const void* ptr) override
    {
        return fwrite(ptr, perSize, want, GetFP());
    }
    virtual void Flush() override
    {
        fflush(GetFP());
    }

    //==========RandomStream=========//
    virtual size_t GetSize() override
    {
        return FileStream::GetSize();
    }
    virtual size_t CurrentPos() const override
    {
        return FileStream::CurrentPos();
    }
    virtual bool SetPos(const size_t offset) override
    {
        return FileStream::SetPos(offset);
    }
};


//class BufferedFileWriter : public Writable<BufferedFileWriter>, public NonCopyable
//{
//private:
//    AlignedBuffer Buffer;
//    FileObject File;
//    size_t BufBegin, BufLen = 0;
//public:
//    BufferedFileWriter(FileObject&& file, const size_t bufSize) : Buffer(bufSize), File(std::move(file))
//    {
//        BufBegin = File.CurrentPos();
//    }
//    BufferedFileWriter(BufferedFileWriter&& other) noexcept
//        : Buffer(std::move(other.Buffer)), File(std::move(other.File)), BufBegin(other.BufBegin), BufLen(other.BufLen) 
//    {
//        other.BufLen = 0;
//    }
//    BufferedFileWriter& operator=(BufferedFileWriter&& other) noexcept
//    {
//        Flush();
//        Buffer = std::move(other.Buffer), File = std::move(other.File), BufBegin = other.BufBegin, BufLen = other.BufLen;
//        other.BufLen = 0;
//        return *this;
//    }
//    ~BufferedFileWriter()
//    {
//        Flush();
//    }
//    template<bool CheckSuccess = true>
//    bool Flush()
//    {
//        if (BufLen == 0)
//            return true;
//        const auto writeBytes = File.WriteMany(BufLen, Buffer.GetRawPtr());
//        BufLen -= writeBytes, BufBegin += writeBytes;
//        if constexpr (CheckSuccess)
//        {
//            if(BufLen > 0)
//                COMMON_THROW(FileException, FileException::Reason::WriteFail, File.Path(), u"fail to write demanded bytes");
//        }
//        return BufLen == 0;
//    }
//    FileObject Release()
//    {
//        Flush();
//        return std::move(File);
//    }
//
//    void Rewind(const size_t offset = 0)
//    {
//        Flush();
//        File.Rewind(BufBegin = offset);
//        BufLen = 0; // assume flush success
//    }
//    size_t CurrentPos() const { return BufBegin + BufLen; }
//    size_t GetSize() { return File.GetSize() + BufLen; }
//
//    bool Write(const size_t len, const void * const ptr)
//    {
//        //copy as many as possible
//        const auto bufBytes = std::min(Buffer.GetSize() - BufLen, len);
//        memcpy_s(Buffer.GetRawPtr() + BufLen, bufBytes, ptr, bufBytes);
//        BufLen += bufBytes;
//
//        const auto stillNeed = len - bufBytes;
//        if (stillNeed == 0)
//            return true;
//        Flush();
//        // force bypass buffer
//        if (stillNeed >= Buffer.GetSize())
//        {
//            const auto ret = File.Write(stillNeed, reinterpret_cast<const uint8_t*>(ptr) + bufBytes);
//            BufBegin = File.CurrentPos();
//            return ret;
//        }
//        // 0 < stillNeed < BufSize
//        memcpy_s(Buffer.GetRawPtr(), stillNeed, reinterpret_cast<const uint8_t*>(ptr) + bufBytes, stillNeed);
//        BufLen += stillNeed;
//        return true;
//    }
//
//    template<typename T = byte>
//    bool WriteByteNE(const T data)//without checking
//    {
//        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
//        if (BufLen >= Buffer.GetSize())
//            Flush<false>();
//        Buffer.GetRawPtr<T>()[BufLen++] = data;
//        return true;
//    }
//    template<typename T = byte>
//    bool WriteByte(const T data)
//    {
//        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
//        if (BufLen >= Buffer.GetSize())
//            Flush();
//        Buffer.GetRawPtr<T>()[BufLen++] = data;
//        return true;
//    }
//
//    using Writable<BufferedFileWriter>::Write;
//};

template<class T>
inline void ReadAll(const fs::path& fpath, T& output)
{
    static_assert(std::is_class_v<T>, "ReadAll should accept container object");
    FileInputStream fis(FileObject::OpenThrow(fpath, OpenFlag::BINARY | OpenFlag::READ));
    fis.ReadInto(output, SIZE_MAX);
}

template<class T>
inline std::vector<T> ReadAll(const fs::path& fpath)
{
    std::vector<T> output;
    ReadAll(fpath, output);
    return output;
}

template<typename Char = char>
inline std::basic_string<Char> ReadAllText(const fs::path& fpath)
{
    std::basic_string<Char> output;
    ReadAll(fpath, output);
    return output;
}


template<class T>
inline void WriteAll(const fs::path& fpath, T& output)
{
    static_assert(std::is_class_v<T>, "WriteAll should accept container object");
    FileOutputStream fis(FileObject::OpenThrow(fpath, OpenFlag::CreatNewBinary));
    fis.WriteFrom(output, 0, SIZE_MAX);
}

}
