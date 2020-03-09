#include "rely.h"
#include "common/parser/SectorFile/ParserRely.h"
#include "common/parser/SectorFile/BlockParser.h"
#include "common/parser/SectorFile/ArgParser.h"


using namespace std::string_view_literals;
using common::parser::ParserContext;
using common::parser::ContextReader;
using xziar::sectorlang::BlockParser;
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


//using BinStmt = std::unique_ptr<BinaryStatement>;
//using UnStmt  = std::unique_ptr<UnaryStatement>;
//using FCall   = std::unique_ptr<FuncCall>;
using BinStmt = BinaryStatement*;
using UnStmt  = UnaryStatement*;
using FCall   = FuncCall*;

TEST(SectorFileParser, ParseFuncBody)
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
        CHECK_VAR_ARG(func.Args[1], int64_t, -456);
    }
    {
        constexpr auto src = U"((123), v.456)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 2);
        CHECK_VAR_ARG(func.Args[0], uint64_t, 123);
        CHECK_VAR_ARG_PROP(func.Args[1], LateBindVar, Name, U"v.456");
    }
    {
        constexpr auto src = U"(1 + 2)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 1);
        EXPECT_TRUE(std::holds_alternative<BinStmt>(func.Args[0]));
        const auto& stmt = *std::get<BinStmt>(func.Args[0]);
        CHECK_VAR_ARG(stmt.LeftOprend, uint64_t, 1);
        EXPECT_EQ(stmt.Operator, EmbedOps::Add);
        CHECK_VAR_ARG(stmt.RightOprend, uint64_t, 2);
    }
    {
        constexpr auto src = U"(!false, 3.5 + var)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 2);
        EXPECT_TRUE(std::holds_alternative<UnStmt>(func.Args[0]));
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
        EXPECT_TRUE(std::holds_alternative<FCall>(func.Args[1]));
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

TEST(SectorFileParser, ParseSector)
{
    xziar::sectorlang::MemoryPool pool;
    {
        constexpr auto src = U"#Block.Main(\"Hello\"){\r\n}"sv;
        ParserContext context(src);
        BlockParser Parser(pool, context);
        const auto block = Parser.GetNextBlock();
        EXPECT_EQ(block.Type, U"Main"sv);
        EXPECT_EQ(block.Name, U"Hello"sv);
        EXPECT_EQ(block.Source, U""sv);
        EXPECT_EQ(block.Position.first,  1);
        EXPECT_EQ(block.Position.second, 0);
    }

    {
        constexpr auto src = U"#Block.Main(\"Hello\"){\r\n}\r\n#Block.Main(\"Hello\"){\r\n}"sv;
        ParserContext context(src);
        BlockParser Parser(pool, context);

        const auto block0 = Parser.GetNextBlock();
        EXPECT_EQ(block0.Type, U"Main"sv);
        EXPECT_EQ(block0.Name, U"Hello"sv);
        EXPECT_EQ(block0.Source, U""sv);
        EXPECT_EQ(block0.Position.first,  1);
        EXPECT_EQ(block0.Position.second, 0);

        const auto block1 = Parser.GetNextBlock();
        EXPECT_EQ(block1.Type, U"Main"sv);
        EXPECT_EQ(block1.Name, U"Hello"sv);
        EXPECT_EQ(block1.Source, U""sv);
        EXPECT_EQ(block1.Position.first,  3);
        EXPECT_EQ(block1.Position.second, 0);
    }
    {
        constexpr auto src = 
            UR"(
#Block.Main("Hello")
{
Here
}

#Block.Main("Hello")
{===+++
{Here}
===+++}
)"sv;
        ParserContext context(src);
        BlockParser Parser(pool, context);

        const auto block0 = Parser.GetNextBlock();
        EXPECT_EQ(block0.Type, U"Main"sv);
        EXPECT_EQ(block0.Name, U"Hello"sv);
        EXPECT_EQ(ReplaceNewLine(block0.Source), U"Here\n"sv);
        EXPECT_EQ(block0.Position.first,  3);
        EXPECT_EQ(block0.Position.second, 0);

        const auto block1 = Parser.GetNextBlock();
        EXPECT_EQ(block1.Type, U"Main"sv);
        EXPECT_EQ(block1.Name, U"Hello"sv);
        EXPECT_EQ(ReplaceNewLine(block1.Source), U"{Here}\n"sv);
        EXPECT_EQ(block1.Position.first,  8);
        EXPECT_EQ(block1.Position.second, 0);
    }
}

TEST(SectorFileParser, ParseMetaSector)
{
    xziar::sectorlang::MemoryPool pool;
    {
        constexpr auto src =
            UR"(
@func()
#Block.Main("Hello")
{
Here
}
)"sv;
        ParserContext context(src);
        BlockParser Parser(pool, context);

        const auto block = Parser.GetNextBlock();
        EXPECT_EQ(block.Type, U"Main"sv);
        EXPECT_EQ(block.Name, U"Hello"sv);
        EXPECT_EQ(ReplaceNewLine(block.Source), U"Here\n"sv);
        EXPECT_EQ(block.MetaFunctions.size(), 1);
        EXPECT_EQ(block.Position.first,  4);
        EXPECT_EQ(block.Position.second, 0);
        const auto& meta = block.MetaFunctions[0];
        EXPECT_EQ(meta.Name, U"func"sv);
        EXPECT_EQ(meta.Args.size(), 0);
    }
    {
        constexpr auto src =
            UR"(
@func(1)
@func(abc, 0xff)
#Block.Main("Hello")
{
Here
}
#Block.Main("Hello")
{
Here
}
)"sv;
        ParserContext context(src);
        BlockParser Parser(pool, context);

        const auto block = Parser.GetNextBlock();
        EXPECT_EQ(block.Type, U"Main"sv);
        EXPECT_EQ(block.Name, U"Hello"sv);
        EXPECT_EQ(ReplaceNewLine(block.Source), U"Here\n"sv);
        EXPECT_EQ(block.Position.first,  5);
        EXPECT_EQ(block.Position.second, 0);
        EXPECT_EQ(block.MetaFunctions.size(), 2);
        {
            const auto& meta = block.MetaFunctions[0];
            EXPECT_EQ(meta.Name, U"func"sv);
            EXPECT_EQ(meta.Args.size(), 1);
            CHECK_VAR_ARG(meta.Args[0], uint64_t, 1);
        }
        {
            const auto& meta = block.MetaFunctions[1];
            EXPECT_EQ(meta.Name, U"func"sv);
            EXPECT_EQ(meta.Args.size(), 2);
            CHECK_VAR_ARG_PROP(meta.Args[0], LateBindVar, Name, U"abc");
            CHECK_VAR_ARG(meta.Args[1], uint64_t, 0xff);
        }
        {
            const auto block_ = Parser.GetNextBlock();
            EXPECT_EQ(block_.Type, U"Main"sv);
            EXPECT_EQ(block_.Name, U"Hello"sv);
            EXPECT_EQ(ReplaceNewLine(block_.Source), U"Here\n"sv);
            EXPECT_EQ(block_.Position.first,  9);
            EXPECT_EQ(block_.Position.second, 0);
            EXPECT_EQ(block_.MetaFunctions.size(), 0);
        }
    }
}


