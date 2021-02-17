#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef DXU_EXPORT
#   define DXUAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define DXUAPI _declspec(dllimport)
# endif
#else
# ifdef DXU_EXPORT
#   define DXUAPI [[gnu::visibility("default")]]
#   define COMMON_EXPORT
# else
#   define DXUAPI
# endif
#endif



#include "ImageUtil/ImageCore.h"
#include "ImageUtil/TexFormat.h"
#include "SystemCommon/HResultHelper.h"
#include "SystemCommon/PromiseTask.h"
#include "common/EnumEx.hpp"
#include "common/AtomicUtil.hpp"
#include "common/EasyIterator.hpp"
#include "common/FrozenDenseSet.hpp"
#include "common/FileBase.hpp"
#include "common/SpinLock.hpp"
#include "common/TimeUtil.hpp"
#include "common/Exceptions.hpp"
#include "common/CommonRely.hpp"

#if COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wmismatched-tags"
#endif
#include "3rdParty/half/half.hpp"
#if COMPILER_CLANG
#   pragma clang diagnostic pop
#endif


#if COMPILER_MSVC && !defined(_ENABLE_EXTENDED_ALIGNED_STORAGE)
#   error "require aligned storage fix"
#endif
#if !COMMON_OVER_ALIGNED
#   error require c++17 + aligned_new to support memory management for over-aligned SIMD type.
#endif


#include <cstddef>
#include <cstdio>
#include <memory>
#include <functional>
#include <type_traits>
#include <string.h>
#include <string>
#include <cstring>
#include <array>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <optional>
#include <variant>
#include <atomic>


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif



namespace dxu
{

template<typename T>
struct PtrProxy
{
    void* Pointer;
    constexpr PtrProxy() noexcept : Pointer(nullptr) {}
    constexpr PtrProxy(const PtrProxy<T>& ptr) noexcept : Pointer(ptr.Pointer)
    {
        if (Pointer)
            Ptr()->AddRef();
    }
    constexpr PtrProxy(PtrProxy<T>&& ptr) noexcept : Pointer(ptr.Pointer)
    {
        ptr.Pointer = nullptr;
    }
    explicit constexpr PtrProxy(void* ptr) noexcept : Pointer(ptr) {}
    ~PtrProxy()
    {
        if (Pointer)
        {
            Ptr()->Release();
            Pointer = nullptr;
        }
    }
    PtrProxy<T>& operator=(const PtrProxy<T>&) noexcept = delete;
    PtrProxy<T>& operator=(PtrProxy<T>&&) noexcept = delete;
    T* Ptr() const noexcept 
    {
        return reinterpret_cast<T*>(Pointer);
    }
    template<typename U>
    PtrProxy<U>& AsDerive() noexcept
    {
        static_assert(std::is_base_of_v<typename T::RealType, typename U::RealType>);
        return *reinterpret_cast<PtrProxy<U>*>(this);
    }
    operator T* () const noexcept
    {
        return Ptr();
    }
    constexpr explicit operator bool() const noexcept
    {
        return Pointer != nullptr;
    }
    T& operator*() const noexcept
    {
        return *Ptr();
    }
    T* operator->() const noexcept
    {
        return Ptr();
    }
    auto operator&() noexcept
    {
        return reinterpret_cast<typename T::RealType**>(&Pointer);
    }
    auto operator&() const noexcept
    {
        return reinterpret_cast<typename T::RealType* const*>(&Pointer);
    }
};


class DXUAPI DxNamable
{
protected:
    std::u16string Name;
private:
    virtual void* GetD3D12Object() const noexcept = 0;
public:
    virtual ~DxNamable();
    void SetName(std::u16string name);
    forceinline std::u16string_view GetName() const noexcept { return Name; }
};


class DxException : public common::BaseException
{
public:
    enum class DXComponent { Compiler, Driver, DXU };
private:
    PREPARE_EXCEPTION(DxException, BaseException,
        template<typename T>
        ExceptionInfo(T&& msg)
            : ExceptionInfo(TYPENAME, std::forward<T>(msg))
        { }
    protected:
        template<typename T>
        ExceptionInfo(const char* type, T&& msg)
            : TPInfo(type, std::forward<T>(msg))
        { }
    );
    DxException(std::u16string msg);
    DxException(common::HResultHolder hresult, std::u16string msg);
};


enum class HeapType     : uint8_t { Default = 1, Upload, Readback, Custom };
enum class CPUPageProps : uint8_t { Unknown, NotAvailable, WriteCombine, WriteBack };
enum class MemPrefer    : uint8_t { Unknown, PreferCPU, PreferGPU };

enum class ResourceState : uint32_t
{
    Common              = 0, 
    ConstBuffer         = 0x1, 
    IndexBuffer         = 0x2, 
    RenderTarget        = 0x4, 
    UnorderAccess       = 0x8, 
    DepthWrite          = 0x10, 
    DepthRead           = 0x20, 
    NonPSRes            = 0x40, 
    PsRes               = 0x80, 
    StreamOut           = 0x100, 
    IndirectArg         = 0x200,
    CopyDst             = 0x400, 
    CopySrc             = 0x800, 
    ResolveSrc          = 0x1000, 
    ResolveDst          = 0x2000, 
    RTStruct            = 0x400000, 
    ShadeRateRes        = 0x1000000, 
    Read                = ConstBuffer | IndexBuffer | NonPSRes | PsRes | IndirectArg | CopySrc, 
    Present             = 0, 
    Pred                = 0x200,
    VideoDecodeRead     = 0x10000, 
    VideoDecodeWrite    = 0x20000, 
    VideoProcessRead    = 0x40000, 
    VideoProcessWrite   = 0x80000, 
    VideoEncodeRead     = 0x200000, 
    VideoEncodeWrite    = 0x800000,
};


enum class BoundedResourceType : uint16_t
{
    RWMask   = 0x0100,
    TypeMask = 0xf000, OtherType = 0x0000, UAVType = 0x1000, SRVType = 0x2000, CBVType = 0x3000, SamplerType = 0x4000,
    Other           =   OtherType | 1, 
    CBuffer         =     CBVType | 1,
    TBuffer         =     SRVType | 1,
    Texture         =     SRVType | 2, 
    Sampler         = SamplerType | 1, 
    TypedUAV        =     UAVType | 1,
    UntypedUAV      =     UAVType | 2,
    RawUAV          =     UAVType | 3,
    RWTypedUAV      = RWMask | TypedUAV,
    RWUntypedUAV    = RWMask | UntypedUAV,
    RWRawUAV        = RWMask | RawUAV,
};
MAKE_ENUM_BITFIELD(BoundedResourceType)


namespace detail
{
struct IIDPPVPair;
struct IIDData;

struct Adapter;
struct Device;
struct CmdAllocator;
struct CmdList;
struct CmdQue;
struct Fence;
struct Resource;
struct ResourceDesc;
struct DescHeap;
struct BindResourceDetail;
struct RootSignature;
struct PipelineState;


DXUAPI [[nodiscard]] std::string_view GetBoundedResTypeName(const BoundedResourceType type) noexcept;
}


//struct ContextCapability
//{
//    common::container::FrozenDenseSet<std::string_view> Extensions;
//    std::u16string VendorString;
//    std::u16string VersionString;
//    uint32_t Version            = 0;
//    bool SupportDebug           = false;
//    bool SupportSRGB            = false;
//    bool SupportClipControl     = false;
//    bool SupportGeometryShader  = false;
//    bool SupportComputeShader   = false;
//    bool SupportTessShader      = false;
//    bool SupportBindlessTexture = false;
//    bool SupportImageLoadStore  = false;
//    bool SupportSubroutine      = false;
//    bool SupportIndirectDraw    = false;
//    bool SupportBaseInstance    = false;
//    bool SupportVSMultiLayer    = false;
//
//    DXUAPI std::string GenerateSupportLog() const;
//};


}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif