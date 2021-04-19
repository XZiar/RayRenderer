#include "SystemCommonPch.h"
#include "RawFileEx.h"
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <string>


namespace common::file
{
using std::string;
using std::u16string;
using std::byte;
MAKE_ENABLER_IMPL(RawFileObject)


RawFileObject::RawFileObject(const fs::path& path, const HandleType fileHandle, const OpenFlag flag) :
    FilePath(path), FileHandle(fileHandle), Flag(flag)
{ }
RawFileObject::~RawFileObject()
{
#if COMMON_OS_WIN
    CloseHandle(FileHandle);
#else
    close(FileHandle);
#endif
}


static std::optional<RawFileObject::HandleType> TryOpen(const fs::path& path, const OpenFlag flag)
{
#if COMMON_OS_WIN
    DWORD accessMode = 0L, shareMode = 0L, createMode = 0, realFlag = FILE_ATTRIBUTE_NORMAL;

    if (HAS_FIELD(flag, OpenFlag::FLAG_READ))           accessMode |= GENERIC_READ;
    if (HAS_FIELD(flag, OpenFlag::FLAG_WRITE))          accessMode |= GENERIC_WRITE;

    if (HAS_FIELD(flag, OpenFlag::FLAG_SHARE_READ))     shareMode  |= FILE_SHARE_READ;
    if (HAS_FIELD(flag, OpenFlag::FLAG_SHARE_WRITE))    shareMode  |= FILE_SHARE_WRITE;

    if (HAS_FIELD(flag, OpenFlag::FLAG_DontBuffer))     realFlag   |= FILE_FLAG_NO_BUFFERING;
    if (HAS_FIELD(flag, OpenFlag::FLAG_WriteThrough))   realFlag   |= FILE_FLAG_WRITE_THROUGH;
    if (HAS_FIELD(flag, OpenFlag::FLAG_DeleteOnClose))  realFlag   |= FILE_FLAG_DELETE_ON_CLOSE;

    if (HAS_FIELD(flag, OpenFlag::FLAG_CREATE))
        createMode = HAS_FIELD(flag, OpenFlag::FLAG_TRUNC) ? CREATE_ALWAYS      : CREATE_NEW;
    else
        createMode = HAS_FIELD(flag, OpenFlag::FLAG_TRUNC) ? TRUNCATE_EXISTING  : OPEN_EXISTING;

    auto handle = CreateFile(
        path.wstring().c_str(), 
        accessMode, 
        shareMode, 
        NULL, 
        createMode,
        realFlag, 
        nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        return { };
    else
        return { handle };
#else
    int realFlag = O_LARGEFILE;
    if (HAS_FIELD(flag, OpenFlag::FLAG_READ))
    {
        if (HAS_FIELD(flag, OpenFlag::FLAG_WRITE))
            realFlag |= O_RDWR;
        else
            realFlag |= O_RDONLY;
    }
    else
    {
        if (HAS_FIELD(flag, OpenFlag::FLAG_WRITE))
            realFlag |= O_WRONLY;
    }

    if (HAS_FIELD(flag, OpenFlag::FLAG_APPEND))
        realFlag |= O_APPEND;

    if (HAS_FIELD(flag, OpenFlag::FLAG_CREATE))
    {
        realFlag |= O_CREAT;
        realFlag |= HAS_FIELD(flag, OpenFlag::FLAG_TRUNC) ? O_TRUNC : O_EXCL;
    }
    else if (HAS_FIELD(flag, OpenFlag::FLAG_TRUNC))
        realFlag |= O_TRUNC;

    if (HAS_FIELD(flag, OpenFlag::FLAG_WriteThrough))
        realFlag |= O_DSYNC;

    if (HAS_FIELD(flag, OpenFlag::FLAG_DeleteOnClose))
        realFlag |= O_CLOEXEC;

# if !COMMON_OS_MACOS
    if (HAS_FIELD(flag, OpenFlag::FLAG_DontBuffer))
        realFlag |= O_DIRECT;
# endif

    auto handle = open(path.string().c_str(), realFlag);
    if (handle == -1)
        return { };
# if COMMON_OS_MACOS
    if (HAS_FIELD(flag, OpenFlag::FLAG_DontBuffer))
        fcntl(handle, F_NOCACHE, 1);
# endif
    return { handle };
#endif
}

std::shared_ptr<RawFileObject> RawFileObject::OpenFile(const fs::path& path, const OpenFlag flag)
{
    const auto ret = TryOpen(path, flag);
    if (ret.has_value())
        return MAKE_ENABLER_SHARED(RawFileObject, (path, *ret, flag));
    else
        return {};
}

std::shared_ptr<RawFileObject> RawFileObject::OpenThrow(const fs::path& path, const OpenFlag flag)
{
    const auto ret = TryOpen(path, flag);
    if (ret.has_value())
        return MAKE_ENABLER_SHARED(RawFileObject, (path, *ret, flag));
#if COMMON_OS_WIN
    switch (GetLastError())
    {
    case ERROR_FILE_NOT_FOUND:      
        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::NotExist,       path, u"cannot open target file, not exists");
    case ERROR_ACCESS_DENIED:       
        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::PermissionDeny, path, u"cannot open target file, no permission");
    case ERROR_FILE_EXISTS:
        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::AlreadyExist,   path, u"cannot open target file, already exists");
    case ERROR_SHARING_VIOLATION:   
        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::SharingViolate, path, u"cannot open target file, conflict sharing");
    case ERROR_INVALID_PARAMETER:
        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::WrongParam,     path, u"cannot open target file, invalid params");
#else
    switch (errno)
    {
    case ENOENT:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::NotExist,          path, u"cannot open target file, not exists");
    case ENOMEM:    throw std::bad_alloc(/*"fopen reported no enough memory error"*/);
    case EACCES:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::PermissionDeny,    path, u"cannot open target file, no permission");
    case EEXIST:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::AlreadyExist,      path, u"cannot open target file, already exists");
    case EINVAL:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::WrongParam,        path, u"cannot open target file, invalid params");
#endif
    default:        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::UnknowErr,         path, u"cannot open target file");
    }
}

RawFileStream::RawFileStream(std::shared_ptr<RawFileObject>&& file) noexcept :
    File(std::move(file)) { }

RawFileStream::~RawFileStream() { }

RawFileObject::HandleType RawFileStream::GetHandle() const
{
    return File->FileHandle;
}

void RawFileStream::WriteCheck() const
{
    if (!HAS_FIELD(File->Flag, OpenFlag::FLAG_WRITE))
        COMMON_THROW(FileException, FileErrReason::WriteFail | FileErrReason::OpMismatch, File->FilePath, u"not opened for write");
}

void RawFileStream::ReadCheck() const
{
    if (!HAS_FIELD(File->Flag, OpenFlag::FLAG_READ))
        COMMON_THROW(FileException, FileErrReason::ReadFail | FileErrReason::OpMismatch, File->FilePath, u"not opened for read");
}

bool RawFileStream::FSeek(const int64_t offset, const SeekWhere whence)
{
#if COMMON_OS_WIN
    LARGE_INTEGER tmp, newpos;
    tmp.QuadPart = (LONGLONG)offset;
    return SetFilePointerEx(File->FileHandle, tmp, &newpos, common::enum_cast(whence)) != 0;
#else
    return lseek64(File->FileHandle, offset, common::enum_cast(whence)) != -1;
#endif
}
size_t RawFileStream::LeftSpace()
{
    const auto total = GetSize();
    if (total == 0)
        return 0;
    const auto cur = CurrentPos();
    return total - cur;
}

//==========RandomStream=========//
size_t RawFileStream::GetSize()
{
#if COMMON_OS_WIN
    LARGE_INTEGER tmp;
    if (GetFileSizeEx(GetHandle(), &tmp))
        return tmp.QuadPart;
    else
        return 0;
#else
    struct stat64 info;
    if (fstat64(GetHandle(), &info) == 0)
        return info.st_size;
    else
        return 0;
#endif
}
size_t RawFileStream::CurrentPos() const
{
#if COMMON_OS_WIN
    LARGE_INTEGER tmp, curpos;
    tmp.QuadPart = 0;
    if (SetFilePointerEx(GetHandle(), tmp, &curpos, FILE_CURRENT))
        return curpos.QuadPart;
    else
        return 0;
#else
    const auto curpos = lseek64(GetHandle(), 0, SEEK_CUR);
    if (curpos != -1)
        return curpos;
    else
        return 0;
#endif
}
bool RawFileStream::SetPos(const size_t offset)
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


RawFileInputStream::RawFileInputStream(std::shared_ptr<RawFileObject> file) :
    RawFileStream(std::move(file)) 
{
    ReadCheck();
}

RawFileInputStream::RawFileInputStream(RawFileInputStream&& stream) noexcept :
    RawFileStream(std::move(stream.File)) { }

RawFileInputStream::~RawFileInputStream() { }

//==========InputStream=========//
size_t RawFileInputStream::AvaliableSpace()
{
    return LeftSpace();
}
bool RawFileInputStream::Read(const size_t len, void* ptr)
{
    return ReadMany(len, 1, ptr) == len;
}
size_t RawFileInputStream::ReadMany(const size_t want, const size_t perSize, void* ptr)
{
    uint64_t left = want * perSize;
    while (left > 0)
    {
#if COMMON_OS_WIN
        const uint64_t need = std::min<uint64_t>(left, UINT32_MAX);
        DWORD newread = 0;
        if (!ReadFile(GetHandle(), ptr, static_cast<DWORD>(need), &newread, NULL) || newread == 0)
            break;
#else
        const uint64_t need = std::min<uint64_t>(left, 0x7ffff000u);
        const auto newread = read(GetHandle(), ptr, static_cast<uint32_t>(need));
        if (newread == -1)
            break;
#endif
        left -= newread;
    }
    return (want * perSize - left) / perSize;
}
bool RawFileInputStream::Skip(const size_t len)
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
bool RawFileInputStream::IsEnd() { return LeftSpace() == 0; }


//==========RandomStream=========//
size_t RawFileInputStream::GetSize()
{
    return RawFileStream::GetSize();
}
size_t RawFileInputStream::CurrentPos() const
{
    return RawFileStream::CurrentPos();
}
bool RawFileInputStream::SetPos(const size_t offset)
{
    return RawFileStream::SetPos(offset);
}


RawFileOutputStream::RawFileOutputStream(std::shared_ptr<RawFileObject> file) :
    RawFileStream(std::move(file)) {
    WriteCheck();
}

RawFileOutputStream::RawFileOutputStream(RawFileOutputStream&& stream) noexcept : RawFileStream(std::move(stream.File)) { }

RawFileOutputStream::~RawFileOutputStream() { Flush(); }

//==========OutputStream=========//
size_t RawFileOutputStream::AcceptableSpace()
{
    return SIZE_MAX;
}
bool RawFileOutputStream::Write(const size_t len, const void* ptr)
{
    return WriteMany(len, 1, ptr) == len;
}
size_t RawFileOutputStream::WriteMany(const size_t want, const size_t perSize, const void* ptr)
{
    uint64_t left = want * perSize;
    while (left > 0)
    {
#if COMMON_OS_WIN
        const uint64_t need = std::min<uint64_t>(left, UINT32_MAX);
        DWORD newwrite = 0;
        if (!WriteFile(GetHandle(), ptr, static_cast<DWORD>(need), &newwrite, NULL) || newwrite == 0)
            break;
#else
        const uint64_t need = std::min<uint64_t>(left, 0x7ffff000u);
        const auto newwrite = write(GetHandle(), ptr, static_cast<uint32_t>(need));
        if (newwrite == -1)
            break;
#endif
        left -= newwrite;
    }
    return (want * perSize - left) / perSize;
}
void RawFileOutputStream::Flush()
{
#if COMMON_OS_WIN
    FlushFileBuffers(GetHandle());
#else
    fdatasync(GetHandle());
#endif
}

//==========RandomStream=========//
size_t RawFileOutputStream::GetSize()
{
    return RawFileStream::GetSize();
}
size_t RawFileOutputStream::CurrentPos() const
{
    return RawFileStream::CurrentPos();
}
bool RawFileOutputStream::SetPos(const size_t offset)
{
    return RawFileStream::SetPos(offset);
}




}
