#include "rely.h"
#include "Nailang/ParserRely.h"
#include "Nailang/BlockParser.h"
#include "Nailang/ArgParser.h"


using namespace std::string_view_literals;
using common::parser::ParserContext;
using common::parser::ContextReader;
using xziar::nailang::BlockParser;
using xziar::nailang::ComplexArgParser;
using xziar::nailang::EmbedOps;
using xziar::nailang::LateBindVar;
using xziar::nailang::BinaryStatement;
using xziar::nailang::UnaryStatement;
using xziar::nailang::FuncCall;
using xziar::nailang::RawArg;

#define CHECK_VAR_ARG(target, type, val)                    \
do                                                          \
{                                                           \
    EXPECT_EQ(target.TypeData, RawArg::Type::type);         \
    EXPECT_EQ(target.GetVar<RawArg::Type::type>(), val);    \
} while(0)


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

TEST(NailangParser, ParseFuncBody)
{
    xziar::nailang::MemoryPool pool;
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
        CHECK_VAR_ARG(func.Args[0], Uint, 123);
        CHECK_VAR_ARG(func.Args[1],  Int, -456);
    }
    {
        constexpr auto src = U"((123), v.456)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 2);
        CHECK_VAR_ARG(func.Args[0], Uint, 123);
        CHECK_VAR_ARG(func.Args[1], Var, U"v.456");
    }
    {
        constexpr auto src = U"(1 + 2)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 1);

        EXPECT_EQ(func.Args[0].TypeData, RawArg::Type::Binary);
        const auto& stmt = *func.Args[0].GetVar<RawArg::Type::Binary>();
        CHECK_VAR_ARG(stmt.LeftOprend, Uint, 1);
        EXPECT_EQ(stmt.Operator, EmbedOps::Add);
        CHECK_VAR_ARG(stmt.RightOprend, Uint, 2);
    }
    {
        constexpr auto src = U"(!false, 3.5 + var)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 2);
        EXPECT_EQ(func.Args[0].TypeData, RawArg::Type::Unary);
        EXPECT_EQ(func.Args[1].TypeData, RawArg::Type::Binary);
        {
            const auto& stmt = *func.Args[0].GetVar<RawArg::Type::Unary>();
            EXPECT_EQ(stmt.Operator, EmbedOps::Not);
            CHECK_VAR_ARG(stmt.Oprend, Bool, false);
        }
        {
            const auto& stmt = *func.Args[1].GetVar<RawArg::Type::Binary>();
            CHECK_VAR_ARG(stmt.LeftOprend, FP, 3.5);
            EXPECT_EQ(stmt.Operator, EmbedOps::Add);
            CHECK_VAR_ARG(stmt.RightOprend, Var, U"var");
        }
    }
    {
        constexpr auto src = U"(6 >= $foo(bar), $foo(bar, (4+5)==9))"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 2);
        EXPECT_EQ(func.Args[0].TypeData, RawArg::Type::Binary);
        EXPECT_EQ(func.Args[1].TypeData, RawArg::Type::Func);
        {
            const auto& stmt = *func.Args[0].GetVar<RawArg::Type::Binary>();
            CHECK_VAR_ARG(stmt.LeftOprend, Uint, 6);
            EXPECT_EQ(stmt.Operator, EmbedOps::GreaterEqual);
            EXPECT_EQ(stmt.RightOprend.TypeData, RawArg::Type::Func);
            const auto& fcall = *stmt.RightOprend.GetVar<RawArg::Type::Func>();
            EXPECT_EQ(fcall.Name, U"foo"sv);
            EXPECT_EQ(fcall.Args.size(), 1);
            CHECK_VAR_ARG(fcall.Args[0], Var, U"bar");
        }
        {
            const auto& fcall = *func.Args[1].GetVar<RawArg::Type::Func>();
            EXPECT_EQ(fcall.Name, U"foo"sv);
            EXPECT_EQ(fcall.Args.size(), 2);
            CHECK_VAR_ARG(fcall.Args[0], Var, U"bar");
            EXPECT_EQ(fcall.Args[1].TypeData, RawArg::Type::Binary);
            const auto& stmt = *fcall.Args[1].GetVar<RawArg::Type::Binary>();
            EXPECT_EQ(stmt.LeftOprend.TypeData, RawArg::Type::Binary);
            {
                const auto& stmt2 = *stmt.LeftOprend.GetVar<RawArg::Type::Binary>();
                CHECK_VAR_ARG(stmt2.LeftOprend, Uint, 4);
                EXPECT_EQ(stmt2.Operator, EmbedOps::Add);
                CHECK_VAR_ARG(stmt2.RightOprend, Uint, 5);
            }
            EXPECT_EQ(stmt.Operator, EmbedOps::Equal);
            CHECK_VAR_ARG(stmt.RightOprend, Uint, 9);
        }
    }
}


TEST(NailangParser, ParseBlockBody)
{
    xziar::nailang::MemoryPool pool;
    const auto ParseAll = [&pool](std::u32string_view src)
    {
        xziar::nailang::RawBlock block;// { U""sv, U""sv, src, u"", { 0,0 } };
        block.Source = src;
        return xziar::nailang::BlockParser::ParseBlockRaw(block, pool);
    };
    {
        constexpr auto src = UR"(
$func(hey);
)"sv;
        const auto block = ParseAll(src);
        EXPECT_EQ(block.Content.size(), 1);
        const auto [meta, stmt] = block[0];
        EXPECT_EQ(stmt.GetType(), xziar::nailang::BlockContent::Type::FuncCall);
        EXPECT_EQ(meta.size(), 0);
        const auto& fcall = *std::get<1>(stmt.GetStatement());
        EXPECT_EQ(fcall.Name, U"func"sv);
        EXPECT_EQ(fcall.Args.size(), 1);
        CHECK_VAR_ARG(fcall.Args[0], Var, U"hey");
    }
    {
        constexpr auto src = UR"(
@meta()
hey = 13;
)"sv;
        const auto block = ParseAll(src);
        EXPECT_EQ(block.Content.size(), 1);
        const auto [meta, stmt] = block[0];
        EXPECT_EQ(stmt.GetType(), xziar::nailang::BlockContent::Type::Assignment);
        EXPECT_EQ(meta.size(), 1);
        EXPECT_EQ(meta[0].Name, U"meta"sv);
        EXPECT_EQ(meta[0].Args.size(), 0);
        const auto& assign = *std::get<0>(stmt.GetStatement());
        EXPECT_EQ(assign.Variable.Name, U"hey"sv);
        CHECK_VAR_ARG(assign.Statement, Uint, 13u);
    }
    {
        constexpr auto src = UR"(
hey /= 9+1;
$func(hey);
@meta()
#Block.Main("block")
{
empty
}
)"sv;
        const auto block = ParseAll(src);
        EXPECT_EQ(block.Content.size(), 3);
        {
            const auto [meta, stmt] = block[0];
            EXPECT_EQ(stmt.GetType(), xziar::nailang::BlockContent::Type::Assignment);
            EXPECT_EQ(meta.size(), 0);
            const auto& assign = *std::get<0>(stmt.GetStatement());
            EXPECT_EQ(assign.Variable.Name, U"hey"sv);
            EXPECT_EQ(assign.Statement.TypeData, RawArg::Type::Binary);
            {
                const auto& stmt_ = *assign.Statement.GetVar<RawArg::Type::Binary>();
                CHECK_VAR_ARG(stmt_.LeftOprend, Var, U"hey");
                EXPECT_EQ(stmt_.Operator, EmbedOps::Div);
                EXPECT_EQ(stmt_.RightOprend.TypeData, RawArg::Type::Binary);
                {
                    const auto& stmt2 = *stmt_.RightOprend.GetVar<RawArg::Type::Binary>();
                    CHECK_VAR_ARG(stmt2.LeftOprend, Uint, 9u);
                    EXPECT_EQ(stmt2.Operator, EmbedOps::Add);
                    CHECK_VAR_ARG(stmt2.RightOprend, Uint, 1u);
                }
            }
        }
        {
            const auto [meta, stmt] = block[1];
            EXPECT_EQ(stmt.GetType(), xziar::nailang::BlockContent::Type::FuncCall);
            EXPECT_EQ(meta.size(), 0);
            const auto& fcall = *std::get<1>(stmt.GetStatement());
            EXPECT_EQ(fcall.Name, U"func"sv);
            EXPECT_EQ(fcall.Args.size(), 1);
            CHECK_VAR_ARG(fcall.Args[0], Var, U"hey");
        }
        {
            const auto [meta, stmt] = block[2];
            EXPECT_EQ(stmt.GetType(), xziar::nailang::BlockContent::Type::RawBlock);
            EXPECT_EQ(meta.size(), 1);
            EXPECT_EQ(meta[0].Name, U"meta"sv);
            EXPECT_EQ(meta[0].Args.size(), 0);
            const auto& blk = *std::get<2>(stmt.GetStatement());
            EXPECT_EQ(ReplaceNewLine(blk.Source), U"empty\n"sv);
        }
    }
}


TEST(NailangParser, ParseBlock)
{
    xziar::nailang::MemoryPool pool;
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

TEST(NailangParser, ParseMetaBlock)
{
    xziar::nailang::MemoryPool pool;
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
            CHECK_VAR_ARG(meta.Args[0], Uint, 1);
        }
        {
            const auto& meta = block.MetaFunctions[1];
            EXPECT_EQ(meta.Name, U"func"sv);
            EXPECT_EQ(meta.Args.size(), 2);
            CHECK_VAR_ARG(meta.Args[0], Var, U"abc");
            CHECK_VAR_ARG(meta.Args[1], Uint, 0xff);
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

