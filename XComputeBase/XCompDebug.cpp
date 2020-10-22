#include "XCompDebug.h"
#include "StringUtil/Format.h"
#include "StringUtil/Convert.h"
#include <ctime>
#include <algorithm>



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
                Args[idx].Name = ArgNames.AllocateString(str);
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
    return common::str::to_u8string(str);
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


InfoProvider::ExecuteInfo InfoProvider::GetBasicExeInfo(common::span<const uint32_t> space) noexcept
{
    const auto& gsize = *reinterpret_cast<const uint32_t(*)[3]>(space.data() + 1);
    const auto count = gsize[0] * gsize[1] * gsize[2];
    return { {gsize[0], gsize[1], gsize[2]}, 0, count };
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
    return { Manager, InfoProv, InfoBuffer.CreateSubBuffer(), DataBuffer.CreateSubBuffer() };
}


common::str::u8string_view CachedDebugPackage::MessageItemWrapper::Str() const noexcept
{
    if (!Item.Str.GetLength())
    {
        const auto str = Block().GetString(common::as_bytes(GetDataSpan()));
        Item.Str = Host.MsgTexts.AllocateString(str);
    }
    return Host.MsgTexts.GetStringView(Item.Str);
}

const WorkItemInfo& CachedDebugPackage::MessageItemWrapper::Info() const noexcept
{
    return Host.Infos->GetInfo(Host.InfoSpan(), Item.ThreadId);
}

common::span<const uint32_t> CachedDebugPackage::MessageItemWrapper::GetDataSpan() const noexcept
{
    return Host.DataBuffer.AsSpan<uint32_t>().subspan(Item.DataOffset, Item.DataLength);
}

const MessageBlock& CachedDebugPackage::MessageItemWrapper::Block() const noexcept
{
    return Host.Manager->GetBlocks()[Item.BlockId];
}


CachedDebugPackage::CachedDebugPackage(std::shared_ptr<DebugManager> manager, std::shared_ptr<InfoProvider> infoProv, common::AlignedBuffer&& info, common::AlignedBuffer&& data) :
    DebugPackage(std::move(manager), std::move(infoProv), std::move(info), std::move(data))
{
    Infos = InfoMan().GetInfoPack(InfoSpan());
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


class PackagePrinter
{
private:
    common::io::OutputStream& Stream;
    const InfoProvider& InfoProv;
    //const DebugPackage& BasePackage;

    common::span<const uint32_t> InfoSpan;
    common::span<const WorkItemInfo::InfoField> InfoFields;
    std::string InfoHeader0, InfoHeader1, InfoHeader2;
    uint32_t InfoColumns = 0;
    common::StringPool<char> InfoTexts;
    std::vector<common::StringPiece<char>> InfoIndexes;

    void PrepareInfo();
    common::StringPiece<char> GenerateThreadInfo(const WorkItemInfo& info);
    forceinline void Write(const std::string_view str) 
    {
        Stream.Write(str.size(), str.data());
    }
    // perform char replacement
    void WriteStr(const std::string_view str);

public:
    static constexpr auto RowBegin      = R"(   <Row ss:AutoFitHeight="0">
)"sv;
    static constexpr auto RowEnd        = R"(   </Row>
)"sv;
    static constexpr auto StrCellBegin  = R"(    <Cell><Data ss:Type="String">)"sv;
    static constexpr auto NumCellBegin  = R"(    <Cell><Data ss:Type="Number">)"sv;
    static constexpr auto CellEnd       = R"(</Data></Cell>
)"sv;

    PackagePrinter(common::io::OutputStream& stream, const InfoProvider& infoProv, common::span<const uint32_t> infoSpan) : 
        Stream(stream), InfoProv(infoProv), InfoSpan(infoSpan)
    {
        PrepareInfo();
    }
    ~PackagePrinter() { }
    static std::string ProcSpecialChar(std::u32string_view source);
    
    uint32_t BeginWorkSheet(const MessageBlock& block);
    void EndWorkSheet(const uint32_t row, const uint32_t col);
    void BeginRow();
    void EndRow();
    void PrintThreadInfo(const uint32_t tid);
    void PrintThreadInfo(const WorkItemInfo& info);
    void PrintMessage(const MessageBlock& block, const common::str::u8string_view str, common::span<const uint32_t> data);
};


ExcelXmlPrinter::ExcelXmlPrinter(common::io::OutputStream& stream) : Stream(stream)
{
    PrintFileHeader();
}
ExcelXmlPrinter::~ExcelXmlPrinter()
{
    PrintFileFooter();
}

void ExcelXmlPrinter::PrintPackage(const DebugPackage& package)
{
    PackagePrinter printer(Stream, package.InfoMan(), package.InfoSpan());
    const auto& dbgMan = package.DebugMan();
    for (const auto& msgBlk : dbgMan.GetBlocks())
    {
        const auto cols = printer.BeginWorkSheet(msgBlk);
        uint32_t rows = 0;
        package.VisitData([&](const uint32_t tid, const InfoProvider&, const MessageBlock& block,
            const common::span<const uint32_t>, const common::span<const uint32_t> dat)
            {
                if (&block != &msgBlk) return;
                printer.BeginRow();
                printer.PrintThreadInfo(tid);
                const auto str = msgBlk.GetString(common::as_bytes(dat));
                printer.PrintMessage(msgBlk, str, dat);
                printer.EndRow();
                rows++;
            });
        printer.EndWorkSheet(rows, cols);
    }
}
void ExcelXmlPrinter::PrintPackage(CachedDebugPackage& package)
{
    PackagePrinter printer(Stream, package.InfoMan(), package.InfoSpan());
    const auto& dbgMan = package.DebugMan();
    for (const auto& msgBlk : dbgMan.GetBlocks())
    {
        const auto cols = printer.BeginWorkSheet(msgBlk);
        uint32_t rows = 0;
        for (const auto item : package)
        {
            if (&item.Block() != &msgBlk) continue;
            printer.BeginRow();
            printer.PrintThreadInfo(item.Info());
            printer.PrintMessage(msgBlk, item.Str(), item.GetDataSpan());
            printer.EndRow();
            rows++;
        }
        printer.EndWorkSheet(rows, cols);
    }
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
    Stream.Write(Header1.size(), Header1.data());
    {
        const auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        const auto tm = fmt::gmtime(t);
        //<Created>1970-01-01T00:00:00Z</Created>
        const auto tstr = fmt::format("{:%Y-%m-%dT%H:%M:%SZ}", tm);
        Stream.Write(tstr.size(), tstr.data());
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
    Stream.Write(Header2.size(), Header2.data());
}

void ExcelXmlPrinter::PrintFileFooter()
{
    constexpr std::string_view Footer =
R"(</Workbook>
)"sv;
    Stream.Write(Footer.size(), Footer.data());
}


void PackagePrinter::PrepareInfo()
{
    const auto exeInfo = InfoProv.GetExecuteInfo(InfoSpan);
    InfoIndexes.resize(exeInfo.ThreadCount);

    InfoFields = InfoProv.QueryFields();
    InfoColumns = 0;
    for (const auto& field : InfoFields)
    {
        if (field.VecType.Dim0 > 1)
        {
            Expects(field.VecType.Dim0 <= 4);
            InfoColumns += field.VecType.Dim0;
            APPEND_FMT(InfoHeader1, R"(    <Cell ss:MergeAcross="{}" ss:StyleID="s63"><Data ss:Type="String">{}</Data></Cell>)"sv,
                field.VecType.Dim0 - 1, field.Name);
            InfoHeader1.append("\r\n");
            char parts[] = "xyzw";
            for (uint32_t i = 0; i < field.VecType.Dim0; ++i)
            {
                APPEND_FMT(InfoHeader2, R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">{}</Data></Cell>)"sv, parts[i]);
                InfoHeader2.append("\r\n");
            }
        }
        else
        {
            InfoColumns += 1;
            const auto str = FMTSTR(R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">{}</Data></Cell>)"sv, field.Name);
            InfoHeader1.append(str).append("\r\n");
            InfoHeader2.append(str).append("\r\n");
        }
    }
    Expects(InfoColumns > 1);
    InfoHeader0 = FMTSTR(R"(    <Cell ss:MergeAcross="{}" ss:StyleID="s63"><Data ss:Type="String">INFO</Data></Cell>)"sv, InfoColumns - 1);
    InfoHeader0.append("\r\n");
}


static constexpr auto XmlSpecial  = R"(&<>'")"sv;
static constexpr auto XmlReplacer = [](const char ch)
{
    switch (ch)
    {
    case '&':   return "&amp;"sv;
    case '<':   return "&lt;"sv;
    case '>':   return "&gt;"sv;
    case '"':   return "&quot;"sv;
    case '\'':  return "&apos;"sv;
    default:    return ""sv;
    }
};
void PackagePrinter::WriteStr(std::string_view str)
{
    while (!str.empty())
    {
        const auto strEnd = str.data() + str.size();
        // use ptr seems to be faster https://www.bfilipek.com/2018/07/string-view-perf-followup.html
        const auto ptr = std::find_first_of(str.data(), strEnd, XmlSpecial.data(), XmlSpecial.data() + XmlSpecial.size());
        if (ptr == strEnd)
        {
            Write(str); break;
        }
        const auto ch = *ptr;
        const auto len = ptr - str.data();
        Write(str.substr(0, len));
        Write(XmlReplacer(ch));
        str.remove_prefix(len + 1);
    }
}

std::string PackagePrinter::ProcSpecialChar(std::u32string_view source)
{
    const auto str = common::str::to_string(source, Charset::UTF8);
    const auto strLen = str.size();
    const auto strEnd = str.data() + strLen;
    if (str.empty()) return {};
    size_t idx = 0;
    std::string dst;
    {
        const auto ptr = std::find_first_of(str.data(), strEnd, XmlSpecial.data(), XmlSpecial.data() + XmlSpecial.size());
        if (ptr == strEnd)
            return str;
        dst.reserve(strLen + 20);
        dst.assign(str.data(), ptr);
        dst.append(XmlReplacer(*ptr));
        idx = ptr - str.data() + 1;
    }
    while (idx < strLen)
    {
        const auto ptr = std::find_first_of(&str[idx], strEnd, XmlSpecial.data(), XmlSpecial.data() + XmlSpecial.size());
        dst.append(&str[idx], strLen - idx);
        if (ptr == strEnd)
            break;
        dst.append(XmlReplacer(*ptr));
        idx = ptr - str.data() + 1;
    }
    return dst;
}


uint32_t PackagePrinter::BeginWorkSheet(const MessageBlock& block)
{
    const auto wsName = ProcSpecialChar(block.Name);
    const auto wsStr = FMTSTR(R"( <Worksheet ss:Name="{}">)", wsName);
    Write(wsStr);
    constexpr auto TableBegin = R"(
  <Table x:FullColumns="1" x:FullRows="1">
)"sv;
    Write(TableBegin);
    Write(  RowBegin);

    // prepare MsgHeader
    std::string msgHeader0, msgHeader1, msgHeader2;
    {
        constexpr auto TxtCol = R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">TXT</Data></Cell>)"sv;
        msgHeader1.append(TxtCol).append("\r\n"); 
        msgHeader2.append(TxtCol).append("\r\n");
        uint32_t idx = 0;
        for (const auto& arg : block.Layout.ByIndex())
        {
            if (arg.Name.GetLength() == 0)
            {
                const auto str = FMTSTR(R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">ARG{}</Data></Cell>)"sv, idx++);
                InfoHeader1.append(str).append("\r\n");
                InfoHeader2.append(str).append("\r\n");
            }
            else
            {
                APPEND_FMT(InfoHeader1, R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">ARG{}</Data></Cell>)"sv, idx++);
                InfoHeader1.append("\r\n");
                const auto argName = ProcSpecialChar(block.Layout.GetName(arg));
                APPEND_FMT(InfoHeader2, R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">{}</Data></Cell>)"sv, argName);
                InfoHeader2.append("\r\n");
            }
        }
        if (block.Layout.ArgCount > 0)
            msgHeader0 = FMTSTR(R"(    <Cell ss:MergeAcross="{}" ss:StyleID="s63"><Data ss:Type="String">MSG</Data></Cell>)"sv, 
                block.Layout.ArgCount - 1);
        else
            msgHeader0 = R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">MSG</Data></Cell>)"sv;
        msgHeader0.append("\r\n");
    }
    
    Write(InfoHeader0);
    Write( msgHeader0);
    Write(     RowEnd);
    Write(   RowBegin);
    Write(InfoHeader1);
    Write( msgHeader1);
    Write(     RowEnd);
    Write(   RowBegin);
    Write(InfoHeader2);
    Write( msgHeader2);
    Write(     RowEnd);

    return InfoColumns + block.Layout.ArgCount + 1;
}

forceinline void PackagePrinter::EndWorkSheet(const uint32_t row, const uint32_t col)
{
    static constexpr auto FilterFmt = R"(  </Table>
  <AutoFilter x:Range="R3C1:R{}C{}" xmlns="urn:schemas-microsoft-com:office:excel"></AutoFilter>
 </Worksheet>
)"sv;
    const auto filter = FMTSTR(FilterFmt, row + 3, col);
    Write(filter);
}

forceinline void PackagePrinter::BeginRow()
{
    Write(RowBegin);
}

forceinline void PackagePrinter::EndRow()
{
    Write(RowEnd);
}

common::StringPiece<char> PackagePrinter::GenerateThreadInfo(const WorkItemInfo& info)
{
    using half_float::half;
    const auto space = InfoProv.GetFullInfoSpace(info);
    std::string txt;
    for (const auto& field : InfoFields)
    {
        const auto sp = space.subspan(field.Offset);
        for (uint32_t i = 0; i < field.VecType.Dim0; ++i)
        {
            fmt::to_string(13);
            txt.append(NumCellBegin);
            switch (field.VecType.Type)
            {
            case VecDataInfo::DataTypes::Float:
                switch (field.VecType.Bit)
                {
                case 16: txt += fmt::to_string(static_cast<float>(reinterpret_cast<const   half*>(sp.data())[i])); break;
                case 32: txt += fmt::to_string(                   reinterpret_cast<const  float*>(sp.data())[i]);  break;
                case 64: txt += fmt::to_string(                   reinterpret_cast<const double*>(sp.data())[i]);  break;
                default: break;
                } break;
            case VecDataInfo::DataTypes::Unsigned:
                switch (field.VecType.Bit)
                {
                case  8: txt += fmt::to_string(reinterpret_cast<const  uint8_t*>(sp.data())[i]); break;
                case 16: txt += fmt::to_string(reinterpret_cast<const uint16_t*>(sp.data())[i]); break;
                case 32: txt += fmt::to_string(reinterpret_cast<const uint32_t*>(sp.data())[i]); break;
                case 64: txt += fmt::to_string(reinterpret_cast<const uint64_t*>(sp.data())[i]); break;
                default: break;
                } break;
            case VecDataInfo::DataTypes::Signed:
                switch (field.VecType.Bit)
                {
                case  8: txt += fmt::to_string(reinterpret_cast<const  int8_t*>(sp.data())[i]); break;
                case 16: txt += fmt::to_string(reinterpret_cast<const int16_t*>(sp.data())[i]); break;
                case 32: txt += fmt::to_string(reinterpret_cast<const int32_t*>(sp.data())[i]); break;
                case 64: txt += fmt::to_string(reinterpret_cast<const int64_t*>(sp.data())[i]); break;
                default: break;
                } break;
            default: break;
            }
            txt.append(CellEnd);
        }
    }
    return InfoTexts.AllocateString(txt);
}
void PackagePrinter::PrintThreadInfo(const uint32_t tid)
{
    auto& idx = InfoIndexes[tid];
    if (idx.GetLength() == 0) // need to generate
        idx = GenerateThreadInfo(*InfoProv.GetThreadInfo(InfoSpan, tid));
    const auto txt = InfoTexts.GetStringView(idx);
    Write(txt);
}
void PackagePrinter::PrintThreadInfo(const WorkItemInfo& info)
{
    auto& idx = InfoIndexes[info.ThreadId];
    if (idx.GetLength() == 0) // need to generate
        idx = GenerateThreadInfo(info);
    const auto txt = InfoTexts.GetStringView(idx);
    Write(txt);
}

template<typename T>
static forceinline void Print(common::io::OutputStream& stream, const ArgsLayout::VecItem<T>& data)
{
    const auto& cellb = data.Count > 1 ? PackagePrinter::StrCellBegin : PackagePrinter::NumCellBegin;
    const auto& celle = PackagePrinter::CellEnd;
    stream.WriteMany(  cellb.size(), sizeof(char),   cellb.data());
    const auto content = fmt::to_string(data);
    stream.WriteMany(content.size(), sizeof(char), content.data());
    stream.WriteMany(  celle.size(), sizeof(char),   celle.data());
}
static forceinline void Print(common::io::OutputStream& stream, std::nullopt_t)
{
    constexpr auto str = R"(    <Cell><Data ss:Type="String">ERROR</Data></Cell>)"sv;
    stream.WriteMany(str.size(), sizeof(u8ch_t), str.data());
}
void PackagePrinter::PrintMessage(const MessageBlock& block, const common::str::u8string_view str, common::span<const uint32_t> data)
{
    for (const auto& arg : block.Layout.ByIndex())
    {
        arg.VisitData(common::as_bytes(data), 
            [&](const auto& vecItem) { Print(Stream, vecItem); });
    }
    Write(StrCellBegin);
    WriteStr({ reinterpret_cast<const char*>(str.data()), str.size() });
    Write(CellEnd);
}



}
