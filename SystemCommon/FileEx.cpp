#include "FileEx.h"

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>


#if !defined(_WIN32)
#   include <unistd.h>
#   include <cerrno>
#endif

namespace common::file
{

using std::string;
using std::u16string;
using std::byte;


const FileObject::FlagType* FileObject::ParseFlag(const OpenFlag flag)
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


FileObject::FileObject(const fs::path& path, FILE* fp, const OpenFlag flag) : 
    FilePath(path), fp(fp), Flag(flag)
{
    ::std::setvbuf(fp, NULL, _IOFBF, 16384);
}
FileObject::~FileObject()
{
    if (fp != nullptr)
        fclose(fp);
}

std::shared_ptr<FileObject> FileObject::OpenFile(const fs::path& path, const OpenFlag flag)
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

std::shared_ptr<FileObject> FileObject::OpenThrow(const fs::path& path, const OpenFlag flag)
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


FileStream::FileStream(std::shared_ptr<FileObject>&& file) noexcept : 
    File(std::move(file)) { }

FileStream::~FileStream() { }

bool FileStream::FSeek(const size_t offset, int32_t whence)
{
#if defined(_WIN32)
    return _fseeki64(GetFP(), offset, whence) == 0;
#else
    return fseeko64(GetFP(), offset, whence) == 0;
#endif
}
size_t FileStream::FTell() const
{
#if defined(_WIN32)
    return _ftelli64(GetFP());
#else
    return ftello64(GetFP());
#endif
}

FILE* FileStream::GetFP() const { return File->fp; }

void FileStream::WriteCheck() const
{
    if (!HAS_FIELD(File->Flag, OpenFlag::WRITE))
        COMMON_THROW(FileException, FileException::Reason::WriteFail, File->FilePath, u"not opened for write");
}

void FileStream::ReadCheck() const
{
    if (!HAS_FIELD(File->Flag, OpenFlag::READ))
        COMMON_THROW(FileException, FileException::Reason::ReadFail, File->FilePath, u"not opened for read");
}

void FileStream::CheckError(const FileException::Reason reason)
{
    if (feof(GetFP()) != 0)
        COMMON_THROW(FileException, reason, File->FilePath, u"reach end of file");
    if (ferror(GetFP()) != 0)
        COMMON_THROW(FileException, reason, File->FilePath, u"access file failed");
    COMMON_THROW(FileException, reason, File->FilePath, u"unknown error");
}

size_t FileStream::LeftSpace()
{
    const auto cur = FTell();
    FSeek(0, SEEK_END);
    const auto flen = FTell();
    FSeek((int64_t)cur, SEEK_SET);
    return flen - cur;
}

//==========RandomStream=========//
size_t FileStream::GetSize()
{
    const auto cur = FTell();
    FSeek(0, SEEK_END);
    const auto flen = FTell();
    FSeek((int64_t)cur, SEEK_SET);
    return flen;
}
size_t FileStream::CurrentPos() const
{
    return FTell();
}
bool FileStream::SetPos(const size_t offset)
{
    return FSeek((int64_t)offset, SEEK_SET);
}


FileInputStream::FileInputStream(std::shared_ptr<FileObject> file) : 
    FileStream(std::move(file)) { ReadCheck(); }

FileInputStream::FileInputStream(FileInputStream&& stream) noexcept : 
    FileStream(std::move(stream.File)) { }

FileInputStream::~FileInputStream() { }

//==========InputStream=========//
size_t FileInputStream::AvaliableSpace()
{
    return LeftSpace();
}
bool FileInputStream::Read(const size_t len, void* ptr)
{
    return fread(ptr, len, 1, GetFP()) != 0;
}
size_t FileInputStream::ReadMany(const size_t want, const size_t perSize, void* ptr)
{
    return fread(ptr, perSize, want, GetFP());
}
bool FileInputStream::Skip(const size_t len)
{
    return FSeek((int64_t)len, SEEK_CUR);
}
bool FileInputStream::IsEnd() { return feof(GetFP()) != 0; }
std::byte FileInputStream::ReadByteNE(bool& isSuccess)
{
    const auto ret = fgetc(GetFP());
    isSuccess = ret != EOF;
    return std::byte(ret);
}
std::byte FileInputStream::ReadByteME()
{
    const auto ret = fgetc(GetFP());
    if (ret == EOF)
        CheckError(FileException::Reason::ReadFail);
    return std::byte(ret);
}

//==========RandomStream=========//
size_t FileInputStream::GetSize()
{
    return FileStream::GetSize();
}
size_t FileInputStream::CurrentPos() const
{
    return FileStream::CurrentPos();
}
bool FileInputStream::SetPos(const size_t offset)
{
    return FileStream::SetPos(offset);
}


FileOutputStream::FileOutputStream(std::shared_ptr<FileObject> file) : 
    FileStream(std::move(file)) { WriteCheck(); }

inline FileOutputStream::FileOutputStream(FileOutputStream&& stream) noexcept : FileStream(std::move(stream.File)) { }

inline FileOutputStream::~FileOutputStream() { Flush(); }

//==========OutputStream=========//
size_t FileOutputStream::AcceptableSpace()
{
    return SIZE_MAX;
}
bool FileOutputStream::Write(const size_t len, const void* ptr)
{
    return fwrite(ptr, len, 1, GetFP()) != 0;
}
size_t FileOutputStream::WriteMany(const size_t want, const size_t perSize, const void* ptr)
{
    return fwrite(ptr, perSize, want, GetFP());
}
void FileOutputStream::Flush()
{
    fflush(GetFP());
}

//==========RandomStream=========//
size_t FileOutputStream::GetSize()
{
    return FileStream::GetSize();
}
size_t FileOutputStream::CurrentPos() const
{
    return FileStream::CurrentPos();
}
bool FileOutputStream::SetPos(const size_t offset)
{
    return FileStream::SetPos(offset);
}




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




}
