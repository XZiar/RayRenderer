#include "TestRely.h"
#include "Nailang/NailangParser.h"
#include "common/Linq2.hpp"
#include "StringCharset/Detect.h"
#include "StringCharset/Convert.h"
#include <iostream>

using namespace common::mlog;
using namespace common;
using namespace xziar::nailang;
using namespace std::string_view_literals;
using std::vector;
using std::string;
using std::string_view;
using std::u16string_view;
using std::u32string_view;
using std::cin;

static MiniLogger<false>& log()
{
    static MiniLogger<false> log(u"Nailang", { GetConsoleBackend() });
    return log;
}


static void ShowMeta(const common::span<const FuncCall> metas, const string& indent)
{
    size_t idx = 0;
    for (const auto& meta : metas)
    {
        log().info(FMT_STRING(u"{}@meta[{}] func[{}], arg[{}]\n"), indent, idx++, meta.Name, meta.Args.size());
    }
}


static void ShowContent(MemoryPool& pool, const common::span<const FuncCall> metas, const Assignment& content, const string& indent)
{
    ShowMeta(metas, indent);
    log().info(FMT_STRING(u"{}assign [{}]\n"), indent, content.Variable.Name);
}
static void ShowContent(MemoryPool& pool, const common::span<const FuncCall> metas, const FuncCall& content, const string& indent)
{
    ShowMeta(metas, indent);
    log().info(FMT_STRING(u"{}call func[{}], arg[{}]\n"), indent, content.Name, content.Args.size());
}
static void ShowBlock(MemoryPool& pool, const Block& block, const string& indent);
static void ShowContent(MemoryPool& pool, const common::span<const FuncCall> metas, const RawBlock& content, const string& indent)
{
    ShowMeta(metas, indent);
    log().info(FMT_STRING(u"{}rawblock type[{}], name[{}]\n"), indent, content.Type, content.Name);
    if (common::linq::FromIterable(metas)
        .ContainsIf([](const FuncCall& call) { return call.Name == U"parse"sv; }))
    {
        ShowBlock(pool, BlockParser::ParseRawBlock(content, pool), indent);
    }
    log().info(u"{}\n", indent);
}
static void ShowContent(MemoryPool& pool, const common::span<const FuncCall> metas, const Block& content, const string& indent)
{
    ShowMeta(metas, indent);
    log().info(FMT_STRING(u"{}block type[{}], name[{}]\n"), indent, content.Type, content.Name);
    ShowBlock(pool, content, indent);
    log().info(u"{}\n", indent);
}
static void ShowBlock(MemoryPool& pool, const Block& block, const string& indent)
{
    string indent2 = indent + "|   ";
    log().info(u"{}\n", indent2);
    //for (const auto pp : block);
    for (const auto [metas, content] : block)
    {
        content.Visit([&, meta = metas](const auto* ele)
            {
                ShowContent(pool, meta, *ele, indent2);
                log().info(u"{}\n", indent2);
            });
    }
}

static void TestNailang()
{
    while (true)
    {
        try
        {
            log().info(u"\n\ninput nailang file:");
            string fpath;
            std::getline(cin, fpath);
            const auto data = common::file::ReadAll<std::byte>(fpath);
            const auto chset = common::strchset::DetectEncoding(data);
            log().verbose(u"file has encoding [{}].\n", common::str::getCharsetName(chset));
            const auto u32str = common::strchset::to_u32string(data, chset);
            const auto u16fname = common::strchset::to_u16string(fpath, common::strchset::DetectEncoding(fpath));

            common::parser::ParserContext context(u32str, u16fname);
            MemoryPool pool;
            const auto all = BlockParser::ParseAllAsBlock(pool, context);
            for (const auto [meta, content] : all)
            {
                switch (content.GetType())
                {
                case xziar::nailang::BlockContent::Type::RawBlock:
                    ShowContent(pool, meta, *content.Get<RawBlock>(), "  ");
                    break;
                case xziar::nailang::BlockContent::Type::Block:
                    ShowContent(pool, meta, *content.Get<Block>(), "  ");
                    break;
                default:
                    throw u"should not exist Non-Block content here"sv;
                }
            }
        }
        catch (common::BaseException & be)
        {
            log().error(u"Exception: \n{}\n", (u16string_view)be.message);
        }
        catch (std::u32string_view & sv)
        {
            log().error(u"Exception: {}\n", sv);
        }
        catch (std::u16string_view & sv)
        {
            log().error(u"Exception: {}\n", sv);
        }
    }
}


const static uint32_t ID = RegistTest("NailangTest", &TestNailang);