#pragma once


#include "CommonRely.hpp"
#include "Exceptions.hpp"
#include "AlignedContainer.hpp"
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


//template<class Base>
//class Readable
//{
//private:
//    forceinline bool Read_(const size_t len, void * const ptr) { return static_cast<Base*>(this)->Read(len, ptr); }
//    forceinline void Rewind_(const size_t offset = 0) { return static_cast<Base*>(this)->Rewind(offset); }
//    forceinline size_t CurrentPos_() const { return static_cast<const Base*>(this)->CurrentPos(); }
//    forceinline size_t GetSize_() { return static_cast<Base*>(this)->GetSize(); }
//public:
//    forceinline size_t LeftSpace() { return GetSize_() - CurrentPos_(); }
//
//    template<typename T>
//    bool Read(T& output)
//    {
//        return Read_(sizeof(T), &output);
//    }
//
//    template<class T, size_t N>
//    size_t Read(T(&output)[N], size_t count = N)
//    {
//        const size_t elementSize = sizeof(T);
//        const auto left = LeftSpace();
//        count = std::min(count, N);
//        count = std::min(left / elementSize, N);
//        auto ret = Read_(count * elementSize, output);
//        return ret ? count : 0;
//    }
//
//    template<class T, typename = typename std::enable_if<std::is_class<T>::value>::type>
//    size_t Read(size_t count, T& output)
//    {
//        const size_t elementSize = sizeof(typename T::value_type);
//        const auto left = LeftSpace();
//        count = std::min(left / elementSize, count);
//        output.resize(count);
//        return Read_(count * elementSize, output.data()) ? count : 0;
//    }
//
//    template<class T, typename = typename std::enable_if_t<std::is_class_v<T>>>
//    void ReadAll(T& output)
//    {
//        static_assert(sizeof(typename T::value_type) == 1, "element's size should be 1 byte");
//        const auto flen = GetSize_();
//        Rewind_();
//        Read(flen, output);
//    }
//
//    template<typename T = byte>
//    std::vector<T> ReadAll()
//    {
//        std::vector<T> fdata;
//        ReadAll(fdata);
//        return fdata;
//    }
//
//    std::string ReadAllText()
//    {
//        std::string text;
//        ReadAll(text);
//        return text;
//    }
//};

//template<class Base>
//class Writable
//{
//private:
//    forceinline bool Write_(const size_t len, const void * const ptr) { return static_cast<Base*>(this)->Write(len, ptr); }
//public:
//    template<typename T>
//    bool Write(const T& output)
//    {
//        return Write_(sizeof(T), &output);
//    }
//
//    template<class T, size_t N>
//    size_t Write(T(&output)[N], size_t count = N)
//    {
//        const size_t elementSize = sizeof(T);
//        count = std::min(count, N);
//        auto ret = Write_(count * elementSize, output);
//        return ret ? count : 0;
//    }
//
//    template<class T, typename = typename std::enable_if<std::is_class_v<T>>::type>
//    size_t Write(size_t count, const T& input)
//    {
//        const size_t elementSize = sizeof(typename T::value_type);
//        count = std::min(input.size(), count);
//        return Write_(count * elementSize, input.data()) ? count : 0;
//    }
//};



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

    FileStream(const std::shared_ptr<FileObject>& file) noexcept : File(file) {}
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
    FileInputStream(const std::shared_ptr<FileObject>& file) : FileStream(file) { ReadCheck(); }
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
    FileOutputStream(const std::shared_ptr<FileObject>& file) : FileStream(file) { WriteCheck(); }
    FileOutputStream(FileOutputStream&& stream) noexcept : FileStream(std::move(stream.File)) { }
    virtual ~FileOutputStream() override {}
    
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



//class BufferedFileReader : public Readable<BufferedFileReader>, public NonCopyable
//{
//private:
//    AlignedBuffer Buffer;
//    FileObject File;
//    size_t BufBegin, BufPos = 0, BufLen = 0;
//    template<bool IsNext = true>
//    void LoadBuffer()
//    {
//        if(IsNext)
//            BufBegin += BufLen;
//        BufPos = 0;
//        BufLen = File.ReadMany(Buffer.GetSize(), Buffer.GetRawPtr());
//    }
//public:
//    BufferedFileReader(FileObject&& file, const size_t bufSize) : Buffer(bufSize), File(std::move(file))
//    {
//        BufBegin = File.CurrentPos();
//        LoadBuffer();
//    }
//
//    void Flush() 
//    {
//        File.Rewind(BufBegin + BufPos);
//        BufLen -= BufPos;
//        memmove_s(Buffer.GetRawPtr(), Buffer.GetSize(), Buffer.GetRawPtr() + BufPos, BufLen);
//    }
//    FileObject Release()
//    {
//        Flush();
//        return std::move(File);
//    }
//
//    void Rewind(const size_t offset = 0) 
//    {
//        if (offset >= BufBegin)
//        {
//            const auto dist = offset - BufBegin;
//            if (dist < BufLen)
//            {
//                BufPos = dist;
//                return;
//            }
//        }
//        //need reload
//        File.Rewind(BufBegin = offset);
//        LoadBuffer<false>();
//    }
//    void Skip(const size_t offset = 0) 
//    {
//        const auto newBufPos = BufPos + offset;
//        if (newBufPos < BufLen)
//        {
//            BufPos = newBufPos;
//            return;
//        }
//        //need reload
//        BufBegin += offset;
//        File.Rewind(BufBegin);
//        LoadBuffer<false>();
//    }
//    size_t CurrentPos() const { return BufBegin + BufPos; }
//    bool IsEnd() const { return (BufPos >= BufLen) && File.IsEnd(); }
//    size_t GetSize() { return File.GetSize(); }
//
//    bool Read(const size_t len, void * const ptr)
//    {
//        //copy as many as possible
//        const auto bufBytes = std::min(BufLen - BufPos, len);
//        memcpy_s(ptr, len, Buffer.GetRawPtr() + BufPos, bufBytes);
//        BufPos += bufBytes;
//
//        const auto stillNeed = len - bufBytes;
//        if (stillNeed == 0)
//            return true;
//        // force bypass buffer
//        if (stillNeed >= Buffer.GetSize())
//        {
//            const auto ret = File.Read(stillNeed, reinterpret_cast<uint8_t*>(ptr) + bufBytes);
//            BufBegin = File.CurrentPos();
//            LoadBuffer<false>();
//            return ret;
//        }
//        // 0 < stillNeed < BufSize
//        LoadBuffer();
//        if (stillNeed > BufLen) //simple reject
//            return false;
//        memcpy_s(reinterpret_cast<uint8_t*>(ptr) + bufBytes, stillNeed, Buffer.GetRawPtr(), stillNeed);
//        BufPos += stillNeed;
//        return true;
//    }
//
//    template<typename T = byte>
//    T ReadByteNE()//without checking
//    {
//        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
//        if (BufPos >= BufLen)
//            LoadBuffer();
//        return (T)Buffer[BufPos++];
//    }
//    template<typename T = byte>
//    T ReadByte()
//    {
//        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
//        if (BufPos >= BufLen)
//            LoadBuffer();
//        if (BufPos >= BufLen)
//            COMMON_THROW(FileException, FileException::Reason::ReadFail, File.Path(), u"reach end of file");
//        return (T)Buffer[BufPos++];
//    }
//
//    using Readable<BufferedFileReader>::Read;
//};

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

template<typename Char = char>
inline string ReadAllText(const fs::path& fpath)
{
    FileInputStream fis(FileObject::OpenThrow(fpath, OpenFlag::BINARY | OpenFlag::READ));
    std::basic_string<Char> text;
    fis.ReadInto(text, SIZE_MAX);
    return text;
}


template<class T>
inline void WriteAll(const fs::path& fpath, T& output)
{
    static_assert(std::is_class_v<T>, "WriteAll should accept container object");
    FileOutputStream fis(FileObject::OpenThrow(fpath, OpenFlag::CreatNewBinary));
    fis.WriteFrom(output, 0, SIZE_MAX);
}

}


//#undef FSeek64
//#undef FTell64
