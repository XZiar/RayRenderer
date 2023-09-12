#include "XCompRely.h"
#include "common/StaticLookup.hpp"
#include "SystemCommon/ConsoleEx.h"
#include "SystemCommon/Format.h"
#include "SystemCommon/FormatExtra.h"
#include "SystemCommon/MiscIntrins.h"

#pragma message("Compiling miniBLAS with " STRINGIZE(COMMON_SIMD_INTRIN) )

#define FMTSTR2(syntax, ...) common::str::Formatter<char16_t>{}.FormatStatic(FmtString(syntax), __VA_ARGS__)


namespace xcomp
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using common::simd::VecDataInfo;


void PCI_BDF::FormatWith(common::str::FormatterExecutor& executor, common::str::FormatterExecutor::Context& context, const common::str::FormatSpec*) const
{
    using namespace common::str;
    static const auto specCache = []() 
    {
        std::array<OpaqueFormatSpec, 2> specs = {};
        FormatterExecutor::ConvertSpec(specs[0], U"02X"sv, ArgRealType::UInt);
        FormatterExecutor::ConvertSpec(specs[1], U"1X"sv, ArgRealType::UInt);
        return specs;
    }();
    executor.PutInteger(context, Bus(), false, specCache[0]);
    executor.PutString(context, ":"sv, nullptr);
    executor.PutInteger(context, Device(), false, specCache[0]);
    executor.PutString(context, "."sv, nullptr);
    executor.PutInteger(context, Function(), false, specCache[1]);
}
std::string PCI_BDF::ToString() const noexcept
{
    return common::str::Formatter<char>{}.DirectFormat(*this);
}


#define GENV(dtype, bit, n, least) VTypeInfo{ common::simd::VecDataInfo::DataTypes::dtype, n, 0, bit, least ? VTypeInfo::TypeFlags::MinBits : VTypeInfo::TypeFlags::Empty }
#define PERPFX(pfx, type, bit, least) \
    { STRINGIZE(pfx) "",    GENV(type, bit, 1,  least) }, \
    { STRINGIZE(pfx) "v2",  GENV(type, bit, 2,  least) }, \
    { STRINGIZE(pfx) "v3",  GENV(type, bit, 3,  least) }, \
    { STRINGIZE(pfx) "v4",  GENV(type, bit, 4,  least) }, \
    { STRINGIZE(pfx) "v8",  GENV(type, bit, 8,  least) }, \
    { STRINGIZE(pfx) "v16", GENV(type, bit, 16, least) }

#define MIN2(tstr, type, bit)                    \
    PERPFX(PPCAT(tstr, bit),  type, bit, false), \
    PERPFX(PPCAT(tstr, bit+), type, bit, true)

static constexpr auto S2TMapping = BuildStaticLookup(common::str::ShortStrVal<8>, VTypeInfo,
    MIN2(u, Unsigned, 8),
    MIN2(u, Unsigned, 16),
    MIN2(u, Unsigned, 32),
    MIN2(u, Unsigned, 64),
    MIN2(i, Signed,   8),
    MIN2(i, Signed,   16),
    MIN2(i, Signed,   32),
    MIN2(i, Signed,   64),
    MIN2(f, Float,    16),
    MIN2(f, Float,    32),
    MIN2(f, Float,    64));

#undef MIN2
#undef PERPFX
#undef GENV

VTypeInfo ParseVDataType(const std::u32string_view type) noexcept
{
    const auto ret = S2TMapping(type);
    if (ret.has_value())
        return ret.value();
    return {};
}


#define GENV(type, flag, bit, n) static_cast<uint32_t>(VTypeInfo{ VecDataInfo::DataTypes::type, n, 0, bit, VTypeInfo::TypeFlags::flag })
#define MIN2(s, type, bit, n) \
    { GENV(type, Empty,   bit, n), U"" STRINGIZE(s) ""sv }, { GENV(type, MinBits, bit, n), U"" STRINGIZE(s) "+"sv }
#define PERPFX(pfx, type, bit) \
    MIN2(pfx,             type, bit, 1), \
    MIN2(PPCAT(pfx, v2),  type, bit, 2), \
    MIN2(PPCAT(pfx, v3),  type, bit, 3), \
    MIN2(PPCAT(pfx, v4),  type, bit, 4), \
    MIN2(PPCAT(pfx, v8),  type, bit, 8), \
    MIN2(PPCAT(pfx, v16), type, bit, 16)

static constexpr auto T2SMapping = BuildStaticLookup(uint32_t, std::u32string_view,
    PERPFX(u8,  Unsigned, 8),
    PERPFX(u16, Unsigned, 16),
    PERPFX(u32, Unsigned, 32),
    PERPFX(u64, Unsigned, 64),
    PERPFX(i8,  Signed,   8),
    PERPFX(i16, Signed,   16),
    PERPFX(i32, Signed,   32),
    PERPFX(i64, Signed,   64),
    PERPFX(f16, Float,    16),
    PERPFX(f32, Float,    32),
    PERPFX(f64, Float,    64));

#undef PERPFX
#undef MIN2
#undef GENV
std::u32string_view StringifyVDataType(const VTypeInfo vtype) noexcept
{
    if (vtype.IsCustomType())
        return U"Custom"sv;
    return T2SMapping(vtype).value_or(U"Error"sv);
}


CommonDeviceInfo::CommonDeviceInfo() noexcept :
    IsSoftware(false), SupportCompute(true), SupportRender(false), SupportDisplay(false), SupportPV(false)
{}
std::u16string_view CommonDeviceInfo::GetICDPath(ICDTypes) const noexcept { return {}; }
std::u16string_view CommonDeviceInfo::GetDevicePath() const noexcept { return {}; }
uint32_t CommonDeviceInfo::GetDisplay() const noexcept { return UINT32_MAX; }
bool CommonDeviceInfo::CheckDisplay(uint32_t) const noexcept { return false; }

#if !COMMON_OS_WIN && !COMMON_OS_LINUX
struct CommonDeviceContainerImpl final : public CommonDeviceContainer
{
    const CommonDeviceInfo* LocateByDevicePath(std::u16string_view devPath) const noexcept final { return nullptr; }
    const CommonDeviceInfo& Get(size_t index) const noexcept final 
    {
        std::abort();
        CM_UNREACHABLE();
    }
    size_t GetSize() const noexcept final { return 0; }
};
const CommonDeviceContainer& ProbeDevice()
{
    static CommonDeviceContainerImpl container;
    return container;
}
#endif

const CommonDeviceInfo* CommonDeviceContainer::LocateExactDevice(const std::array<std::byte, 8>* luid,
    const std::array<std::byte, 16>* guid, const PCI_BDF* pcie, std::u16string_view devPath) const noexcept
{
    const auto& console = common::console::ConsoleEx::Get();
    const CommonDeviceInfo* ret = nullptr;
    if (luid || guid || pcie)
    {
        const auto size = GetSize();
        for (size_t i = 0; i < size; ++i)
        {
            const auto& dev = Get(i);
            if (pcie && *pcie == dev.PCIEAddress)
            {
                ret = &dev;
                break;
            }
            if (luid && *luid == dev.Luid)
            {
                ret = &dev;
                break;
            }
            if (guid && *guid == dev.Guid)
            {
                ret = &dev;
                break;
            }
        }
    }
    if (!ret)
        ret = LocateByDevicePath(devPath);
    if (ret)
    {
        if (pcie && *pcie != ret->PCIEAddress)
            console.Print(common::CommonColor::BrightYellow,
                FMTSTR2(u"Found pcie-addr mismatch for device PCIE({}) to [{}]({})\n"sv,
                    *pcie, ret->Name, ret->PCIEAddress));
        if (luid && *luid != ret->Luid)
            console.Print(common::CommonColor::BrightYellow,
                FMTSTR2(u"Found luid mismatch for device LUID({}) to [{}]({})\n"sv,
                    common::MiscIntrin.HexToStr(*luid), ret->Name, common::MiscIntrin.HexToStr(ret->Luid)));
        if (guid && *guid != ret->Guid)
            console.Print(common::CommonColor::BrightYellow,
                FMTSTR2(u"Found guid mismatch for device GUID({}) to [{}]({})\n"sv,
                    common::MiscIntrin.HexToStr(*guid), ret->Name, common::MiscIntrin.HexToStr(ret->Guid)));
        return ret;
    }
    return nullptr;
}
const CommonDeviceInfo* CommonDeviceContainer::TryLocateDevice(const uint32_t* vid, const uint32_t* did, std::u16string_view name) const noexcept
{
    const auto& console = common::console::ConsoleEx::Get();
    if (vid || did || name.empty())
    {
        const CommonDeviceInfo* ret = nullptr;
        const auto size = GetSize();
        for (size_t i = 0; i < size; ++i)
        {
            const auto& dev = Get(i);
            if ((!vid || *vid == dev.VendorId) && (!did || *did == dev.DeviceId) && (!name.empty() || name == dev.Name))
            {
                if (!ret)
                    ret = &dev;
                else // multiple devices have same info, give up
                {
                    console.Print(common::CommonColor::Magenta,
                        FMTSTR2(u"Found multiple match for [{}](VID {:#010x},DID {:#010x}): [{}](VID {:#010x},DID {:#010x}), [{}](VID {:#010x},DID {:#010x})\n"sv,
                            name, vid ? *vid : 0, did ? *did : 0,
                            ret->Name, ret->VendorId, ret->DeviceId,
                            dev.Name, dev.VendorId, dev.DeviceId));
                    ret = nullptr;
                    break;
                }
            }
        }
        if (ret)
            return ret;
    }
    return nullptr;
}


struct RangeHolder::Range
{
    std::shared_ptr<const RangeHolder> Host;
    std::shared_ptr<Range> Previous;
    Range(std::shared_ptr<const RangeHolder> host) noexcept :
        Host(std::move(host)), Previous(Host->CurrentRange.lock())
    { }
    ~Range()
    {
        Host->EndRange();
    }
};

RangeHolder::~RangeHolder() {}
std::shared_ptr<void> RangeHolder::DeclareRange(std::u16string_view msg)
{
    auto range = std::make_shared<Range>(BeginRange(msg));
    CurrentRange = range;
    return range;
}


}