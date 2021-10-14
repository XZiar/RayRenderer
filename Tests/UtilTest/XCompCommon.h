#include "XComputeBase/XCompNailang.h"
#include "Nailang/NailangStruct.h"
#include <deque>
#include <any>
#include <memory>


template<size_t N>
inline std::string Hex2Str(const std::array<std::byte, N>& data)
{
    constexpr auto ch = "0123456789abcdef";
    std::string ret;
    ret.reserve(N * 2);
    for (size_t i = 0; i < N; ++i)
    {
        const uint8_t dat = static_cast<uint8_t>(data[i]);
        ret.push_back(ch[dat / 16]);
        ret.push_back(ch[dat % 16]);
    }
    return ret;
}

struct RunArgInfo
{
    enum class ArgType : uint16_t { Buffer, Image, Val8, Val16, Val32, Val64 };
    uint64_t Meta0;
    uint32_t Meta1;
    uint32_t Val0;
    uint32_t Val1;
    ArgType Type;
    constexpr RunArgInfo(uint64_t meta0, uint32_t meta1, ArgType type) noexcept : 
        Meta0(meta0), Meta1(meta1), Val0(0), Val1(0), Type(type)
    { }
    static std::u16string_view GetTypeName(const ArgType type) noexcept;
};


struct RunConfig
{
    std::u32string Name;
    std::string KernelName;
    std::vector<RunArgInfo> Args;
    std::array<size_t, 3> WgSize;
    std::array<size_t, 3> LcSize;
    RunConfig(std::u32string_view name, std::string kerName) noexcept :
        Name(name), KernelName(kerName), WgSize{ 0,0,0 }, LcSize{ 0,0,0 }
    { }
};
struct RunInfo
{
    std::deque<RunConfig> Configs; // in case add invalids reference
};

struct ArgWrapperHandler : public xziar::nailang::CustomVar::Handler
{
    static ArgWrapperHandler Handler;
    static xziar::nailang::CustomVar CreateBuffer(uint32_t size)
    {
        return { &Handler, size, size, common::enum_cast(RunArgInfo::ArgType::Buffer) };
    }
    static xziar::nailang::CustomVar CreateImage(uint32_t w, uint32_t h)
    {
        return { &Handler, w, h, common::enum_cast(RunArgInfo::ArgType::Image) };
    }
    static xziar::nailang::CustomVar CreateVal8(uint8_t val)
    {
        return { &Handler, val, val, common::enum_cast(RunArgInfo::ArgType::Val8) };
    }
    static xziar::nailang::CustomVar CreateVal16(uint16_t val)
    {
        return { &Handler, val, val, common::enum_cast(RunArgInfo::ArgType::Val16) };
    }
    static xziar::nailang::CustomVar CreateVal32(uint32_t val)
    {
        return { &Handler, val, val, common::enum_cast(RunArgInfo::ArgType::Val32) };
    }
    static xziar::nailang::CustomVar CreateVal64(uint64_t val)
    {
        return { &Handler, val >> 32, static_cast<uint32_t>(val & UINT32_MAX), common::enum_cast(RunArgInfo::ArgType::Val64) };
    }
};

struct XCStubHelper
{
private:
    struct RunConfigVar;
    struct XCStubExtension;
    std::unique_ptr<RunConfigVar> RunConfigHandler;
    virtual std::any TryFindKernel(xcomp::XCNLContext& context, std::string_view name) const = 0;
    virtual void FillArgs(std::vector<RunArgInfo>& dst, const std::any& cookie) const = 0;
    virtual std::optional<size_t> FindArgIdx(common::span<const RunArgInfo> args, std::string_view name) const = 0;
    virtual bool CheckType(const RunArgInfo& dst, RunArgInfo::ArgType type) const noexcept = 0;
    virtual std::string_view GetRealTypeName(const RunArgInfo& info) const noexcept = 0;
public:
    XCStubHelper();
    virtual ~XCStubHelper();
    std::unique_ptr<xcomp::XCNLExtension> GenerateExtension(xcomp::XCNLContext& context, RunInfo& info) const;
};
