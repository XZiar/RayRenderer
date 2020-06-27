#include "oclPch.h"
#include "oclKernelDebug.h"
#include "oclException.h"
#include "fmt/ranges.h"

namespace oclu
{
using namespace std::string_view_literals;
using common::simd::VecDataInfo;


std::pair<VecDataInfo, bool> ParseVDataType(const std::u32string_view type) noexcept
{
#define CASE(str, dtype, bit, n, least) \
    HashCase(type, str) return { {common::simd::VecDataInfo::DataTypes::dtype, bit, n, 0}, least };
#define CASEV(pfx, type, bit, least) \
    CASE(PPCAT(U, STRINGIZE(pfx)""),    type, bit, 1,  least) \
    CASE(PPCAT(U, STRINGIZE(pfx)"v2"),  type, bit, 2,  least) \
    CASE(PPCAT(U, STRINGIZE(pfx)"v3"),  type, bit, 3,  least) \
    CASE(PPCAT(U, STRINGIZE(pfx)"v4"),  type, bit, 4,  least) \
    CASE(PPCAT(U, STRINGIZE(pfx)"v8"),  type, bit, 8,  least) \
    CASE(PPCAT(U, STRINGIZE(pfx)"v16"), type, bit, 16, least) \

#define CASE2(tstr, type, bit)                  \
    CASEV(PPCAT(tstr, bit),  type, bit, false)  \
    CASEV(PPCAT(tstr, bit+), type, bit, true)   \

    switch (hash_(type))
    {
    CASE2(u, Unsigned, 8)
    CASE2(u, Unsigned, 16)
    CASE2(u, Unsigned, 32)
    CASE2(u, Unsigned, 64)
    CASE2(i, Signed,   8)
    CASE2(i, Signed,   16)
    CASE2(i, Signed,   32)
    CASE2(i, Signed,   64)
    CASE2(f, Float,    16)
    CASE2(f, Float,    32)
    CASE2(f, Float,    64)
    default: break;
    }
    return { {VecDataInfo::DataTypes::Unsigned, 0, 0, 0}, false };

#undef CASE2
#undef CASEV
#undef CASE
}

std::u32string_view StringifyVDataType(const VecDataInfo vtype) noexcept
{
#define CASE(s, type, bit, n) \
    case static_cast<uint32_t>(VecDataInfo{VecDataInfo::DataTypes::type, bit, n, 0}): return PPCAT(PPCAT(U,s),sv);
#define CASEV(pfx, type, bit) \
    CASE(STRINGIZE(pfx),             type, bit, 1)  \
    CASE(STRINGIZE(PPCAT(pfx, v2)),  type, bit, 2)  \
    CASE(STRINGIZE(PPCAT(pfx, v3)),  type, bit, 3)  \
    CASE(STRINGIZE(PPCAT(pfx, v4)),  type, bit, 4)  \
    CASE(STRINGIZE(PPCAT(pfx, v8)),  type, bit, 8)  \
    CASE(STRINGIZE(PPCAT(pfx, v16)), type, bit, 16) \

    switch (static_cast<uint32_t>(vtype))
    {
    CASEV(u8,  Unsigned, 8)
    CASEV(u16, Unsigned, 16)
    CASEV(u32, Unsigned, 32)
    CASEV(u64, Unsigned, 64)
    CASEV(i8,  Signed,   8)
    CASEV(i16, Signed,   16)
    CASEV(i32, Signed,   32)
    CASEV(i64, Signed,   64)
    CASEV(f16, Float,    16)
    CASEV(f32, Float,    32)
    CASEV(f64, Float,    64)
    default: return U"Error"sv;
    }

#undef CASEV
#undef CASE
}


DebugDataLayout::DebugDataLayout(common::span<const VecDataInfo> infos, const uint16_t align) :
    Blocks(std::make_unique<DataBlock[]>(infos.size())), ArgLayout(std::make_unique<uint16_t[]>(infos.size())),
    TotalSize(0), ArgCount(gsl::narrow_cast<uint32_t>(infos.size()))
{
    Expects(ArgCount <= UINT8_MAX);
    std::vector<DataBlock> tmp;
    uint16_t offset = 0, layoutidx = 0;
    { 
        uint16_t idx = 0;
        for (const auto info : infos)
        {
            Blocks[idx].Info   = info;
            Blocks[idx].ArgIdx = idx;
            const auto size = info.Bit * info.Dim0 / 8;
            if (size % align == 0)
            {
                Blocks[idx].Offset = offset;
                offset = static_cast<uint16_t>(offset + size);
                ArgLayout[layoutidx++] = idx;
            }
            else
            {
                Blocks[idx].Offset = UINT16_MAX;
                tmp.emplace_back(Blocks[idx]);
            }
            idx++;
        }
    }
    std::sort(tmp.begin(), tmp.end(), [](const auto& left, const auto& right)
        { 
            return left.Info.Bit == right.Info.Bit ? 
                left.Info.Bit * left.Info.Dim0 > right.Info.Bit * right.Info.Dim0 : 
                left.Info.Bit > right.Info.Bit; 
        });

    for (const auto [info, dummy, idx] : tmp)
    {
        Expects(dummy == UINT16_MAX);
        Blocks[idx].Offset = offset;
        const auto size = info.Bit * info.Dim0 / 8;
        offset = static_cast<uint16_t>(offset + size);
        ArgLayout[layoutidx++] = idx;
    }
    TotalSize = (offset + (align - 1)) / align * align;
}



oclDebugInfoMan::~oclDebugInfoMan()
{
}

uint32_t oclDebugInfoMan::GetInfoBufferSize(const size_t* worksize, const uint32_t dims) const noexcept
{
    size_t size = 1;
    for (uint32_t i = 0; i < dims; ++i)
        size *= worksize[i];
    Expects(size < 0x00ffffffu);
    return static_cast<uint32_t>(sizeof(uint32_t) * (size + 7));
}

oclThreadInfo NonSubgroupInfoMan::GetThreadInfo(common::span<const uint32_t> space, const uint32_t tid) const noexcept
{
    const auto& gsize = *reinterpret_cast<const uint32_t(*)[3]>(space.data() + 1);
    const auto& lsize = *reinterpret_cast<const uint32_t(*)[3]>(space.data() + 4);
    auto info = BasicInfo(gsize, lsize, tid);
    info.SubgroupId = info.SubgroupLocalId = 0;
    const auto infodata = space[tid + 7];
    const auto lid = info.LocalId[0] + info.LocalId[1] * lsize[0] + info.LocalId[2] * lsize[1] * lsize[0];
    Expects(infodata == lid);
    return info;
}

oclThreadInfo SubgroupInfoMan::GetThreadInfo(common::span<const uint32_t> space, const uint32_t tid) const noexcept
{
    const auto& gsize = *reinterpret_cast<const uint32_t(*)[3]>(space.data() + 1);
    const auto& lsize = *reinterpret_cast<const uint32_t(*)[3]>(space.data() + 4);
    auto info = BasicInfo(gsize, lsize, tid);
    const auto infodata = space[tid + 7];
    info.SubgroupId = static_cast<uint16_t>(infodata / 65536u);
    info.SubgroupLocalId = static_cast<uint16_t>(infodata % 65536u);
    return info;
}


template<typename T>
static void Insert(fmt::dynamic_format_arg_store<fmt::u32format_context>& store, common::span<const T> data)
{
    if (data.size() == 1)
    {
        if constexpr (std::is_same_v<T, half_float::half>)
            store.push_back(static_cast<float>(data[0]));
        else
            store.push_back(data[0]);
    }
    else
    {
        if constexpr (std::is_same_v<T, half_float::half>)
        {
            Expects(data.size() <= 16);
            std::array<float, 16> tmp;
            size_t idx = 0;
            for (const auto val : data)
                tmp[idx++] = val;
            const auto datspan = common::to_span(tmp).subspan(0, data.size());
            store.push_back(fmt::format(FMT_STRING("{}"sv), datspan));
        }
        else
        {
            store.push_back(fmt::format(FMT_STRING("{}"sv), data));
        }
    }
}
static void Insert(fmt::dynamic_format_arg_store<fmt::u32format_context>& store, std::nullopt_t)
{
    store.push_back(U"Unknown"sv);
}
common::str::u8string oclDebugBlock::GetString(common::span<const std::byte> data) const
{
    fmt::dynamic_format_arg_store<fmt::u32format_context> store;
    for (const auto& arg : Layout.ByIndex())
    {
        arg.VisitData(data, [&](auto ele) { Insert(store, ele); });
    }
    auto str = fmt::vformat(Formatter, store);
    return common::strchset::to_u8string(str, common::str::Charset::UTF32LE);
}


void oclDebugManager::CheckNewBlock(const std::u32string_view name) const
{
    const auto idx = Blocks.size();
    if (idx >= 250u)
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"Too many DebugString defined, maximum 250");
    for (const auto& block : Blocks)
        if (block.Name == name)
            COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"DebugString re-defiend");
}

std::pair<const oclDebugBlock*, uint32_t> oclDebugManager::RetriveMessage(common::span<const uint32_t> data) const
{
    const auto uid = data[0];
    const auto dbgIdx = uid >> 24;
    const auto tid = uid & 0x00ffffffu;
    if (dbgIdx == 0) return { nullptr, 0 };
    if (dbgIdx > Blocks.size())
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"Wrong message with idx overflow");
    const auto& block = Blocks[dbgIdx - 1];
    if (data.size_bytes() < block.Layout.TotalSize)
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"Wrong message with insufficiant buffer space");
    return { &block, tid };
}

}
