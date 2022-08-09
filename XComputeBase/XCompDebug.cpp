#include "XCompDebug.h"
#include "SystemCommon/StringFormat.h"
#include "SystemCommon/StringConvert.h"
#include <ctime>
#include <algorithm>



//template<typename Char, typename T>
//struct fmt::formatter<xcomp::debug::ArgsLayout::VecItem<T>, Char>
//{
//    using R = std::conditional_t<std::is_same_v<T, half_float::half>, float, T>;
//    std::basic_string_view<Char> Presentation;
//    template<typename ParseContext>
//    constexpr auto parse(ParseContext& ctx)
//    {
//        const auto begin = ctx.begin(), end = ctx.end();
//        auto it = begin;
//        size_t count = 0;
//        while (it != end && *it != static_cast<Char>('}'))
//            ++it, ++count;
//        if (it != end && *it != static_cast<Char>('}'))
//            throw format_error("invalid format");
//        Presentation = { &(*begin), count };
//        return it;
//    }
//    template<typename FormatContext>
//    auto format(const xcomp::debug::ArgsLayout::VecItem<T>& arg, FormatContext& ctx)
//    {
//        Expects(arg.Count > 0);
//        auto it = ctx.out();
//        if (arg.Count > 1)
//        {
//            *it++ = static_cast<Char>('{');
//            *it++ = static_cast<Char>(' ');
//        }
//        if (Presentation.empty())
//        {
//            for (uint32_t idx = 0; idx < arg.Count; ++idx)
//            {
//                if (idx > 0)
//                {
//                    *it++ = static_cast<Char>(',');
//                    *it++ = static_cast<Char>(' ');
//                }
//                if constexpr (std::is_same_v<Char, char>)
//                {
//                    it = fmt::format_to(it, FMT_STRING("{}"),   static_cast<R>(arg.Ptr[idx]));
//                }
//#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
//                else if constexpr (std::is_same_v<Char, char8_t>)
//                {
//                    it = fmt::format_to(it, FMT_STRING(u8"{}"), static_cast<R>(arg.Ptr[idx]));
//                }
//#endif
//                else if constexpr (std::is_same_v<Char, char16_t>)
//                {
//                    it = fmt::format_to(it, FMT_STRING(u"{}"),  static_cast<R>(arg.Ptr[idx]));
//                }
//                else if constexpr (std::is_same_v<Char, char32_t>)
//                {
//                    it = fmt::format_to(it, FMT_STRING(U"{}"),  static_cast<R>(arg.Ptr[idx]));
//                }
//                else if constexpr (std::is_same_v<Char, wchar_t>)
//                {
//                    it = fmt::format_to(it, FMT_STRING(L"{}"),  static_cast<R>(arg.Ptr[idx]));
//                }
//                else
//                {
//                    static_assert(!common::AlwaysTrue<T>, "Unsupportted Char type");
//                }
//            }
//        }
//        else
//        {
//            const auto baseSize = Presentation.size();
//            std::basic_string<Char> fmter; 
//            fmter.resize(baseSize + 3);
//            fmter[0]     = static_cast<Char>('{');
//            fmter[1]     = static_cast<Char>(':');
//            fmter.back() = static_cast<Char>('}');
//            const auto size = Presentation.size() * sizeof(Char);
//            memcpy_s(&fmter[2], size, Presentation.data(), size);
//            
//            for (uint32_t idx = 0; idx < arg.Count; ++idx)
//            {
//                if (idx > 0)
//                {
//                    *it++ = static_cast<Char>(',');
//                    *it++ = static_cast<Char>(' ');
//                }
//                it = fmt::format_to(it, fmter, static_cast<R>(arg.Ptr[idx]));
//            }
//        }
//        if (arg.Count > 1)
//        {
//            *it++ = static_cast<Char>(' ');
//            *it++ = static_cast<Char>('}');
//        }
//        return it;
//    }
//};


namespace xcomp::debug
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using common::simd::VecDataInfo;
using common::BaseException;
using common::str::Encoding;

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
        for (const auto& [str, info] : infos)
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

    for (const auto& [info, name, dummy, idx] : tmp)
    {
        Expects(dummy == UINT16_MAX);
        Args[idx].Offset = offset;
        const auto size = info.Bit * info.Dim0 / 8;
        offset = static_cast<uint16_t>(offset + size);
        ArgLayout[layoutidx++] = idx;
    }
    TotalSize = (offset + (align - 1)) / align * align;
}


MessageBlock::MessageBlock(const uint8_t idx, const std::u32string_view name, const std::u32string_view formatter,
    common::span<const NamedVecPair> infos) :
    Layout(infos, 4), Name(name), FormatCache([](auto fmtstr)
        {
            const auto fmtu8 = common::str::to_string(fmtstr, common::str::Encoding::UTF8);
            const auto result = common::str::ParseResult::ParseString<char>(fmtu8);
            return common::str::DynamicTrimedResultCh<char>{ result, fmtu8 };
        }(formatter)), DebugId(idx) 
{ }

struct MessageFormatExecutor final : public common::str::FormatterExecutor, public common::str::Formatter<char>
{
    using CTX = common::str::FormatterExecutor::Context;
    struct Context : public CTX
    {
        std::basic_string<char>& Dst;
        std::basic_string_view<char> FmtStr;
        const ArgsLayout& Layout;
        common::span<const std::byte> Data;
        constexpr Context(std::basic_string<char>& dst, std::string_view fmtstr, const ArgsLayout& layout, common::span<const std::byte> data) noexcept :
            Dst(dst), FmtStr(fmtstr), Layout(layout), Data(data)
        { }
    };

    void OnFmtStr(CTX& ctx, uint32_t offset, uint32_t length) final
    {
        auto& context = static_cast<Context&>(ctx);
        context.Dst.append(context.FmtStr.substr(offset, length));
    }
    void OnBrace(CTX& ctx, bool isLeft) final
    {
        auto& context = static_cast<Context&>(ctx);
        context.Dst.push_back(isLeft ? '{' : '}');
    }
    void OnColor(CTX& ctx, common::ScreenColor color) final
    {
        auto& context = static_cast<Context&>(ctx);
        PutColor(context.Dst, color);
    }
    void OnArg(CTX& ctx, uint8_t argIdx, bool isNamed, const common::str::FormatSpec* spec) final
    {
        Expects(!isNamed);
        auto& context = static_cast<Context&>(ctx);
        const auto& item = context.Layout[argIdx];
        const auto data = &context.Data[item.Offset];
        Expects(item.Info.Dim0 > 0);
        if (item.Info.Dim0 > 1)
        {
            context.Dst.append("{ "sv);
        }
        for (uint32_t i = 0; i < item.Info.Dim0; ++i)
        {
            if (i > 0)
                context.Dst.append(", "sv);
            switch (item.Info.Type)
            {
            case VecDataInfo::DataTypes::Float:
                switch (item.Info.Bit)
                {
                case 16: PutFloat(context.Dst, static_cast<float>(reinterpret_cast<const half_float::half*>(data)[i]), spec); break;
                case 32: PutFloat(context.Dst, reinterpret_cast<const float *>(data)[i], spec); break;
                case 64: PutFloat(context.Dst, reinterpret_cast<const double*>(data)[i], spec); break;
                default: break;
                }
                break;
            case VecDataInfo::DataTypes::Unsigned:
                switch (item.Info.Bit)
                {
                case  8: PutInteger(context.Dst, static_cast<uint32_t>(reinterpret_cast<const uint8_t *>(data)[i]), false, spec); break;
                case 16: PutInteger(context.Dst, static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(data)[i]), false, spec); break;
                case 32: PutInteger(context.Dst, reinterpret_cast<const uint32_t*>(data)[i], false, spec); break;
                case 64: PutInteger(context.Dst, reinterpret_cast<const uint64_t*>(data)[i], false, spec); break;
                default: break;
                }
                break;
            case VecDataInfo::DataTypes::Signed:
                switch (item.Info.Bit)
                {
                case  8: PutInteger(context.Dst, static_cast<uint32_t>(reinterpret_cast<const int8_t *>(data)[i]), true, spec); break;
                case 16: PutInteger(context.Dst, static_cast<uint32_t>(reinterpret_cast<const int16_t*>(data)[i]), true, spec); break;
                case 32: PutInteger(context.Dst, static_cast<uint32_t>(reinterpret_cast<const int32_t*>(data)[i]), true, spec); break;
                case 64: PutInteger(context.Dst, static_cast<uint64_t>(reinterpret_cast<const int64_t*>(data)[i]), true, spec); break;
                default: break;
                }
                break;
            default: break;
            }
        }
        if (item.Info.Dim0 > 1)
        {
            context.Dst.append(" }"sv);
        }
    }

    using FormatterBase::Execute;
};
static MessageFormatExecutor MsgFmtExecutor;



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
    const auto strInfo = FormatCache.ToStrArgInfo();
    common::str::u8string ret;
    MessageFormatExecutor::Context ctx { *reinterpret_cast<std::string*>(&ret), strInfo.FormatString, Layout, data };
    uint32_t opOffset = 0;
    while (opOffset < strInfo.Opcodes.size())
    {
        MessageFormatExecutor::Execute<common::str::FormatterExecutor>(strInfo.Opcodes, opOffset, MsgFmtExecutor, ctx);
    }
    return ret;
    /*fmt::dynamic_format_arg_store<fmt::u32format_context> store;
    for (const auto& arg : Layout.ByIndex())
    {
        arg.VisitData(data, [&](auto ele)
            {
                Insert(store, ele);
            });
    }
    auto str = fmt::vformat(Formatter, store);
    return common::str::to_u8string(str);*/
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


DebugPackage::DebugPackage(std::u32string_view exeName, std::shared_ptr<DebugManager> manager, std::shared_ptr<InfoProvider> infoProv,
    common::AlignedBuffer&& info, common::AlignedBuffer&& data) noexcept :
    ExecutionName(exeName), Manager(std::move(manager)), InfoProv(std::move(infoProv)),
    InfoBuffer(std::move(info)), DataBuffer(std::move(data))
{ }
DebugPackage::DebugPackage(const DebugPackage& package) noexcept :
    ExecutionName(package.ExecutionName), Manager(package.Manager), InfoProv(package.InfoProv),
    InfoBuffer(package.InfoBuffer.CreateSubBuffer()), DataBuffer(package.DataBuffer.CreateSubBuffer())
{ }
DebugPackage::DebugPackage(DebugPackage&& package) noexcept :
    ExecutionName(std::move(package.ExecutionName)), Manager(std::move(package.Manager)), InfoProv(std::move(package.InfoProv)),
    InfoBuffer(std::move(package.InfoBuffer)), DataBuffer(std::move(package.DataBuffer))
{ }
DebugPackage::~DebugPackage()
{ }

CachedDebugPackage DebugPackage::GetCachedData() const
{
    return { ExecutionName, Manager, InfoProv, InfoBuffer.CreateSubBuffer(), DataBuffer.CreateSubBuffer() };
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


CachedDebugPackage::CachedDebugPackage(std::u32string_view exeName, std::shared_ptr<DebugManager> manager, std::shared_ptr<InfoProvider> infoProv, 
    common::AlignedBuffer&& info, common::AlignedBuffer&& data) :
    DebugPackage(exeName, std::move(manager), std::move(infoProv), std::move(info), std::move(data))
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
CachedDebugPackage::CachedDebugPackage(CachedDebugPackage&& package) noexcept :
    DebugPackage(std::move(package)), MsgTexts(std::move(package.MsgTexts)),
    Items(std::move(package.Items)), Infos(std::move(package.Infos))
{ }
CachedDebugPackage::~CachedDebugPackage()
{ }


ExcelXmlPrinter::InfoCache::InfoCache(const InfoProvider& infoProv, common::span<const uint32_t> infoSpan) :
    InfoProv(infoProv), InfoSpan(infoSpan), Fields(infoProv.QueryFields())
{ 
    const auto exeInfo = InfoProv.GetExecuteInfo(InfoSpan);
    InfoIndexes.resize(exeInfo.ThreadCount);
}

std::string_view ExcelXmlPrinter::InfoCache::GetThreadInfo(const uint32_t tid)
{
    auto& idx = InfoIndexes[tid];
    if (idx.GetLength() == 0) // need to generate
        idx = GenerateThreadInfo(*InfoProv.GetThreadInfo(InfoSpan, tid));
    return InfoTexts.GetStringView(idx);
}
std::string_view ExcelXmlPrinter::InfoCache::GetThreadInfo(const WorkItemInfo& info)
{
    auto& idx = InfoIndexes[info.ThreadId];
    if (idx.GetLength() == 0) // need to generate
        idx = GenerateThreadInfo(info);
    return InfoTexts.GetStringView(idx);
}

static constexpr auto RowBegin      = R"(   <Row ss:AutoFitHeight="0">
)"sv;
static constexpr auto RowEnd        = R"(   </Row>
)"sv;
static constexpr auto StrCellBegin  = R"(    <Cell><Data ss:Type="String">)"sv;
static constexpr auto NumCellBegin  = R"(    <Cell><Data ss:Type="Number">)"sv;
static constexpr auto CellEnd       = R"(</Data></Cell>
)"sv;
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

uint32_t ExcelXmlPrinter::LocateInfo(const InfoCache& infoCache)
{
    uint32_t idx = 0;
    for (auto& info : InfoPacks)
    {
        if (info.Fields.data() == infoCache.Fields.data() && info.Fields.size() == infoCache.Fields.size())
            return idx;
        idx++;
    }

    // prepare info
    InfoPacks.push_back({});
    auto& info = InfoPacks.back();
    info.Fields = infoCache.Fields;
    for (const auto& field : infoCache.Fields)
    {
        if (field.VecType.Dim0 > 1)
        {
            Expects(field.VecType.Dim0 <= 4);
            info.Columns += field.VecType.Dim0;
            APPEND_FMT(info.Header1, R"(    <Cell ss:MergeAcross="{}" ss:StyleID="s63"><Data ss:Type="String">{}</Data></Cell>)"sv,
                field.VecType.Dim0 - 1, field.Name);
            info.Header1.append("\r\n");
            char parts[] = "xyzw";
            for (uint32_t i = 0; i < field.VecType.Dim0; ++i)
            {
                APPEND_FMT(info.Header2, R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">{}</Data></Cell>)"sv, parts[i]);
                info.Header2.append("\r\n");
            }
        }
        else
        {
            info.Columns += 1;
            const auto str = FMTSTR(R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">{}</Data></Cell>)"sv, field.Name);
            info.Header1.append(str).append("\r\n");
            info.Header2.append(str).append("\r\n");
        }
    }
    Expects(info.Columns > 1);
    info.Header0 = FMTSTR(R"(    <Cell ss:MergeAcross="{}" ss:StyleID="s63"><Data ss:Type="String">INFO</Data></Cell>)"sv, info.Columns - 1);
    info.Header0.append("\r\n");

    return idx;
}

uint32_t ExcelXmlPrinter::LocateBlock(const MessageBlock& block, const std::shared_ptr<DebugManager>& dbgMan)
{
    uint32_t idx = 0;
    for (auto& msg : MsgPacks)
    {
        if (msg.MsgBlock.get() == &block)
            return idx;
        idx++;
    }

    // prepare MsgHeader
    MsgPacks.push_back({});
    auto& msg = MsgPacks.back();
    msg.MsgBlock = { dbgMan, &block };
    AppendXmlStr(msg.BlockName, block.Name);
    uint32_t argIdx = 0;
    for (const auto& arg : block.Layout.ByIndex())
    {
        if (arg.Name.GetLength() == 0)
        {
            const auto str = FMTSTR(R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">ARG{}</Data></Cell>)"sv, argIdx++);
            msg.Header1.append(str).append("\r\n");
            msg.Header2.append(str).append("\r\n");
        }
        else
        {
            APPEND_FMT(msg.Header1, R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">ARG{}</Data></Cell>)"sv, argIdx++);
            msg.Header1.append("\r\n");
            std::string argName;
            AppendXmlStr(argName, block.Layout.GetName(arg));
            APPEND_FMT(msg.Header2, R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">{}</Data></Cell>)"sv, argName);
            msg.Header2.append("\r\n");
        }
    }
    constexpr auto TxtCol = R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">TXT</Data></Cell>)"sv;
    msg.Header1.append(TxtCol).append("\r\n");
    msg.Header2.append(TxtCol).append("\r\n");
    if (block.Layout.ArgCount > 0)
        msg.Header0 = FMTSTR(R"(    <Cell ss:MergeAcross="{}" ss:StyleID="s63"><Data ss:Type="String">MSG</Data></Cell>)"sv,
            block.Layout.ArgCount - 1);
    else
        msg.Header0 = R"(    <Cell ss:StyleID="s63"><Data ss:Type="String">MSG</Data></Cell>)"sv;
    msg.Header0.append("\r\n");
    msg.Columns = block.Layout.ArgCount + 1;

    return idx;
}

uint32_t ExcelXmlPrinter::LocateSheet(std::u32string_view exeName, const uint32_t infoIdx, const uint32_t msgIdx)
{
    uint32_t repeat = 0;
    uint32_t idx = 0;
    for (auto& sheet : Sheets)
    {
        if (sheet.ExeName == exeName && sheet.MsgPkgIdx == msgIdx)
        {
            if (sheet.InfoPkgIdx == infoIdx)
                return idx;
            repeat = std::max(repeat, sheet.RepeatId);
        }
        idx++;
    }

    // prepare sheet
    Sheets.push_back({});
    auto& sheet = Sheets.back();
    sheet.ExeName       = exeName;
    sheet.RepeatId      = repeat + 1;
    sheet.InfoPkgIdx    = infoIdx;
    sheet.MsgPkgIdx     = msgIdx;

    return idx;
}

std::vector<std::pair<ExcelXmlPrinter::SheetPackage*, const MessageBlock*>> ExcelXmlPrinter::PrepareLookup(
    std::u32string_view exeName, const InfoCache& infoCache, const std::shared_ptr<DebugManager>& dbgMan)
{
    std::vector<std::pair<SheetPackage*, const MessageBlock*>> sheetLookup;
    const auto  msgBlks = dbgMan->GetBlocks();
    sheetLookup.reserve(msgBlks.size());
    // prepare sheet
    {
        const auto  infoIdx = LocateInfo(infoCache);
        std::vector<uint32_t> sheetIdxes; sheetIdxes.reserve(msgBlks.size());
        for (const auto& msgBlk : msgBlks)
        {
            const auto msgIdx = LocateBlock(msgBlk, dbgMan);
            sheetIdxes.push_back(LocateSheet(exeName, infoIdx, msgIdx));
        }
        for (const auto idx : sheetIdxes)
        {
            sheetLookup.emplace_back(&Sheets[idx], MsgPacks[Sheets[idx].MsgPkgIdx].MsgBlock.get());
        }
    }
    return sheetLookup;
}


//template<typename T>
//static forceinline void Print(std::string& output, const ArgsLayout::VecItem<T>& data)
//{
//    const auto& cellb = data.Count > 1 ? StrCellBegin : NumCellBegin;
//    const auto& celle = CellEnd;
//    output.append(cellb);
//    output.append(fmt::to_string(data));
//    output.append(celle);
//}
//static forceinline void Print(std::string& output, std::nullopt_t)
//{
//    output.append(R"(    <Cell><Data ss:Type="String">ERROR</Data></Cell>)"sv);
//}
void ExcelXmlPrinter::AddItem(SheetPackage& sheet, std::string_view info, common::str::u8string_view msg, const common::span<const std::byte> dat)
{
    sheet.Contents.append(RowBegin);
    sheet.Contents.append(info);
    const auto& block = *MsgPacks[sheet.MsgPkgIdx].MsgBlock;
    const auto strInfo = block.FormatCache.ToStrArgInfo();
    MessageFormatExecutor::Context ctx{ sheet.Contents, strInfo.FormatString, block.Layout, dat };
    for (uint32_t i = 0; i < block.Layout.ArgCount; ++i)
    {
        const auto& arg = block.Layout[i];
        const auto& cellb = arg.Info.Dim0 > 1 ? StrCellBegin : NumCellBegin;
        const auto& celle = CellEnd;
        sheet.Contents.append(cellb);
        MsgFmtExecutor.OnArg(ctx, static_cast<uint8_t>(i), false, nullptr);
        sheet.Contents.append(celle);
    }
    /*for (const auto& arg : block.Layout.ByIndex())
    {
        arg.VisitData(dat, [&](const auto& vecItem) { Print(sheet.Contents, vecItem); });
    }*/
    sheet.Contents.append(StrCellBegin);
    AppendXmlStr(sheet.Contents, { reinterpret_cast<const char*>(msg.data()), msg.size() });
    sheet.Contents.append(CellEnd);
    sheet.Contents.append(RowEnd);
    sheet.Rows++;
}


ExcelXmlPrinter::ExcelXmlPrinter()
{ }
ExcelXmlPrinter::~ExcelXmlPrinter()
{ }

void ExcelXmlPrinter::PrintPackage(const DebugPackage& package)
{
    InfoCache  infoCache(package.InfoMan(), package.InfoSpan());
    const auto dbgMan       = package.GetDebugManager();
    const auto msgBlks      = dbgMan->GetBlocks();
    const auto sheetLookup  = PrepareLookup(U""sv, infoCache, dbgMan);

    package.VisitData([&](const uint32_t tid, const InfoProvider&, const MessageBlock& block,
        const common::span<const uint32_t>, const common::span<const uint32_t> dat)
        {
            const auto tinfo = infoCache.GetThreadInfo(tid);

            const auto [sheet, msgBlk] = sheetLookup[&block - msgBlks.data()];
            const auto datSpan = common::as_bytes(dat);
            const auto msg = msgBlk->GetString(datSpan);

            AddItem(*sheet, tinfo, msg, datSpan);
        });
}

void ExcelXmlPrinter::PrintPackage(const CachedDebugPackage& package)
{
    InfoCache  infoCache(package.InfoMan(), package.InfoSpan());
    const auto dbgMan       = package.GetDebugManager();
    const auto sheetLookup  = PrepareLookup(U""sv, infoCache, dbgMan);

    for (const auto item : package)
    {
        const auto tinfo = infoCache.GetThreadInfo(item.Info());
        const auto [sheet, msgBlk] = sheetLookup[item.BlockIdx()];
        const auto msg   = item.Str();
        const auto datSpan = common::as_bytes(item.GetDataSpan());
        AddItem(*sheet, tinfo, msg, datSpan);
    }
}

void ExcelXmlPrinter::Output(common::io::OutputStream& stream)
{
    PrintFileHeader(stream);
    for (const auto& sheet : Sheets)
    {
        WriteWorkSheet(stream, sheet);
    }
    PrintFileFooter(stream);
}


common::StringPiece<char> ExcelXmlPrinter::InfoCache::GenerateThreadInfo(const WorkItemInfo& info)
{
    using half_float::half;
    const auto space = InfoProv.GetFullInfoSpace(info);
    std::string txt;
    for (const auto& field : Fields)
    {
        const auto sp = space.subspan(field.Offset);
        for (uint32_t i = 0; i < field.VecType.Dim0; ++i)
        {
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

#define WriteTo(str) stream.Write(str.size(), str.data())
void ExcelXmlPrinter::PrintFileHeader(common::io::OutputStream& stream)
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
    WriteTo(Header1);
    {
        const auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        const auto tm = fmt::gmtime(t);
        //<Created>1970-01-01T00:00:00Z</Created>
        const auto tstr = fmt::format("{:%Y-%m-%dT%H:%M:%SZ}", tm);
        WriteTo(tstr);
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
    WriteTo(Header2);
}

void ExcelXmlPrinter::PrintFileFooter(common::io::OutputStream& stream)
{
    constexpr std::string_view Footer =
R"(</Workbook>
)"sv;
    WriteTo(Footer);
}

void ExcelXmlPrinter::WriteWorkSheet(common::io::OutputStream& stream, const SheetPackage& sheet)
{
    if (sheet.Rows == 0)
        return;
    
    const auto& info = InfoPacks[sheet.InfoPkgIdx];
    const auto& msg  =  MsgPacks[sheet. MsgPkgIdx];

    std::string wsName;
    AppendXmlStr(wsName, sheet.ExeName);
    wsName.append(" - "sv).append(msg.BlockName);
    const auto wsStr = FMTSTR(R"( <Worksheet ss:Name="{}">)", wsName);

    constexpr auto TableBegin = R"(
  <Table x:FullColumns="1" x:FullRows="1">
)"sv;

    static constexpr auto FilterFmt = R"(  </Table>
  <AutoFilter x:Range="R3C1:R{}C{}" xmlns="urn:schemas-microsoft-com:office:excel"></AutoFilter>
 </Worksheet>
)"sv;
    const auto filter = FMTSTR(FilterFmt, sheet.Rows + 3, info.Columns + msg.Columns);
    
    WriteTo(wsStr);
    WriteTo(TableBegin);

    WriteTo(RowBegin);
    WriteTo(info.Header0);
    WriteTo( msg.Header0);
    WriteTo(RowEnd);

    WriteTo(RowBegin);
    WriteTo(info.Header1);
    WriteTo( msg.Header1);
    WriteTo(RowEnd);

    WriteTo(RowBegin);
    WriteTo(info.Header2);
    WriteTo( msg.Header2);
    WriteTo(RowEnd);

    WriteTo(sheet.Contents);

    WriteTo(filter);
}

void ExcelXmlPrinter::AppendXmlStr(std::string& output, std::string_view str)
{
    while (!str.empty())
    {
        const auto strEnd = str.data() + str.size();
        // use ptr seems to be faster https://www.bfilipek.com/2018/07/string-view-perf-followup.html
        const auto ptr = std::find_first_of(str.data(), strEnd, XmlSpecial.data(), XmlSpecial.data() + XmlSpecial.size());
        if (ptr == strEnd)
        {
            output.append(str);
            break;
        }
        const auto ch = *ptr;
        const auto len = ptr - str.data();
        output.append(str, 0, len);
        output.append(XmlReplacer(ch));
        str.remove_prefix(len + 1);
    }
}

void ExcelXmlPrinter::AppendXmlStr(std::string& output, std::u32string_view str)
{
    return AppendXmlStr(output, common::str::to_string(str, Encoding::UTF8));
}



}
