#include "FileEx.h"

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <variant>


#if COMMON_OS_WIN
#else
#   include <unistd.h>
#   include <cerrno>
#endif

#if COMMON_COMPILER_GCC
#   define TMP_COMPILER_NAME "GCC"
#   define TMP_COMPILER_VER  __GNUC__.__GNUC_MINOR__.__GNUC_PATCHLEVEL__
#elif COMMON_COMPILER_CLANG
#   define TMP_COMPILER_NAME "clang"
#   define TMP_COMPILER_VER  __clang_major__.__clang_minor__.__clang_patchlevel__
#elif COMMON_COMPILER_MSVC
#   define TMP_COMPILER_NAME "MSVC"
#   define TMP_COMPILER_VER  _MSC_VER
#else
#   define TMP_COMPILER_NAME 
#   define TMP_COMPILER_VER  
#endif
#if COMMON_FS == 1
#   define TMP_FS_NAME "filesystem"
#   define TMP_FS_VER  __cpp_lib_filesystem
#elif COMMON_FS == 2
#   define TMP_FS_NAME "experimental/filesystem"
#   define TMP_FS_VER  __cpp_lib_experimental_filesystem
#elif COMMON_FS == 3
#   define TMP_FS_NAME "ghc-filesystem"
#   define TMP_FS_VER GHC_FILESYSTEM_VERSION
#else
#   define TMP_FS_NAME
#   define TMP_FS_VER
#endif
#pragma message("Compiling SystemCommon with [" TMP_FS_NAME "(" STRINGIZE(TMP_FS_VER) ")] from [" TMP_COMPILER_NAME "][" STRINGIZE(TMP_COMPILER_VER) "]")

namespace common::file
{
using std::string;
using std::u16string;
using std::byte;
MAKE_ENABLER_IMPL(FileObject)


COMMON_EXCEPTION_IMPL(FileException)


#if COMMON_OS_WIN
using FlagType = wchar_t;
#   define StrText(x) L ##x
#else
using FlagType = char;
#   define StrText(x) x
#endif
static constexpr const FlagType* ParseFlag(OpenFlag flag)
{
    const bool isText = HAS_FIELD(flag, OpenFlag::FLAG_TEXT);
    flag = REMOVE_MASK(flag, OpenFlag::MASK_EXTEND, OpenFlag::FLAG_TEXT);
    switch (flag)
    {
    case OpenFlag::FLAG_READ:
        return isText ? StrText("r")    : StrText("rb");
    case OpenFlag::FLAG_READWRITE:
        return isText ? StrText("r+")   : StrText("r+b");
    case OpenFlag::FLAG_WRITE     | OpenFlag::FLAG_CREATE:
        return isText ? StrText("wx")   : StrText("wxb");
    case OpenFlag::FLAG_READWRITE | OpenFlag::FLAG_CREATE:
        return isText ? StrText("w+x")  : StrText("w+xb");
    case OpenFlag::FLAG_WRITE     | OpenFlag::FLAG_CREATE | OpenFlag::FLAG_TRUNC:
        return isText ? StrText("w") : StrText("wb");
    case OpenFlag::FLAG_READWRITE | OpenFlag::FLAG_CREATE | OpenFlag::FLAG_TRUNC:
        return isText ? StrText("w+")   : StrText("w+b");
    case OpenFlag::Append:
        return isText ? StrText("a")    : StrText("ab");
    case OpenFlag::Append | OpenFlag::FLAG_READ:
        return isText ? StrText("a+")   : StrText("a+b");
    default:      
        return StrText("");
    }
}
#undef StrText


FileObject::FileObject(const fs::path& path, FILE* fp, const OpenFlag flag) : 
    FilePath(path), FHandle(fp), Flag(flag)
{
    if (HAS_FIELD(flag, OpenFlag::FLAG_DontBuffer))
        ::std::setvbuf(FHandle, NULL, _IONBF, 0);
    else
        ::std::setvbuf(FHandle, NULL, _IOFBF, 16384);
}
FileObject::~FileObject()
{
    if (FHandle != nullptr)
        fclose(FHandle);
}

#if COMMON_OS_WIN
#   define ErrType errno_t
#else
#   define ErrType int
#endif
static std::variant<FILE*, ErrType> TryOpen(const fs::path& path, const OpenFlag flag)
{
    std::error_code ec;
    if (!fs::exists(path, ec) && !HAS_FIELD(flag, OpenFlag::FLAG_CREATE))
        return ENOENT;
    FILE* fp = nullptr;
#if COMMON_OS_WIN
    ErrType err = _wfopen_s(&fp, path.wstring().c_str(), ParseFlag(flag));
#else
    fp = fopen(reinterpret_cast<const char*>(path.u8string().c_str()), ParseFlag(flag));
    ErrType err = (fp == nullptr) ? errno : 0;
#endif
    if (fp != nullptr && err == 0)
        return fp;
    else
        return err;
}

std::shared_ptr<FileObject> FileObject::OpenFile(const fs::path& path, const OpenFlag flag)
{
    const auto ret = TryOpen(path, flag);
    if (ret.index() == 0)
        return MAKE_ENABLER_SHARED(FileObject, (path, std::get<0>(ret), flag));
    else
        return {};
}

std::shared_ptr<FileObject> FileObject::OpenThrow(const fs::path& path, const OpenFlag flag)
{
    const auto ret = TryOpen(path, flag);
    if (ret.index() == 0)
        return MAKE_ENABLER_SHARED(FileObject, (path, std::get<0>(ret), flag));
    switch (std::get<1>(ret))
    {
    case ENOENT:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::NotExist      , path, u"cannot open target file, not exists");
    case ENOMEM:    throw std::bad_alloc(/*"fopen reported no enough memory error"*/);
    case EACCES:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::PermissionDeny, path, u"cannot open target file, no permission");
    case EEXIST:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::AlreadyExist  , path, u"cannot open target file, already exists");
    case EISDIR:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::IsDir         , path, u"cannot open target file, is a directory");
    case EINVAL:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::WrongParam    , path, u"cannot open target file, invalid params");
    default:        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::UnknowErr     , path, u"cannot open target file");
    }
}


FileStream::FileStream(std::shared_ptr<FileObject>&& file) noexcept : 
    File(std::move(file)) { }

FileStream::~FileStream() { }

bool FileStream::FSeek(const int64_t offset, SeekWhere whence)
{
#if COMMON_OS_WIN
    return _fseeki64(GetFP(), offset, common::enum_cast(whence)) == 0;
#else
    return fseeko64(GetFP(), offset, common::enum_cast(whence)) == 0;
#endif
}
size_t FileStream::FTell() const
{
#if COMMON_OS_WIN
    return _ftelli64(GetFP());
#else
    return ftello64(GetFP());
#endif
}

FILE* FileStream::GetFP() const { return File->FHandle; }

void FileStream::WriteCheck() const
{
    if (!HAS_FIELD(File->Flag, OpenFlag::FLAG_WRITE))
        COMMON_THROW(FileException, FileErrReason::WriteFail | FileErrReason::OpMismatch, File->FilePath, u"not opened for write");
}

void FileStream::ReadCheck() const
{
    if (!HAS_FIELD(File->Flag, OpenFlag::FLAG_READ))
        COMMON_THROW(FileException, FileErrReason::ReadFail  | FileErrReason::OpMismatch, File->FilePath, u"not opened for read");
}

void FileStream::CheckError(FileErrReason fileop)
{
    fileop &= FileErrReason::MASK_OP;
    if (feof(GetFP()) != 0)
        COMMON_THROW(FileException, fileop | FileErrReason::EndOfFile, File->FilePath, u"reach end of file");
    if (ferror(GetFP()) != 0)
        COMMON_THROW(FileException, fileop | FileErrReason::EndOfFile, File->FilePath, u"access file failed");
    else
        COMMON_THROW(FileException, fileop | FileErrReason::UnknowErr, File->FilePath, u"unknown error");
}

size_t FileStream::LeftSpace()
{
    const auto cur = FTell();
    FSeek(0, SeekWhere::End);
    const auto flen = FTell();
    SetPos(cur);
    return flen - cur;
}

//==========RandomStream=========//
size_t FileStream::GetSize()
{
    const auto cur = FTell();
    FSeek(0, SeekWhere::End);
    const auto flen = FTell();
    SetPos(cur);
    return flen;
}
size_t FileStream::CurrentPos() const
{
    return FTell();
}
bool FileStream::SetPos(const size_t offset)
{
    uint64_t left = offset;
    bool isFirst = true;
    do
    {
        const uint64_t step = std::min<uint64_t>(offset, INT64_MAX);
        if (!FSeek(static_cast<int64_t>(step), isFirst ? SeekWhere::Beginning : SeekWhere::Current))
            return false;
        left -= step;
        isFirst = false;
    } while (left > 0);
    return true;
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
    uint64_t left = len;
    while (left > 0)
    {
        const uint64_t step = std::min<uint64_t>(len, INT64_MAX);
        if (!FSeek(static_cast<int64_t>(step), SeekWhere::Current))
            return false;
        left -= step;
    }
    return true;
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
        CheckError(FileErrReason::ReadFail);
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

FileOutputStream::FileOutputStream(FileOutputStream&& stream) noexcept : FileStream(std::move(stream.File)) { }

FileOutputStream::~FileOutputStream() { Flush(); }

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

//=======RandomOutputStream======//
bool FileOutputStream::ReSize(const size_t newSize)
{
    const auto totalSize = FileStream::GetSize();
    if (newSize > totalSize)
    {
        const auto curPos = FileStream::CurrentPos();
        if (FileStream::SetPos(newSize - 1))
        {
            if (WriteMany(1, 1, "\0") == 1)
            {
                FileStream::SetPos(curPos);
                return true;
            }
        }
        FileStream::SetPos(curPos);
    }
    else if (newSize == totalSize)
        return true;
    return false;
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
