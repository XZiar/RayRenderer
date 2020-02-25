#include "rely.h"
#include "common/parser/SectorFile/SectorsParser.h"
#include "common/parser/SectorFile/MetaFuncParser.h"


using namespace std::string_view_literals;


static std::u32string ReplaceNewLine(std::u32string_view txt)
{
    std::u32string ret;
    ret.reserve(txt.size());
    common::parser::ParserContext context(txt);
    common::parser::ContextReader reader(context);
    while (true)
    {
        const auto ch = reader.ReadNext();
        if (ch != common::parser::special::CharEnd)
            ret.push_back(ch);
        else
            break;
    }
    return ret;
}

TEST(SectorsParser, ParseSector)
{
    {
        constexpr auto src = U"$Sector.Main(\"Hello\"){\r\n}"sv;
        common::parser::ParserContext context(src);
        xziar::sectorlang::SectorsParser Parser(context);
        const auto sector = Parser.ParseNextSector();
        EXPECT_EQ(sector.Type, U"Main"sv);
        EXPECT_EQ(sector.Name, U"Hello"sv);
        EXPECT_EQ(sector.Content, U""sv);
    }

    {
        constexpr auto src = U"$Sector.Main(\"Hello\"){\r\n}\r\n$Sector.Main(\"Hello\"){\r\n}"sv;
        common::parser::ParserContext context(src);
        xziar::sectorlang::SectorsParser Parser(context);

        const auto sector0 = Parser.ParseNextSector();
        EXPECT_EQ(sector0.Type, U"Main"sv);
        EXPECT_EQ(sector0.Name, U"Hello"sv);
        EXPECT_EQ(sector0.Content, U""sv);

        const auto sector1 = Parser.ParseNextSector();
        EXPECT_EQ(sector1.Type, U"Main"sv);
        EXPECT_EQ(sector1.Name, U"Hello"sv);
        EXPECT_EQ(sector1.Content, U""sv);
    }
    {
        constexpr auto src = 
            UR"(
$Sector.Main("Hello")
{
Here
}

$Sector.Main("Hello")
{===+++
{Here}
===+++}
)"sv;
        common::parser::ParserContext context(src);
        xziar::sectorlang::SectorsParser Parser(context);

        const auto sector0 = Parser.ParseNextSector();
        EXPECT_EQ(sector0.Type, U"Main"sv);
        EXPECT_EQ(sector0.Name, U"Hello"sv);
        EXPECT_EQ(ReplaceNewLine(sector0.Content), U"Here\n"sv);

        const auto sector1 = Parser.ParseNextSector();
        EXPECT_EQ(sector1.Type, U"Main"sv);
        EXPECT_EQ(sector1.Name, U"Hello"sv);
        EXPECT_EQ(ReplaceNewLine(sector1.Content), U"{Here}\n"sv);
    }
}


TEST(SectorsParser, ParseMetaFunc)
{
    constexpr auto ParseFunc = [](const std::u32string_view src)
    {
        common::parser::ParserContext context(src);
        return xziar::sectorlang::MetaFuncParser::ParseFuncBody(U"func"sv, context);
    };
    {
        const auto func = ParseFunc(U"()"sv);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 0);
    }
    {
        const auto func = ParseFunc(U"(123)"sv);
        EXPECT_EQ(func.Args.size(), 1);
        EXPECT_EQ(std::get<uint64_t>(func.Args[0]), 123);
    }
    {
        const auto func = ParseFunc(U"(123, -4.5)"sv);
        EXPECT_EQ(func.Args.size(), 2);
        EXPECT_EQ(std::get<uint64_t>(func.Args[0]), 123);
        EXPECT_EQ(std::get<double>  (func.Args[1]), -4.5);
    }
    {
        const auto func = ParseFunc(U"(123, -4.5, k67)"sv);
        EXPECT_EQ(func.Args.size(), 3);
        EXPECT_EQ(std::get<uint64_t>(func.Args[0]), 123);
        EXPECT_EQ(std::get<double>  (func.Args[1]), -4.5);
        EXPECT_EQ(std::get<xziar::sectorlang::LateBindVar>(func.Args[2]).Name, U"k67"sv);
    }
}

