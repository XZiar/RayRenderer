#include "rely.h"
#include "common/parser/SectorFile/SectorsParser.h"


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

