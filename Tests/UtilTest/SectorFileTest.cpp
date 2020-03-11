#include "TestRely.h"
#include "common/parser/SectorFile/BlockParser.h"
#include "common/linq2.hpp"
#include "StringCharset/Detect.h"
#include "StringCharset/Convert.h"
#include <iostream>

using namespace common::mlog;
using namespace common;
using namespace xziar::sectorlang;
using std::vector;
using std::string;
using std::string_view;
using std::u16string_view;
using std::u32string_view;
using std::cin;

static MiniLogger<false>& log()
{
    static MiniLogger<false> log(u"SecFileTest", { GetConsoleBackend() });
    return log;
}


static void ShowMeta(const common::span<const FuncCall> metas, const size_t level)
{
    std::string ident(level * 2, ' ');
    size_t idx = 0;
    for (const auto& meta : metas)
    {
        log().info(FMT_STRING(u"{}@meta[{}] func[{}], arg[{}]\n"), ident, idx, meta.Name, meta.Args.size());
    }
}




//case Type::Assignment:  return visitor(Get<AssignmentWithMeta>());
//case Type::FuncCall:    return visitor(Get<  FuncCallWithMeta>());
//case Type::RawBlock:    return visitor(Get<  RawBlockWithMeta>());
//default:Expects(false); return visitor(static_cast<const AssignmentWithMeta*>(nullptr));

static void ShowContent(MemoryPool& pool, const AssignmentWithMeta& content, const size_t level)
{
    std::string ident(level * 2, ' ');
    ShowMeta(content.MetaFunctions, level);
    log().info(FMT_STRING(u"{}assign [{}]\n"), ident, content.Variable.Name);
}
static void ShowContent(MemoryPool& pool, const FuncCallWithMeta& content, const size_t level)
{
    std::string ident(level * 2, ' ');
    ShowMeta(content.MetaFunctions, level);
    log().info(FMT_STRING(u"{}call func[{}], arg[{}]\n"), ident, content.Name, content.Args.size());
}
static void ShowContent(MemoryPool& pool, const RawBlockWithMeta& content, const size_t level)
{
    std::string ident(level * 2, ' ');
    ShowMeta(content.MetaFunctions, level);
    log().info(FMT_STRING(u"{}block type[{}], name[{}]\n"), ident, content.Type, content.Name);
    const auto block = BlockParser::ParseBlockRaw(content, pool);
    for (const auto& content : block.Content)
    {
        content.Visit([&](const auto* content) 
            {
                ShowContent(pool, *content, level + 1);
            });
    }
}

static void TestSecFile()
{
    while (true)
    {
        try
        {
            log().info(u"\n\ninput sector file:");
            string fpath;
            std::getline(cin, fpath);
            ClearReturn();
            const auto data = common::file::ReadAll<std::byte>(fpath);
            const auto chset = common::strchset::DetectEncoding(data);
            log().verbose(u"file has encoding [{}].\n", common::str::getCharsetName(chset));
            const auto u32str = common::strchset::to_u32string(data, chset);
            const auto u16fname = common::strchset::to_u16string(fpath, common::strchset::DetectEncoding(fpath));

            common::parser::ParserContext context(u32str, u16fname);
            MemoryPool pool;
            BlockParser parser(pool, context);
            const auto sectors = parser.GetAllBlocks();
            for (const auto block : sectors)
            {
                ShowContent(pool, block, 0);
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


const static uint32_t ID = RegistTest("SecFileTest", &TestSecFile);