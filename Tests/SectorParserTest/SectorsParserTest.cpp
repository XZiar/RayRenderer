#include "rely.h"
#include "common/parser/SectorFile/ParserRely.h"
#include "common/parser/SectorFile/SectorsParser.h"
#include "common/parser/SectorFile/FuncParser.h"


using namespace std::string_view_literals;
using common::parser::ParserContext;
using common::parser::ContextReader;
using xziar::sectorlang::SectorsParser;
using xziar::sectorlang::ComplexArgParser;
using xziar::sectorlang::EmbedOps;
using xziar::sectorlang::LateBindVar;
using xziar::sectorlang::BinaryStatement;
using xziar::sectorlang::UnaryStatement;
using xziar::sectorlang::FuncCall;


#define CHECK_VAR_ARG(target, type, val) EXPECT_EQ(std::get<type>(target), val)
#define CHECK_VAR_ARG_PROP(target, type, prop, val) EXPECT_EQ(std::get<type>(target).prop, val)


static std::u32string ReplaceNewLine(std::u32string_view txt)
{
    std::u32string ret;
    ret.reserve(txt.size());
    ParserContext context(txt);
    ContextReader reader(context);
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
        constexpr auto src = U"#Sector.Main(\"Hello\"){\r\n}"sv;
        ParserContext context(src);
        SectorsParser Parser(context);
        const auto sector = Parser.ParseNextSector();
        EXPECT_EQ(sector.Type, U"Main"sv);
        EXPECT_EQ(sector.Name, U"Hello"sv);
        EXPECT_EQ(sector.Content, U""sv);
    }

    {
        constexpr auto src = U"#Sector.Main(\"Hello\"){\r\n}\r\n#Sector.Main(\"Hello\"){\r\n}"sv;
        ParserContext context(src);
        SectorsParser Parser(context);

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
#Sector.Main("Hello")
{
Here
}

#Sector.Main("Hello")
{===+++
{Here}
===+++}
)"sv;
        ParserContext context(src);
        SectorsParser Parser(context);

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

//using BinStmt = std::unique_ptr<BinaryStatement>;
//using UnStmt  = std::unique_ptr<UnaryStatement>;
//using FCall   = std::unique_ptr<FuncCall>;
using BinStmt = BinaryStatement*;
using UnStmt  = UnaryStatement*;
using FCall   = FuncCall*;
TEST(SectorsParser, ParseMetaFunc)
{
    constexpr auto ParseFunc = [](const std::u32string_view src)
    {
        ParserContext context(src);
        return xziar::sectorlang::FuncBodyParser::ParseFuncBody(U"func"sv, context);
    };
    {
        const auto func = ParseFunc(U"()"sv);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 0);
    }
    {
        const auto func = ParseFunc(U"(123)"sv);
        EXPECT_EQ(func.Args.size(), 1);
        CHECK_VAR_ARG(func.Args[0], uint64_t, 123);
    }
    {
        const auto func = ParseFunc(U"(123, -4.5)"sv);
        EXPECT_EQ(func.Args.size(), 2);
        CHECK_VAR_ARG(func.Args[0], uint64_t, 123);
        CHECK_VAR_ARG(func.Args[1], double,   -4.5);
    }
    {
        const auto func = ParseFunc(U"(v3.1, -4.5, k67)"sv);
        EXPECT_EQ(func.Args.size(), 3);
        CHECK_VAR_ARG_PROP(func.Args[0], LateBindVar, Name, U"v3.1"sv);
        CHECK_VAR_ARG(func.Args[1], double,      -4.5);
        CHECK_VAR_ARG_PROP(func.Args[2], LateBindVar, Name, U"k67"sv);
    }
}


TEST(SectorsParser, ParseMetaSector)
{
    {
        constexpr auto src =
            UR"(
@func()
#Sector.Main("Hello")
{
Here
}
)"sv;
        ParserContext context(src);
        SectorsParser Parser(context);

        const auto sector = Parser.ParseNextSector();
        EXPECT_EQ(sector.Type, U"Main"sv);
        EXPECT_EQ(sector.Name, U"Hello"sv);
        EXPECT_EQ(ReplaceNewLine(sector.Content), U"Here\n"sv);
        EXPECT_EQ(sector.MetaFunctions.size(), 1);
        const auto& meta = sector.MetaFunctions[0];
        EXPECT_EQ(meta.Name, U"func"sv);
        EXPECT_EQ(meta.Args.size(), 0);
    }
    {
        constexpr auto src =
            UR"(
@func(1)
@func(abc, 0xff)
#Sector.Main("Hello")
{
Here
}
#Sector.Main("Hello")
{
Here
}
)"sv;
        ParserContext context(src);
        SectorsParser Parser(context);

        const auto sector = Parser.ParseNextSector();
        EXPECT_EQ(sector.Type, U"Main"sv);
        EXPECT_EQ(sector.Name, U"Hello"sv);
        EXPECT_EQ(ReplaceNewLine(sector.Content), U"Here\n"sv);
        EXPECT_EQ(sector.MetaFunctions.size(), 2);
        {
            const auto& meta = sector.MetaFunctions[0];
            EXPECT_EQ(meta.Name, U"func"sv);
            EXPECT_EQ(meta.Args.size(), 1);
            CHECK_VAR_ARG(meta.Args[0], uint64_t, 1);
        }
        {
            const auto& meta = sector.MetaFunctions[1];
            EXPECT_EQ(meta.Name, U"func"sv);
            EXPECT_EQ(meta.Args.size(), 2);
            CHECK_VAR_ARG_PROP(meta.Args[0], LateBindVar, Name, U"abc");
            CHECK_VAR_ARG(meta.Args[1], uint64_t,    0xff);
        }
        {
            const auto sector_ = Parser.ParseNextSector();
            EXPECT_EQ(sector_.Type, U"Main"sv);
            EXPECT_EQ(sector_.Name, U"Hello"sv);
            EXPECT_EQ(ReplaceNewLine(sector_.Content), U"Here\n"sv);
            EXPECT_EQ(sector_.MetaFunctions.size(), 0);
        }
    }
}


TEST(SectorsParser, ParseFuncBody)
{
    xziar::sectorlang::MemoryPool pool;
    {
        constexpr auto src = U"()"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 0);
    }
    {
        constexpr auto src = U"(())"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 0);
    }
    {
        constexpr auto src = U"(((())))"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 0);
    }
    {
        constexpr auto src = U"(123, -456)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 2);
        CHECK_VAR_ARG(func.Args[0], uint64_t, 123);
        CHECK_VAR_ARG(func.Args[1], int64_t,  -456);
    }
    {
        constexpr auto src = U"((123), -456)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 2);
        CHECK_VAR_ARG(func.Args[0], uint64_t, 123);
        CHECK_VAR_ARG(func.Args[1], int64_t, -456);
    }
    {
        constexpr auto src = U"(1 + 2)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 1);
        EXPECT_TRUE(std::holds_alternative<BinStmt>(func.Args[0]));
        const auto& stmt = *std::get<BinStmt>(func.Args[0]);
        CHECK_VAR_ARG(stmt.LeftOprend,  uint64_t, 1);
        EXPECT_EQ(stmt.Operator, EmbedOps::Add);
        CHECK_VAR_ARG(stmt.RightOprend, uint64_t, 2);
    }
    {
        constexpr auto src = U"(!false, 3.5 + var)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 2);
        EXPECT_TRUE(std::holds_alternative<UnStmt> (func.Args[0]));
        EXPECT_TRUE(std::holds_alternative<BinStmt>(func.Args[1]));
        {
            const auto& stmt = *std::get<UnStmt>(func.Args[0]);
            EXPECT_EQ(stmt.Operator, EmbedOps::Not);
            CHECK_VAR_ARG(stmt.Oprend, bool, false);
        }
        {
            const auto& stmt = *std::get<BinStmt>(func.Args[1]);
            CHECK_VAR_ARG(stmt.LeftOprend, double, 3.5);
            EXPECT_EQ(stmt.Operator, EmbedOps::Add);
            CHECK_VAR_ARG_PROP(stmt.RightOprend, LateBindVar, Name, U"var");
        }
    }
    {
        constexpr auto src = U"(6 >= $foo(bar), $foo(bar, (4+5)==9))"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 2);
        EXPECT_TRUE(std::holds_alternative<BinStmt>(func.Args[0]));
        EXPECT_TRUE(std::holds_alternative<FCall>  (func.Args[1]));
        {
            const auto& stmt = *std::get<BinStmt>(func.Args[0]);
            CHECK_VAR_ARG(stmt.LeftOprend, uint64_t, 6);
            EXPECT_EQ(stmt.Operator, EmbedOps::GreaterEqual);
            EXPECT_TRUE(std::holds_alternative<FCall>(stmt.RightOprend));
            const auto& fcall = *std::get<FCall>(stmt.RightOprend);
            EXPECT_EQ(fcall.Name, U"foo"sv);
            EXPECT_EQ(fcall.Args.size(), 1);
            CHECK_VAR_ARG_PROP(fcall.Args[0], LateBindVar, Name, U"bar");
        }
        {
            const auto& fcall = *std::get<FCall>(func.Args[1]);
            EXPECT_EQ(fcall.Name, U"foo"sv);
            EXPECT_EQ(fcall.Args.size(), 2);
            CHECK_VAR_ARG_PROP(fcall.Args[0], LateBindVar, Name, U"bar");
            const auto& stmt = *std::get<BinStmt>(fcall.Args[1]);
            EXPECT_TRUE(std::holds_alternative<BinStmt>(stmt.LeftOprend));
            {
                const auto& stmt2 = *std::get<BinStmt>(stmt.LeftOprend);
                CHECK_VAR_ARG(stmt2.LeftOprend, uint64_t, 4);
                EXPECT_EQ(stmt2.Operator, EmbedOps::Add);
                CHECK_VAR_ARG(stmt2.RightOprend, uint64_t, 5);
            }
            EXPECT_EQ(stmt.Operator, EmbedOps::Equal);
            CHECK_VAR_ARG(stmt.RightOprend, uint64_t, 9);
        }
    }
}

