#include "SystemCommonPch.h"
#include "FileMapperEx.h"

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>

#if COMMON_OS_MACOS
#   include <mach-o/dyld.h>
#endif

namespace common::file
{
using std::string;
using std::u16string;
using std::byte;
MAKE_ENABLER_IMPL(FileMappingObject)

fs::path LocateCurrentExecutable() noexcept
{
#if COMMON_OS_WIN
    std::vector<wchar_t> tmp(64, L'\0');
    while (true)
    {
        const auto result = GetModuleFileNameW(nullptr, tmp.data(), static_cast<DWORD>(tmp.size())); // ends in '\0'
        if (result == tmp.size())
        {
            const auto lastError = GetLastError();
            if (lastError == ERROR_INSUFFICIENT_BUFFER)
            {
                tmp.resize(tmp.size() * 2);
                continue;
            }
        }
        if (result > 0)
            tmp.resize(result);
        else
            tmp.clear();
        break;
    }
#elif COMMON_OS_MACOS
    std::vector<char> tmp(512, '\0');
    uint32_t size = static_cast<uint32_t>(tmp.size());
    while (true)
    {
        const auto result = _NSGetExecutablePath(tmp.data(), &size);
        if (result == -1)
        {
            tmp.resize(size);
            std::fill(tmp.begin(), tmp.end(), '\0');
            continue;
        }
        if (result == 0)
        {
            const auto len = std::char_traits<char>::length(tmp.data());
            tmp.resize(len);
        }
        else
            tmp.clear();
        break;
    }
#elif COMMON_OS_LINUX
    std::vector<char> tmp(512, '\0');
    while (true)
    {
        const auto result = readlink("/proc/self/exe", tmp.data(), tmp.size());
        if (result > 0)
        {
            if (static_cast<size_t>(result) < tmp.size())
            {
                tmp.resize(result);
                break;
            }
            tmp.resize(tmp.size() * 2);
            continue;
        }
        tmp.clear();
        break;
    }
#else
    std::vector<char> tmp;
#endif
    if (!tmp.empty())
    {
        common::fs::path ret(tmp.begin(), tmp.end());
        try
        {
            ret = fs::weakly_canonical(ret);
        }
        catch (...)
        {
        }
        return ret.make_preferred();
    }
    return {};
}


FileMappingObject::FileMappingObject(std::shared_ptr<RawFileObject> file, const MappingFlag flag,
    const HandleType handle, const uint64_t offset, std::byte* ptr, const size_t size)
    : RawFile(std::move(file)), Offset(offset), MappingHandle(handle), Ptr(ptr), Size(size), Flag(flag)
{ }


static std::optional<FileErrReason> CheckFlagCompatible(const OpenFlag openflag, const MappingFlag flag)
{
    if (!HAS_FIELD(openflag, OpenFlag::FLAG_READ))
        return FileErrReason::PermissionDeny;
    if (!HAS_FIELD(openflag, OpenFlag::FLAG_WRITE) && flag == MappingFlag::ReadWrite)
        return FileErrReason::PermissionDeny;
    return {};
}

static uint64_t GetMappingAlign()
{
#if COMMON_OS_WIN
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwAllocationGranularity;
#else
    return sysconf(_SC_PAGE_SIZE);
#endif
}

static std::tuple<uint64_t, uint64_t> HandleMappingAlign(const uint64_t offset)
{
    static auto Requirement = GetMappingAlign();
    const auto newOffset = offset & ~(Requirement - 1);
    const auto padding = newOffset - offset;
    return { newOffset, padding };
}

static std::optional<std::tuple<FileMappingObject::HandleType, byte*, uint64_t, size_t>>
TryMap (RawFileObject::HandleType fileHandle, const MappingFlag flag, const std::pair<uint64_t, uint64_t>& region)
{
    Expects(region.second == UINT64_MAX || ((region.first + region.second) > region.first));
    Expects(region.second == UINT64_MAX || ((region.first + region.second) > region.second));
#if COMMON_OS_WIN
    LARGE_INTEGER realMapSize;
    if (!GetFileSizeEx(fileHandle, &realMapSize))
        return {};
    const uint64_t realLen = std::max<uint64_t>(realMapSize.QuadPart,
        region.second == UINT64_MAX ? region.first : region.first + region.second);
    realMapSize.QuadPart = saturate_cast<LONGLONG>(realLen);

    DWORD access = 0;
    if (flag == MappingFlag::ReadWrite)
        access = PAGE_READWRITE;
    else if (flag == MappingFlag::CopyOnWrite)
        access = PAGE_WRITECOPY;
    else if (flag == MappingFlag::ReadOnly)
        access = PAGE_READONLY;

    const auto mapHandle = CreateFileMapping(fileHandle, nullptr, access, realMapSize.HighPart, realMapSize.LowPart, nullptr);
    if (mapHandle != NULL)
    {
        DWORD mapMode = 0;
        if (flag == MappingFlag::ReadWrite)         
            mapMode = FILE_MAP_WRITE;
        else if (flag == MappingFlag::CopyOnWrite)  
            mapMode = FILE_MAP_COPY;
        else if (flag == MappingFlag::ReadOnly)     
            mapMode = FILE_MAP_READ;

        const auto [offset, padding] = HandleMappingAlign(region.first);
        LARGE_INTEGER realOffset; realOffset.QuadPart = static_cast<LONGLONG>(offset);
        const size_t realSize = offset < realLen ? saturate_cast<size_t>(realLen - offset) : 0u;

        const auto ptr = MapViewOfFile(mapHandle, mapMode, realOffset.HighPart, realOffset.LowPart, realSize);

        if (ptr != NULL)
        {
            const auto realPtr = reinterpret_cast<byte*>(ptr) + padding;
            return std::make_tuple(FileMappingObject::HandleType{ mapHandle, ptr }, realPtr, offset, realSize);
        }

        CloseHandle(mapHandle);
    }
    return {};
#else
    struct stat64 info;
    if (fstat64(fileHandle, &info) == -1)
        return {};
    const uint64_t realLen = std::max<uint64_t>(info.st_size,
        region.second == UINT64_MAX ? region.first : region.first + region.second);

    int prot = PROT_NONE;
    if (flag == MappingFlag::ReadWrite)
        prot = PROT_READ | PROT_WRITE;
    else if (flag == MappingFlag::CopyOnWrite)
        prot = PROT_READ | PROT_WRITE;
    else if (flag == MappingFlag::ReadOnly)
        prot = PROT_READ;

    const auto [offset, padding] = HandleMappingAlign(region.first);
    const size_t realSize = offset < realLen ? saturate_cast<size_t>(realLen - offset) : 0u;

    const auto ptr = mmap(nullptr, realSize, prot, MAP_PRIVATE, fileHandle, offset);
    
    if (ptr != MAP_FAILED)
    {
        const auto realPtr = reinterpret_cast<byte*>(ptr) + padding;
        return std::make_tuple(ptr, realPtr, offset, realSize);
    }
    
    return {};
#endif
}

std::shared_ptr<FileMappingObject> FileMappingObject::OpenMapping(std::shared_ptr<RawFileObject> rawFile,
    const MappingFlag flag, const std::pair<uint64_t, uint64_t>& region)
{
    if (const auto err = CheckFlagCompatible(rawFile->Flag, flag); !err.has_value())
    {
        if (const auto ret = TryMap(rawFile->FileHandle, flag, region); ret.has_value())
        {
            const auto& [handle, ptr, offset, size] = *ret;
            return MAKE_ENABLER_SHARED(FileMappingObject, (std::move(rawFile), flag, handle, offset, ptr, size));
        }
    }
    return {};
}

std::shared_ptr<FileMappingObject> FileMappingObject::OpenThrow(std::shared_ptr<RawFileObject> rawFile, 
    const MappingFlag flag, const std::pair<uint64_t, uint64_t>& region)
{
    const auto& path = rawFile->Path();
    if (const auto err = CheckFlagCompatible(rawFile->Flag, flag); err.has_value())
        COMMON_THROW(FileException, FileErrReason::OpenFail | err.value(), path, u"cannot map target file, not exists");
    if (const auto ret = TryMap(rawFile->FileHandle, flag, region); ret.has_value())
    {
        const auto& [handle, ptr, offset, size] = *ret;
        return MAKE_ENABLER_SHARED(FileMappingObject, (std::move(rawFile), flag, handle, offset, ptr, size));
    }
#if COMMON_OS_WIN
    switch (GetLastError())
    {
    case ERROR_FILE_NOT_FOUND:      
        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::NotExist,       path, u"cannot map target file, not exists");
    case ERROR_ACCESS_DENIED:       
        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::PermissionDeny, path, u"cannot map target file, no permission");
    case ERROR_FILE_EXISTS:
        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::AlreadyExist,   path, u"cannot map target file, already exists");
    case ERROR_SHARING_VIOLATION:   
        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::SharingViolate, path, u"cannot map target file, conflict sharing");
    case ERROR_INVALID_PARAMETER:
        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::WrongParam,     path, u"cannot map target file, invalid params");
#else
    switch (errno)
    {
    case ENOENT:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::NotExist,          path, u"cannot map target file, not exists");
    case ENOMEM:    throw std::bad_alloc(/*"fopen reported no enough memory error"*/);
    case EACCES:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::PermissionDeny,    path, u"cannot map target file, no permission");
    case EEXIST:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::AlreadyExist,      path, u"cannot map target file, already exists");
    case EINVAL:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::WrongParam,        path, u"cannot map target file, invalid params");
#endif
    default:        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::UnknowErr,         path, u"cannot map target file");
    }
}

FileMappingObject::~FileMappingObject()
{
#if COMMON_OS_WIN
    UnmapViewOfFile(MappingHandle.second);
    CloseHandle(MappingHandle.first);
#else
    munmap(MappingHandle, Size);
#endif
}


FileMappingStream::FileMappingStream(std::shared_ptr<FileMappingObject>&& mapping) noexcept
    : MappingObject(std::move(mapping)) { }

FileMappingStream::~FileMappingStream() { }

void FileMappingStream::WriteCheck() const
{
    if (MappingObject->Flag == MappingFlag::ReadOnly)
        COMMON_THROW(FileException, FileErrReason::WriteFail | FileErrReason::OpMismatch, MappingObject->Path(), u"not opened for write");
}

void FileMappingStream::ReadCheck() const
{ }

common::span<std::byte> FileMappingStream::GetSpan() const noexcept { return common::span<std::byte>(MappingObject->Ptr,MappingObject->Size); }

void FileMappingStream::FlushRange(const size_t offset, const size_t len, const bool async) const noexcept
{
#if COMMON_OS_WIN
    FlushViewOfFile(MappingObject->Ptr + offset, len);
    if (!async)
        FlushFileBuffers(MappingObject->GetRawFileHandle());
#else
    const auto [newOffset, padding] = HandleMappingAlign(offset);
    msync(MappingObject->Ptr + newOffset, 
        static_cast<size_t>(len + padding),
        (async ? MS_ASYNC : MS_SYNC) | MS_INVALIDATE);
#endif
}


FileMappingInputStream::FileMappingInputStream(std::shared_ptr<FileMappingObject> mapping)
    : FileMappingStream(std::move(mapping)), 
    MemoryInputStream(this->GetSpan()) { }

FileMappingInputStream::FileMappingInputStream(FileMappingInputStream&& stream) noexcept
    : FileMappingStream(std::move(stream.MappingObject)),
    MemoryInputStream(std::move(stream)) { }

FileMappingInputStream::~FileMappingInputStream()
{
}

FileMappingOutputStream::FileMappingOutputStream(std::shared_ptr<FileMappingObject> mapping)
    : FileMappingStream(std::move(mapping)),
    MemoryOutputStream(this->GetSpan()),
    ModifyRange({ 0,0 }) { }

FileMappingOutputStream::FileMappingOutputStream(FileMappingOutputStream&& stream) noexcept
    : FileMappingStream(std::move(stream.MappingObject)),
    MemoryOutputStream(std::move(stream)),
    ModifyRange(stream.ModifyRange) { }

FileMappingOutputStream::~FileMappingOutputStream()
{
    if (MappingObject)
        Flush();
}

bool FileMappingOutputStream::SetPos(const size_t pos) noexcept
{
    Flush();
    return MemoryOutputStream::SetPos(pos);
}

size_t FileMappingOutputStream::WriteMany(const size_t want, const size_t perSize, const void* ptr)
{
    const auto curPos = CurrentPos();
    const auto cnt = MemoryOutputStream::WriteMany(want, perSize, ptr);
    if (ModifyRange.second == 0)
        ModifyRange.first = curPos;
    ModifyRange.second += cnt * perSize;
    return cnt;
}

void FileMappingOutputStream::Flush() noexcept
{
    FlushRange(ModifyRange.first, ModifyRange.second);
    ModifyRange.second = 0;
}

FileMappingInputStream MapFileForRead(const fs::path& fpath)
{
    return FileMappingInputStream(
        FileMappingObject::OpenThrow(
            RawFileObject::OpenThrow(fpath, OpenFlag::ReadBinary),
            MappingFlag::ReadOnly)
    );
}

}
