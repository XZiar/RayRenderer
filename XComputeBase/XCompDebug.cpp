#include "XCompDebug.h"
#include "StringUtil/Format.h"
#include "StringUtil/Convert.h"
#include "common/linq2.hpp"
#include <ctime>


template<typename Char, typename T>
struct fmt::formatter<xcomp::debug::ArgsLayout::VecItem<T>, Char>
{
    using R = std::conditional_t<std::is_same_v<T, half_float::half>, float, T>;
    std::basic_string_view<Char> Presentation;
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        const auto begin = ctx.begin(), end = ctx.end();
        auto it = begin;
        size_t count = 0;
        while (it != end && *it != static_cast<Char>('}'))
            ++it, ++count;
        if (it != end && *it != static_cast<Char>('}'))
            throw format_error("invalid format");
        Presentation = { &(*begin), count };
        return it;
    }
    template<typename FormatContext>
    auto format(const xcomp::debug::ArgsLayout::VecItem<T>& arg, FormatContext& ctx)
    {
        Expects(arg.Count > 0);
        auto it = ctx.out();
        if (arg.Count > 1)
        {
            *it++ = static_cast<Char>('{');
            *it++ = static_cast<Char>(' ');
        }
        if (Presentation.empty())
        {
            for (uint32_t idx = 0; idx < arg.Count; ++idx)
            {
                if (idx > 0)
                {
                    *it++ = static_cast<Char>(',');
                    *it++ = static_cast<Char>(' ');
                }
                if constexpr (std::is_same_v<Char, char>)
                {
                    it = fmt::format_to(it, FMT_STRING("{}"),   static_cast<R>(arg.Ptr[idx]));
                }
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
                else if constexpr (std::is_same_v<Char, char8_t>)
                {
                    it = fmt::format_to(it, FMT_STRING(u8"{}"), static_cast<R>(arg.Ptr[idx]));
                }
#endif
                else if constexpr (std::is_same_v<Char, char16_t>)
                {
                    it = fmt::format_to(it, FMT_STRING(u"{}"),  static_cast<R>(arg.Ptr[idx]));
                }
                else if constexpr (std::is_same_v<Char, char32_t>)
                {
                    it = fmt::format_to(it, FMT_STRING(U"{}"),  static_cast<R>(arg.Ptr[idx]));
                }
                else if constexpr (std::is_same_v<Char, wchar_t>)
                {
                    it = fmt::format_to(it, FMT_STRING(L"{}"),  static_cast<R>(arg.Ptr[idx]));
                }
                else
                {
                    static_assert(!common::AlwaysTrue<T>, "Unsupportted Char type");
                }
            }
        }
        else
        {
            const auto baseSize = Presentation.size();
            std::basic_string<Char> fmter; 
            fmter.resize(baseSize + 3);
            fmter[0]     = static_cast<Char>('{');
            fmter[1]     = static_cast<Char>(':');
            fmter.back() = static_cast<Char>('}');
            const auto size = Presentation.size() * sizeof(Char);
            memcpy_s(&fmter[2], size, Presentation.data(), size);
            
            for (uint32_t idx = 0; idx < arg.Count; ++idx)
            {
                if (idx > 0)
                {
                    *it++ = static_cast<Char>(',');
                    *it++ = static_cast<Char>(' ');
                }
                it = fmt::format_to(it, fmter, static_cast<R>(arg.Ptr[idx]));
            }
        }
        if (arg.Count > 1)
        {
            *it++ = static_cast<Char>(' ');
            *it++ = static_cast<Char>('}');
        }
        return it;
    }
};


namespace xcomp::debug
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using common::simd::VecDataInfo;
using common::BaseException;
using common::str::Charset;

#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)


ArgsLayout::ArgsLayout(common::span<const NamedVecPair> infos, const uint16_t align) :
    Args(std::make_unique<ArgsLayout::ArgItem[]>(infos.size())), ArgLayout(std::make_unique<uint16_t[]>(infos.size())),
    TotalSize(0), ArgCount(gsl::narrow_cast<uint32_t>(infos.size()))
{
    Expects(ArgCount <= UINT8_MAX);
    std::vector<ArgItem> tmp;
    uint16_t offset = 0, layoutidx = 0;
    { 
        uint16_t idx = 0;
        for (const auto [str, info] : infos)
        {
            if (!str.empty())
            {
                Args[idx].Name = AllocateString(str);
            }
            Args[idx].Info   = info;
            Args[idx].ArgIdx = idx;
            const auto size = info.Bit * info.Dim0 / 8;
            if (size % align == 0)
            {
                Args[idx].Offset = offset;
                offset = static_cast<uint16_t>(offset + size);
                ArgLayout[layoutidx++] = idx;
            }
            else
            {
                Args[idx].Offset = UINT16_MAX;
                tmp.emplace_back(Args[idx]);
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

    for (const auto [info, name, dummy, idx] : tmp)
    {
        Expects(dummy == UINT16_MAX);
        Args[idx].Offset = offset;
        const auto size = info.Bit * info.Dim0 / 8;
        offset = static_cast<uint16_t>(offset + size);
        ArgLayout[layoutidx++] = idx;
    }
    TotalSize = (offset + (align - 1)) / align * align;
}


template<typename T>
static forceinline void Insert(fmt::dynamic_format_arg_store<fmt::u32format_context>& store, const ArgsLayout::VecItem<T>& data)
{
    store.push_back(data);
}
static forceinline void Insert(fmt::dynamic_format_arg_store<fmt::u32format_context>& store, std::nullopt_t)
{
    store.push_back(U"Unknown"sv);
}
common::str::u8string MessageBlock::GetString(common::span<const std::byte> data) const
{
    fmt::dynamic_format_arg_store<fmt::u32format_context> store;
    for (const auto& arg : Layout.ByIndex())
    {
        arg.VisitData(data, [&](auto ele)
            {
                Insert(store, ele);
            });
    }
    auto str = fmt::vformat(Formatter, store);
    return common::str::to_u8string(str, common::str::Charset::UTF32LE);
}


template<> XCOMPBASAPI common::span<const WorkItemInfo::InfoField> WGInfoHelper::Fields<WorkItemInfo>() noexcept
{
    static const auto FIELDS = XCOMP_WGINFO_REG(WorkItemInfo, ThreadId, GlobalId, GroupId, LocalId);
    return FIELDS;
}


InfoPack::InfoPack(const InfoProvider& provider, const uint32_t count) : Provider(provider)
{
    IndexData.assign(count, UINT32_MAX);
}
InfoPack::~InfoPack() {}

const WorkItemInfo& InfoPack::GetInfo(common::span<const uint32_t> space, const uint32_t tid)
{
    auto& idx = IndexData[tid];
    if (idx == UINT32_MAX)
    {
        idx = Generate(space, tid);
    }
    return GetInfo(idx);
}


InfoProvider::~InfoProvider()
{
}

uint32_t InfoProvider::GetInfoBufferSize(const size_t* worksize, const uint32_t dims) const noexcept
{
    size_t size = 1;
    for (uint32_t i = 0; i < dims; ++i)
        size *= worksize[i];
    Expects(size < 0x00ffffffu);
    return static_cast<uint32_t>(sizeof(uint32_t) * (size + 7));
}


void DebugManager::CheckNewBlock(const std::u32string_view name) const
{
    const auto idx = Blocks.size();
    if (idx >= 250u)
        COMMON_THROW(BaseException, u"Too many DebugString defined, maximum 250");
    for (const auto& block : Blocks)
        if (block.Name == name)
            COMMON_THROW(BaseException, u"DebugString re-defiend");
}

std::pair<const MessageBlock*, uint32_t> DebugManager::RetriveMessage(common::span<const uint32_t> data) const
{
    const auto uid = data[0];
    const auto dbgIdx = uid >> 24;
    const auto tid = uid & 0x00ffffffu;
    if (dbgIdx == 0) return { nullptr, 0 };
    if (dbgIdx > Blocks.size())
        COMMON_THROW(BaseException, u"Wrong message with idx overflow");
    const auto& block = Blocks[dbgIdx - 1];
    if (data.size_bytes() < block.Layout.TotalSize)
        COMMON_THROW(BaseException, u"Wrong message with insufficiant buffer space");
    return { &block, tid };
}


DebugPackage::~DebugPackage()
{ }

CachedDebugPackage DebugPackage::GetCachedData() const
{
    return { Manager, InfoBuffer.CreateSubBuffer(), DataBuffer.CreateSubBuffer() };
}


common::str::u8string_view CachedDebugPackage::MessageItemWrapper::Str()
{
    if (!Item.Str.GetLength())
    {
        const auto& block = Host.Manager->GetBlocks()[Item.BlockId];
        const auto data = Host.DataBuffer.AsSpan<uint32_t>().subspan(Item.DataOffset, Item.DataLength);
        const auto str = block.GetString(data);
        Item.Str = Host.AllocateString(str);
    }
    return Host.GetStringView(Item.Str);
}

const WorkItemInfo& CachedDebugPackage::MessageItemWrapper::Info()
{
    return Host.Infos->GetInfo(Host.InfoSpan(), Item.ThreadId);
}


CachedDebugPackage::CachedDebugPackage(std::shared_ptr<DebugManager> manager, common::AlignedBuffer&& info, common::AlignedBuffer&& data) :
    DebugPackage(manager, std::move(info), std::move(data))
{
    Infos = manager->GetInfoProvider().GetInfoPack(InfoSpan());
    const auto blocks  = Manager->GetBlocks();
    const auto alldata = DataBuffer.AsSpan<uint32_t>();
    VisitData([&](const uint32_t tid, const InfoProvider&, const MessageBlock& block,
        const common::span<const uint32_t>, const common::span<const uint32_t> dat) 
        {
            const auto offset = gsl::narrow_cast<uint32_t>(dat.data() - alldata.data());
            const auto datLen = gsl::narrow_cast<uint16_t>(dat.size());
            const auto blkId  = gsl::narrow_cast<uint16_t>(&block - blocks.data());
            Items.push_back({ {0, 0}, tid, offset, datLen, blkId });
        });
}
CachedDebugPackage::~CachedDebugPackage()
{ }


class ExcelXmlPrinter
{
    common::io::OutputStream& Stream;
    common::span<const NamedVecPair> InfoFields;
    void PrintFileHeader();
    void PrintFileFooter();
    void BeginWorkSheet(const MessageBlock& block);
};


void PrintExcelXml(common::io::OutputStream&, const DebugPackage&)
{

}
void PrintExcelXml(common::io::OutputStream&, CachedDebugPackage&)
{

}


void ExcelXmlPrinter::PrintFileHeader()
{
    constexpr std::string_view Header1 = 
R"(<?xml version="1.0" encoding="UTF-8"?>
<?mso-application progid="Excel.Sheet"?>
<Workbook xmlns="urn:schemas-microsoft-com:office:spreadsheet"
 xmlns:o="urn:schemas-microsoft-com:office:office"
 xmlns:x="urn:schemas-microsoft-com:office:excel"
 xmlns:ss="urn:schemas-microsoft-com:office:spreadsheet"
 xmlns:html="http://www.w3.org/TR/REC-html40">
 <DocumentProperties xmlns="urn:schemas-microsoft-com:office:office">
  <Author>XCompute</Author>
  <LastAuthor>XCompute</LastAuthor>
  <Created>)"sv;
    Stream.WriteMany(Header1.size(), sizeof(char), Header1.data());
    {
        const auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        const auto tm = fmt::gmtime(t);
        //<Created>1970-01-01T00:00:00Z</Created>
        const auto tstr = fmt::format("{:%Y-%m-%dT%H:%M:%SZ}", tm);
        Stream.WriteMany(tstr.size(), sizeof(char), tstr.data());
    }
    constexpr std::string_view Header2 =
R"(</Created>
  <Version>16.00</Version>
 </DocumentProperties>
 <OfficeDocumentSettings xmlns="urn:schemas-microsoft-com:office:office">
  <AllowPNG/>
 </OfficeDocumentSettings>
 <Styles>
  <Style ss:ID="Default" ss:Name="Normal">
   <Alignment ss:Vertical="Center"/>
  </Style>
  <Style ss:ID="s63">
   <Alignment ss:Horizontal="Center" ss:Vertical="Center"/>
  </Style>
 </Styles>
)"sv;
    Stream.WriteMany(Header2.size(), sizeof(char), Header2.data());
}
void ExcelXmlPrinter::PrintFileFooter()
{
    constexpr std::string_view Footer =
R"(</Workbook>
)"sv;
    Stream.WriteMany(Footer.size(), sizeof(char), Footer.data());
}
void ExcelXmlPrinter::BeginWorkSheet(const MessageBlock& block)
{
    const auto wsStr = common::str::to_string(FMTSTR(UR"( <Worksheet ss:Name="{}">)", block.Name), Charset::UTF8);
    Stream.WriteMany(wsStr.size(), sizeof(char), wsStr.data());
    constexpr auto TableFmt = UR"(
  <Table x:FullColumns="1" x:FullRows="1">
   <Row ss:AutoFitHeight="0">
)"sv;
    Stream.WriteMany(TableFmt.size(), sizeof(char), TableFmt.data());
    using ArgPtr = std::add_pointer_t<decltype(*block.Layout.ByIndex().begin())>;
    std::vector<ArgPtr> namedArgs;
    for (const auto& item : block.Layout.ByIndex())
        if (item.Name.GetLength() > 0)
            namedArgs.push_back(&item);
    const auto infoCol = InfoFields.size();
    const auto dataCol = namedArgs.size() + 1; // with final arg
}


}
