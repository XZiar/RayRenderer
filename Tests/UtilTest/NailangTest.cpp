#include "TestRely.h"
#include "Nailang/NailangParser.h"
#include "SystemCommon/ConsoleEx.h"
#include "SystemCommon/ColorConsole.h"
#include "StringUtil/Detect.h"
#include "StringUtil/Convert.h"
#include "common/Linq2.hpp"
#include <iostream>

using namespace common::mlog;
using namespace common;
using namespace xziar::nailang;
using namespace std::string_view_literals;
using std::vector;
using std::string;
using std::u16string;
using std::string_view;
using std::u16string_view;
using std::u32string_view;
using std::cin;

static MiniLogger<false>& log()
{
    static MiniLogger<false> log(u"Nailang", { GetConsoleBackend() });
    return log;
}
static const common::console::ConsoleHelper& out()
{
    static common::console::ConsoleHelper helper;
    return helper;
}

#define OutLine(clr, indent, obj, type, syntax, ...) \
    out().Print(FMTSTR(u"\x1b[37m[{:5}]{}\x1b[92m" type " {}" syntax "\x1b[39m\n", obj.Position.first,\
    indent, common::console::ConsoleHelper::GetColorStr(common::console::ConsoleColor::clr), __VA_ARGS__))

static void OutIndent(const u16string& indent)
{
    out().Print(FMTSTR(u"\x1b[37m       {}\x1b[39m\n", indent));
}


static void ShowMeta(const common::span<const FuncCall> metas, const u16string& indent)
{
    for (const auto& meta : metas)
    {
        OutLine(Magenta, indent, meta, "@meta ", "func[{}], arg[{}]", meta.Name, meta.Args.size());
    }
}


static void ShowContent(MemoryPool& pool, const common::span<const FuncCall> metas, const Assignment& content, const u16string& indent)
{
    ShowMeta(metas, indent);
    OutLine(BrightWhite, indent, content, "Assign", "[{}]", content.GetVar());
}
static void ShowContent(MemoryPool& pool, const common::span<const FuncCall> metas, const FuncCall& content, const u16string& indent)
{
    ShowMeta(metas, indent);
    OutLine(BrightWhite, indent, content, "Call  ", "func[{}], arg[{}]", content.Name, content.Args.size());
}
static void ShowBlock(MemoryPool& pool, const Block& block, const u16string& indent);
static void ShowContent(MemoryPool& pool, const common::span<const FuncCall> metas, const RawBlock& content, const u16string& indent)
{
    ShowMeta(metas, indent);
    OutLine(BrightWhite, indent, content, "RawBlk", "type[{}], name[{}]", content.Type, content.Name);
    if (common::linq::FromIterable(metas)
        .ContainsIf([](const FuncCall& call) { return call.Name == U"parse"sv; }))
    {
        ShowBlock(pool, BlockParser::ParseRawBlock(content, pool), indent);
    }
    OutIndent(indent);
}
static void ShowContent(MemoryPool& pool, const common::span<const FuncCall> metas, const Block& content, const u16string& indent)
{
    ShowMeta(metas, indent);
    OutLine(BrightWhite, indent, content, "Block ", "type[{}], name[{}]", content.Type, content.Name);
    ShowBlock(pool, content, indent);
    OutIndent(indent);
}
static void ShowBlock(MemoryPool& pool, const Block& block, const u16string& indent)
{
    u16string indent2 = indent + u"\u2506   ";
    OutIndent(indent2);
    //for (const auto pp : block);
    for (const auto [metas, content] : block)
    {
        content.Visit([&, meta = metas](const auto* ele)
            {
                ShowContent(pool, meta, *ele, indent2);
                OutIndent(indent2);
            });
    }
}

static void TestNailang()
{
    while (true)
    {
        try
        {
            common::mlog::SyncConsoleBackend();
            string fpath = common::console::ConsoleEx::ReadLine("input nailang file:");
            const auto data = common::file::ReadAll<std::byte>(fpath);
            const auto chset = common::str::DetectEncoding(data);
            log().verbose(u"file has encoding [{}].\n", common::str::getCharsetName(chset));
            const auto u32str = common::str::to_u32string(data, chset);
            const auto u16fname = common::str::to_u16string(fpath, common::str::DetectEncoding(fpath));

            common::parser::ParserContext context(u32str, u16fname);
            MemoryPool pool;
            const auto all = BlockParser::ParseAllAsBlock(pool, context);
            for (const auto [meta, content] : all)
            {
                switch (content.GetType())
                {
                case xziar::nailang::BlockContent::Type::RawBlock:
                    ShowContent(pool, meta, *content.Get<RawBlock>(), u"");
                    break;
                case xziar::nailang::BlockContent::Type::Block:
                    ShowContent(pool, meta, *content.Get<Block>(), u"");
                    break;
                default:
                    COMMON_THROW(common::BaseException, u"should not exist Non-Block content here"sv);
                }
            }
        }
        catch (const common::BaseException& be)
        {
            PrintException(be, u"Exception");
        }
        log().info(u"\n<=End of file=>\n\n");
        ClearReturn();
    }
}


const static uint32_t ID = RegistTest("NailangTest", &TestNailang);