#include "rely.h"
#include "Nailang/ParserRely.h"
#include "Nailang/NailangParser.h"
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/size.hpp>

using namespace std::string_view_literals;
using common::parser::ParserContext;
using common::parser::ContextReader;
using xziar::nailang::RawBlockParser;
using xziar::nailang::BlockParser;
using xziar::nailang::ComplexArgParser;
using xziar::nailang::EmbedOps;
using xziar::nailang::SubQuery;
using xziar::nailang::LateBindVar;
using xziar::nailang::BinaryExpr;
using xziar::nailang::UnaryExpr;
using xziar::nailang::FuncCall;
using xziar::nailang::RawArg;
using xziar::nailang::Assignment;


testing::AssertionResult CheckRawArg(const RawArg& arg, const RawArg::Type type)
{
    if (arg.TypeData == type)
        return testing::AssertionSuccess();
    else
        return testing::AssertionFailure() << "rawarg is [" << WideToChar(arg.GetTypeName()) << "](" << common::enum_cast(arg.TypeData) << ")";
}

#define CHECK_DIRECT_ARG(target, type, val)                 \
do                                                          \
{                                                           \
    ASSERT_TRUE(CheckRawArg(target, RawArg::Type::type));   \
    EXPECT_EQ(target.GetVar<RawArg::Type::type>(), val);    \
} while(0)

#define CHECK_VAR_ARG(target, val, info)                    \
do                                                          \
{                                                           \
    ASSERT_TRUE(CheckRawArg(target, RawArg::Type::Var));    \
    const auto& _var = target.GetVar<RawArg::Type::Var>();  \
    EXPECT_EQ(_var.Name, val);                              \
    EXPECT_EQ(_var.Info, LateBindVar::VarInfo::info);       \
} while(0)

#define CHECK_QUERY_SUB(target, name)                       \
do                                                          \
{                                                           \
    EXPECT_EQ(target.first, SubQuery::QueryType::Sub);      \
    CHECK_DIRECT_ARG(target.second, Str, name);             \
} while(0)

#define CHECK_QUERY_SUBFIELD(r, x, i, name) CHECK_QUERY_SUB(x[i], name);
#define CHECK_QUERY_SUBFIELDS(x, names) BOOST_PP_SEQ_FOR_EACH_I(CHECK_QUERY_SUBFIELD, x, names)
#define CHECK_FIELD_QUERY(target, var, info, ...)                           \
do                                                                          \
{                                                                           \
    ASSERT_EQ(target.TypeData, RawArg::Type::Query);                        \
    const auto& query = *target.GetVar<RawArg::Type::Query>();              \
    CHECK_VAR_ARG(query.Target, var, info);                                 \
    constexpr size_t qcnt = BOOST_PP_VARIADIC_SIZE(__VA_ARGS__);            \
    ASSERT_EQ(query.Queries.size(), qcnt);                                  \
    CHECK_QUERY_SUBFIELDS(query, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))     \
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


using BinStmt = BinaryExpr*;
using UnStmt  = UnaryExpr*;
using FCall   = FuncCall*;

TEST(NailangParser, ProcStr)
{
    xziar::nailang::MemoryPool pool;
    {
        constexpr auto src = U"Hello"sv;
        const auto dst = ComplexArgParser::ProcessString(src, pool);
        EXPECT_EQ(dst.GetVar<RawArg::Type::Str>(), U"Hello"sv);
    }
    {
        constexpr auto src = U"Hello\\tWorld"sv;
        const auto dst = ComplexArgParser::ProcessString(src, pool);
        EXPECT_EQ(dst.GetVar<RawArg::Type::Str>(), U"Hello\tWorld"sv);
    }
    {
        constexpr auto src = U"Hello\tWorld\\0"sv;
        const auto dst = ComplexArgParser::ProcessString(src, pool);
        EXPECT_EQ(dst.GetVar<RawArg::Type::Str>(), U"Hello\tWorld\0"sv);
    }
}

TEST(NailangParser, ParseFuncBody)
{
    xziar::nailang::MemoryPool pool;
    {
        constexpr auto src = U"()"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 0u);
    }
    {
        constexpr auto src = U"(())"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 0u);
    }
    {
        constexpr auto src = U"(((())))"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        EXPECT_EQ(func.Args.size(), 0u);
    }
    {
        constexpr auto src = U"(\"Hello\")"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 1u);
        CHECK_DIRECT_ARG(func.Args[0], Str, U"Hello"sv);
    }
    {
        constexpr auto src = UR"(("Hello\t\"World\""))"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 1u);
        CHECK_DIRECT_ARG(func.Args[0], Str, U"Hello\t\"World\""sv);
    }
    {
        constexpr auto src = U"(val.xxx.y.len)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 1u);
        ASSERT_EQ(func.Args[0].TypeData, RawArg::Type::Query);
        const auto& query = *func.Args[0].GetVar<RawArg::Type::Query>();
        CHECK_VAR_ARG(query.Target, U"val"sv, Empty);
        ASSERT_EQ(query.Queries.size(), 3u);
        CHECK_QUERY_SUB(query[0], U"xxx"sv);
        CHECK_QUERY_SUB(query[1], U"y"sv);
        CHECK_QUERY_SUB(query[2], U"len"sv);
    }
    {
        constexpr auto src = U"(123, -456)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 2u);
        CHECK_DIRECT_ARG(func.Args[0], Int, 123);
        CHECK_DIRECT_ARG(func.Args[1], Int, -456);
    }
    {
        constexpr auto src = U"((123u), v._456)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 2u);
        CHECK_DIRECT_ARG(func.Args[0], Uint, 123u);
        CHECK_FIELD_QUERY(func.Args[1], U"v"sv, Empty, U"_456"sv);
        //CHECK_VAR_ARG(func.Args[1], U"v.456"sv, Empty);
    }
    {
        constexpr auto src = U"(1u + 2)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 1u);

        ASSERT_EQ(func.Args[0].TypeData, RawArg::Type::Binary);
        const auto& stmt = *func.Args[0].GetVar<RawArg::Type::Binary>();
        CHECK_DIRECT_ARG(stmt.LeftOprend, Uint, 1u);
        EXPECT_EQ(stmt.Operator, EmbedOps::Add);
        CHECK_DIRECT_ARG(stmt.RightOprend, Int, 2);
    }
    {
        constexpr auto src = U"(!false, 3.5 + var)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 2u);
        ASSERT_EQ(func.Args[0].TypeData, RawArg::Type::Unary);
        ASSERT_EQ(func.Args[1].TypeData, RawArg::Type::Binary);
        {
            const auto& stmt = *func.Args[0].GetVar<RawArg::Type::Unary>();
            EXPECT_EQ(stmt.Operator, EmbedOps::Not);
            CHECK_DIRECT_ARG(stmt.Oprend, Bool, false);
        }
        {
            const auto& stmt = *func.Args[1].GetVar<RawArg::Type::Binary>();
            CHECK_DIRECT_ARG(stmt.LeftOprend, FP, 3.5);
            EXPECT_EQ(stmt.Operator, EmbedOps::Add);
            CHECK_VAR_ARG(stmt.RightOprend, U"var"sv, Empty);
        }
    }
    {
        constexpr auto src = U"(val[5])"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 1u);

        ASSERT_EQ(func.Args[0].TypeData, RawArg::Type::Query);
        const auto& stmt = *func.Args[0].GetVar<RawArg::Type::Query>();
        CHECK_VAR_ARG(stmt.Target, U"val"sv, Empty);
        EXPECT_EQ(stmt[0].first, SubQuery::QueryType::Index);
        CHECK_DIRECT_ARG(stmt[0].second, Int, 5);
    }
    {
        constexpr auto src = U"(val[x+y]+ 12.8)"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 1u);

        ASSERT_EQ(func.Args[0].TypeData, RawArg::Type::Binary);
        const auto& stmt = *func.Args[0].GetVar<RawArg::Type::Binary>();
        EXPECT_EQ(stmt.LeftOprend.TypeData, RawArg::Type::Query);
        EXPECT_EQ(stmt.Operator, EmbedOps::Add);
        CHECK_DIRECT_ARG(stmt.RightOprend, FP, 12.8);

        const auto& stmt1 = *stmt.LeftOprend.GetVar<RawArg::Type::Query>();
        CHECK_VAR_ARG(stmt1.Target, U"val"sv, Empty);
        EXPECT_EQ(stmt1[0].first, SubQuery::QueryType::Index);
        ASSERT_EQ(stmt1[0].second.TypeData, RawArg::Type::Binary);

        const auto& stmt2 = *stmt1[0].second.GetVar<RawArg::Type::Binary>();
        CHECK_VAR_ARG(stmt2.LeftOprend, U"x"sv, Empty);
        EXPECT_EQ(stmt2.Operator, EmbedOps::Add);
        CHECK_VAR_ARG(stmt2.RightOprend, U"y"sv, Empty);
    }
    {
        constexpr auto src = U"(!val[x[3+4][5]])"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 1u);

        ASSERT_EQ(func.Args[0].TypeData, RawArg::Type::Unary);
        const auto& stmt = *func.Args[0].GetVar<RawArg::Type::Unary>();
        EXPECT_EQ(stmt.Operator, EmbedOps::Not);
        ASSERT_EQ(stmt.Oprend.TypeData, RawArg::Type::Query);

        const auto& stmt1 = *stmt.Oprend.GetVar<RawArg::Type::Query>();
        CHECK_VAR_ARG(stmt1.Target, U"val"sv, Empty);
        ASSERT_EQ(stmt1.Queries.size(), 1u);
        EXPECT_EQ(stmt1[0].first, SubQuery::QueryType::Index);
        ASSERT_EQ(stmt1[0].second.TypeData, RawArg::Type::Query);

        const auto& stmt2 = *stmt1[0].second.GetVar<RawArg::Type::Query>();
        CHECK_VAR_ARG(stmt2.Target, U"x"sv, Empty);
        ASSERT_EQ(stmt2.Queries.size(), 2u);
        EXPECT_EQ(stmt2[0].first, SubQuery::QueryType::Index);
        ASSERT_EQ(stmt2[0].second.TypeData, RawArg::Type::Binary);

        const auto& stmt3 = *stmt2[0].second.GetVar<RawArg::Type::Binary>();
        CHECK_DIRECT_ARG(stmt3.LeftOprend, Int, 3);
        EXPECT_EQ(stmt3.Operator, EmbedOps::Add);
        CHECK_DIRECT_ARG(stmt3.RightOprend, Int, 4);

        EXPECT_EQ(stmt2[1].first, SubQuery::QueryType::Index);
        CHECK_DIRECT_ARG(stmt2[1].second, Int, 5);
    }
    {
        constexpr auto src = U"(6 >= $foo(:bar), $foo(`bar, (4-5)==9))"sv;
        ParserContext context(src);
        const auto func = ComplexArgParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 2u);
        ASSERT_EQ(func.Args[0].TypeData, RawArg::Type::Binary);
        ASSERT_EQ(func.Args[1].TypeData, RawArg::Type::Func);
        {
            const auto& stmt = *func.Args[0].GetVar<RawArg::Type::Binary>();
            CHECK_DIRECT_ARG(stmt.LeftOprend, Int, 6);
            EXPECT_EQ(stmt.Operator, EmbedOps::GreaterEqual);
            ASSERT_EQ(stmt.RightOprend.TypeData, RawArg::Type::Func);
            const auto& fcall = *stmt.RightOprend.GetVar<RawArg::Type::Func>();
            EXPECT_EQ(*fcall.Name, U"foo"sv);
            ASSERT_EQ(fcall.Args.size(), 1u);
            CHECK_VAR_ARG(fcall.Args[0], U"bar", Local);
        }
        {
            const auto& fcall = *func.Args[1].GetVar<RawArg::Type::Func>();
            EXPECT_EQ(*fcall.Name, U"foo"sv);
            ASSERT_EQ(fcall.Args.size(), 2u);
            CHECK_VAR_ARG(fcall.Args[0], U"bar", Root);
            ASSERT_EQ(fcall.Args[1].TypeData, RawArg::Type::Binary);
            const auto& stmt = *fcall.Args[1].GetVar<RawArg::Type::Binary>();
            ASSERT_EQ(stmt.LeftOprend.TypeData, RawArg::Type::Binary);
            {
                const auto& stmt2 = *stmt.LeftOprend.GetVar<RawArg::Type::Binary>();
                CHECK_DIRECT_ARG(stmt2.LeftOprend, Int, 4);
                EXPECT_EQ(stmt2.Operator, EmbedOps::Sub);
                CHECK_DIRECT_ARG(stmt2.RightOprend, Int, 5);
            }
            EXPECT_EQ(stmt.Operator, EmbedOps::Equal);
            CHECK_DIRECT_ARG(stmt.RightOprend, Int, 9);
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
        return xziar::nailang::BlockParser::ParseRawBlock(block, pool);
    };
    {
        constexpr auto src = UR"(
$func(hey);
)"sv;
        const auto block = ParseAll(src);
        ASSERT_EQ(block.Content.size(), 1u);
        const auto [meta, stmt] = block[0];
        EXPECT_EQ(stmt.GetType(), xziar::nailang::BlockContent::Type::FuncCall);
        EXPECT_EQ(meta.size(), 0u);
        const auto& fcall = *std::get<1>(stmt.GetStatement());
        EXPECT_EQ(*fcall.Name, U"func"sv);
        ASSERT_EQ(fcall.Args.size(), 1u);
        CHECK_VAR_ARG(fcall.Args[0], U"hey", Empty);
    }
    {
        constexpr auto src = UR"(
@meta()
hey = 13;
)"sv;
        const auto block = ParseAll(src);
        EXPECT_EQ(block.Content.size(), 1u);
        const auto [meta, stmt] = block[0];
        EXPECT_EQ(stmt.GetType(), xziar::nailang::BlockContent::Type::Assignment);
        ASSERT_EQ(meta.size(), 1u);
        EXPECT_EQ(*meta[0].Name, U"meta"sv);
        EXPECT_EQ(meta[0].Args.size(), 0u);
        const auto& assign = *std::get<0>(stmt.GetStatement());
        EXPECT_EQ(assign.GetVar(), U"hey"sv);
        CHECK_DIRECT_ARG(assign.Statement, Int, 13);
    }
    {
        constexpr auto src = UR"(
x.y = z;
a[0][1].b[2] = c;
)"sv;
        const auto block = ParseAll(src);
        ASSERT_EQ(block.Content.size(), 2u);
        {
            const auto [meta, stmt] = block[0];
            EXPECT_EQ(stmt.GetType(), xziar::nailang::BlockContent::Type::Assignment);
            ASSERT_EQ(meta.size(), 0u);
            const auto& assign = *std::get<0>(stmt.GetStatement());
            EXPECT_EQ(assign.GetVar(), U"x"sv);
            ASSERT_EQ(assign.Queries.Size(), 1u);
            const auto [qtype, qarg] = assign.Queries[0];
            EXPECT_EQ(qtype, SubQuery::QueryType::Sub);
            CHECK_DIRECT_ARG(qarg, Str, U"y"sv);
            CHECK_VAR_ARG(assign.Statement, U"z"sv, Empty);
        }
        {
            const auto [meta, stmt] = block[1];
            EXPECT_EQ(stmt.GetType(), xziar::nailang::BlockContent::Type::Assignment);
            ASSERT_EQ(meta.size(), 0u);
            const auto& assign = *std::get<0>(stmt.GetStatement());
            EXPECT_EQ(assign.GetVar(), U"a"sv);
            ASSERT_EQ(assign.Queries.Size(), 4u);
            {
                const auto [qtype, qarg] = assign.Queries[0];
                EXPECT_EQ(qtype, SubQuery::QueryType::Index);
                CHECK_DIRECT_ARG(qarg, Int, 0);
            }
            {
                const auto [qtype, qarg] = assign.Queries[1];
                EXPECT_EQ(qtype, SubQuery::QueryType::Index);
                CHECK_DIRECT_ARG(qarg, Int, 1);
            }
            {
                const auto [qtype, qarg] = assign.Queries[2];
                EXPECT_EQ(qtype, SubQuery::QueryType::Sub);
                CHECK_DIRECT_ARG(qarg, Str, U"b"sv);
            }
            {
                const auto [qtype, qarg] = assign.Queries[3];
                EXPECT_EQ(qtype, SubQuery::QueryType::Index);
                CHECK_DIRECT_ARG(qarg, Int, 2);
            }
            CHECK_VAR_ARG(assign.Statement, U"c"sv, Empty);
        }
    }
    {
        constexpr auto src = UR"(
@meta()
#Block("13")
{
abc = 0u;
}
)"sv;
        const auto block = ParseAll(src);
        ASSERT_EQ(block.Content.size(), 1u);
        const auto [meta, stmt] = block[0];
        EXPECT_EQ(stmt.GetType(), xziar::nailang::BlockContent::Type::Block);
        ASSERT_EQ(meta.size(), 1u);
        EXPECT_EQ(*meta[0].Name, U"meta"sv);
        EXPECT_EQ(meta[0].Args.size(), 0u);
        const auto& blk = *std::get<3>(stmt.GetStatement());
        EXPECT_EQ(blk.Name, U"13"sv);
        ASSERT_EQ(blk.Content.size(), 1u);
        {
            const auto [meta_, stmt_] = blk[0];
            EXPECT_EQ(stmt_.GetType(), xziar::nailang::BlockContent::Type::Assignment);
            EXPECT_EQ(meta_.size(), 0u);
            const auto& assign = *std::get<0>(stmt_.GetStatement());
            EXPECT_EQ(assign.GetVar(), U"abc"sv);
            CHECK_DIRECT_ARG(assign.Statement, Uint, 0u);
        }
    }
    {
        constexpr auto src = UR"(
hey /= 9u+1u;
$func(hey);
@meta()
#Raw.Main("block")
{
empty
}
)"sv;
        const auto block = ParseAll(src);
        ASSERT_EQ(block.Content.size(), 3u);
        {
            const auto [meta, stmt] = block[0];
            EXPECT_EQ(stmt.GetType(), xziar::nailang::BlockContent::Type::Assignment);
            EXPECT_EQ(meta.size(), 0u);
            const auto& assign = *std::get<0>(stmt.GetStatement());
            EXPECT_EQ(assign.GetVar(), U"hey"sv);
            EXPECT_EQ(assign.Check, Assignment::NilCheck::ReqNotNull);
            EXPECT_EQ(assign.Statement.TypeData, RawArg::Type::Binary);
            {
                const auto& stmt_ = *assign.Statement.GetVar<RawArg::Type::Binary>();
                CHECK_VAR_ARG(stmt_.LeftOprend, U"hey", Empty);
                EXPECT_EQ(stmt_.Operator, EmbedOps::Div);
                EXPECT_EQ(stmt_.RightOprend.TypeData, RawArg::Type::Binary);
                {
                    const auto& stmt2 = *stmt_.RightOprend.GetVar<RawArg::Type::Binary>();
                    CHECK_DIRECT_ARG(stmt2.LeftOprend, Uint, 9u);
                    EXPECT_EQ(stmt2.Operator, EmbedOps::Add);
                    CHECK_DIRECT_ARG(stmt2.RightOprend, Uint, 1u);
                }
            }
        }
        {
            const auto [meta, stmt] = block[1];
            EXPECT_EQ(stmt.GetType(), xziar::nailang::BlockContent::Type::FuncCall);
            EXPECT_EQ(meta.size(), 0u);
            const auto& fcall = *std::get<1>(stmt.GetStatement());
            EXPECT_EQ(*fcall.Name, U"func"sv);
            ASSERT_EQ(fcall.Args.size(), 1u);
            CHECK_VAR_ARG(fcall.Args[0], U"hey", Empty);
        }
        {
            const auto [meta, stmt] = block[2];
            EXPECT_EQ(stmt.GetType(), xziar::nailang::BlockContent::Type::RawBlock);
            ASSERT_EQ(meta.size(), 1u);
            EXPECT_EQ(*meta[0].Name, U"meta"sv);
            EXPECT_EQ(meta[0].Args.size(), 0u);
            const auto& blk = *std::get<2>(stmt.GetStatement());
            EXPECT_EQ(ReplaceNewLine(blk.Source), U"empty\n"sv);
        }
    }
}


TEST(NailangParser, ParseBlock)
{
    xziar::nailang::MemoryPool pool;
    {
        constexpr auto src = U"#Raw.Main(\"Hello\"){\r\n}"sv;
        ParserContext context(src);
        RawBlockParser Parser(pool, context);
        const auto block = Parser.GetNextRawBlock();
        EXPECT_EQ(block.Type, U"Main"sv);
        EXPECT_EQ(block.Name, U"Hello"sv);
        EXPECT_EQ(block.Source, U""sv);
        EXPECT_EQ(block.Position.first,  1u);
        EXPECT_EQ(block.Position.second, 0u);
    }

    {
        constexpr auto src = U"#Raw.Main(\"Hello\"){\r\n}\r\n#Raw.Main(\"Hello\"){\r\n}"sv;
        ParserContext context(src);
        RawBlockParser Parser(pool, context);

        const auto block0 = Parser.GetNextRawBlock();
        EXPECT_EQ(block0.Type, U"Main"sv);
        EXPECT_EQ(block0.Name, U"Hello"sv);
        EXPECT_EQ(block0.Source, U""sv);
        EXPECT_EQ(block0.Position.first,  1u);
        EXPECT_EQ(block0.Position.second, 0u);

        const auto block1 = Parser.GetNextRawBlock();
        EXPECT_EQ(block1.Type, U"Main"sv);
        EXPECT_EQ(block1.Name, U"Hello"sv);
        EXPECT_EQ(block1.Source, U""sv);
        EXPECT_EQ(block1.Position.first,  3u);
        EXPECT_EQ(block1.Position.second, 0u);
    }
    {
        constexpr auto src = 
            UR"(
#Raw.Main("Hello")
{
Here
}

#Raw.Main("Hello")
{===+++
{Here}
===+++}
)"sv;
        ParserContext context(src);
        RawBlockParser Parser(pool, context);

        const auto block0 = Parser.GetNextRawBlock();
        EXPECT_EQ(block0.Type, U"Main"sv);
        EXPECT_EQ(block0.Name, U"Hello"sv);
        EXPECT_EQ(ReplaceNewLine(block0.Source), U"Here\n"sv);
        EXPECT_EQ(block0.Position.first,  2u);
        EXPECT_EQ(block0.Position.second, 0u);

        const auto block1 = Parser.GetNextRawBlock();
        EXPECT_EQ(block1.Type, U"Main"sv);
        EXPECT_EQ(block1.Name, U"Hello"sv);
        EXPECT_EQ(ReplaceNewLine(block1.Source), U"{Here}\n"sv);
        EXPECT_EQ(block1.Position.first,  7u);
        EXPECT_EQ(block1.Position.second, 0u);
    }
}

TEST(NailangParser, ParseMetaBlock)
{
    xziar::nailang::MemoryPool pool;
    {
        constexpr auto src =
            UR"(
@func()
#Raw.Main("Hello")
{
Here
}
)"sv;
        ParserContext context(src);
        RawBlockParser Parser(pool, context);

        const auto block = Parser.GetNextRawBlock();
        EXPECT_EQ(block.Type, U"Main"sv);
        EXPECT_EQ(block.Name, U"Hello"sv);
        EXPECT_EQ(ReplaceNewLine(block.Source), U"Here\n"sv);
        EXPECT_EQ(block.Position.first,  3u);
        EXPECT_EQ(block.Position.second, 0u);
        ASSERT_EQ(block.MetaFunctions.size(), 1u);
        const auto& meta = block.MetaFunctions[0];
        EXPECT_EQ(*meta.Name, U"func"sv);
        EXPECT_EQ(meta.Args.size(), 0u);
    }
    {
        constexpr auto src =
            UR"(
@func(1)
@func(abc, 0xff)
#Raw.Main("Hello")
{
Here
}
#Raw.Main("Hello")
{
Here
}
)"sv;
        ParserContext context(src);
        RawBlockParser Parser(pool, context);

        const auto block = Parser.GetNextRawBlock();
        EXPECT_EQ(block.Type, U"Main"sv);
        EXPECT_EQ(block.Name, U"Hello"sv);
        EXPECT_EQ(ReplaceNewLine(block.Source), U"Here\n"sv);
        EXPECT_EQ(block.Position.first,  4u);
        EXPECT_EQ(block.Position.second, 0u);
        ASSERT_EQ(block.MetaFunctions.size(), 2u);
        {
            const auto& meta = block.MetaFunctions[0];
            EXPECT_EQ(*meta.Name, U"func"sv);
            ASSERT_EQ(meta.Args.size(), 1u);
            CHECK_DIRECT_ARG(meta.Args[0], Int, 1);
        }
        {
            const auto& meta = block.MetaFunctions[1];
            EXPECT_EQ(*meta.Name, U"func"sv);
            EXPECT_EQ(meta.Args.size(), 2u);
            CHECK_VAR_ARG(meta.Args[0], U"abc", Empty);
            CHECK_DIRECT_ARG(meta.Args[1], Uint, 0xffu);
        }
        const auto block_ = Parser.GetNextRawBlock();
        EXPECT_EQ(block_.Type, U"Main"sv);
        EXPECT_EQ(block_.Name, U"Hello"sv);
        EXPECT_EQ(ReplaceNewLine(block_.Source), U"Here\n"sv);
        EXPECT_EQ(block_.Position.first,  8u);
        EXPECT_EQ(block_.Position.second, 0u);
        EXPECT_EQ(block_.MetaFunctions.size(), 0u);
    }
}


class Replacer : public xziar::nailang::ReplaceEngine
{
public:
    std::vector<std::u32string_view> Vars;
    std::vector<std::pair<std::u32string_view, std::vector<std::u32string_view>>> Funcs;
    std::u32string_view Target;
    Replacer(std::u32string_view target) : Target(target) {}
    ~Replacer() override {}
    void OnReplaceVariable(std::u32string& output, void*, std::u32string_view var) override
    {
        Vars.push_back(var);
        output.append(Target);
    }
    void OnReplaceFunction(std::u32string& output, void*, std::u32string_view func, common::span<const std::u32string_view> args) override
    {
        std::vector<std::u32string_view> arg(args.begin(), args.end());
        Funcs.emplace_back(func, std::move(arg));
        output.append(Target);
    }
    using ReplaceEngine::ProcessVariable;
    using ReplaceEngine::ProcessFunction;
};
TEST(NailangParser, ReplaceEngine)
{
    {
        Replacer replacer(U"l"sv);
        const auto result = replacer.ProcessVariable(U"He{1}{2}o Wor{3}d"sv, U"{"sv, U"}"sv);
        EXPECT_EQ(result, U"Hello World"sv);
        EXPECT_THAT(replacer.Vars, testing::ElementsAre(U"1"sv, U"2"sv, U"3"sv));
    }
    {
        Replacer replacer(U"l"sv);
        const auto result = replacer.ProcessVariable(U"He$$!{ab}$$!{cd}o Wor$$!{ef}d"sv, U"$$!{"sv, U"}"sv);
        EXPECT_EQ(result, U"Hello World"sv);
        EXPECT_THAT(replacer.Vars, testing::ElementsAre(U"ab"sv, U"cd"sv, U"ef"sv));
    }
    {
        Replacer replacer(U"func()"sv);
        const auto result = replacer.ProcessFunction(U"y = $$!x()"sv, U"$$!"sv, U""sv);
        EXPECT_EQ(result, U"y = func()"sv);
        ASSERT_EQ(replacer.Funcs.size(), 1u);
        EXPECT_EQ(replacer.Funcs[0].first, U"x"sv);
        EXPECT_THAT(replacer.Funcs[0].second, testing::ElementsAre());
    }
    {
        Replacer replacer(U"func()"sv);
        const auto result = replacer.ProcessFunction(UR"(y = $$!xy(""))"sv, U"$$!"sv, U""sv);
        EXPECT_EQ(result, U"y = func()"sv);
        ASSERT_EQ(replacer.Funcs.size(), 1u);
        EXPECT_EQ(replacer.Funcs[0].first, U"xy"sv);
        EXPECT_THAT(replacer.Funcs[0].second, testing::ElementsAre(UR"("")"sv));
    }
    {
        Replacer replacer(U"func()"sv);
        const auto result = replacer.ProcessFunction(UR"(y = $$!xy("1+2     ,3"))"sv, U"$$!"sv, U""sv);
        EXPECT_EQ(result, U"y = func()"sv);
        ASSERT_EQ(replacer.Funcs.size(), 1u);
        EXPECT_EQ(replacer.Funcs[0].first, U"xy"sv);
        EXPECT_THAT(replacer.Funcs[0].second, testing::ElementsAre(UR"("1+2     ,3")"sv));
    }
    {
        Replacer replacer(U"func()"sv);
        const auto result = replacer.ProcessFunction(U"y = $$!xy(1,2)"sv, U"$$!"sv, U""sv);
        EXPECT_EQ(result, U"y = func()"sv);
        ASSERT_EQ(replacer.Funcs.size(), 1u);
        EXPECT_EQ(replacer.Funcs[0].first, U"xy"sv);
        EXPECT_THAT(replacer.Funcs[0].second, testing::ElementsAre(U"1"sv, U"2"sv));
    }
    {
        Replacer replacer(U"func()"sv);
        const auto result = replacer.ProcessFunction(U"y = $$!xy((1), 2)"sv, U"$$!"sv, U""sv);
        EXPECT_EQ(result, U"y = func()"sv);
        ASSERT_EQ(replacer.Funcs.size(), 1u);
        EXPECT_EQ(replacer.Funcs[0].first, U"xy"sv);
        EXPECT_THAT(replacer.Funcs[0].second, testing::ElementsAre(U"(1)"sv, U"2"sv));
    }
    {
        Replacer replacer(U"func()"sv);
        const auto result = replacer.ProcessFunction(UR"kk(y = $$!xy((1+("(2)")), 2))kk"sv, U"$$!"sv, U""sv);
        EXPECT_EQ(result, U"y = func()"sv);
        ASSERT_EQ(replacer.Funcs.size(), 1u);
        EXPECT_EQ(replacer.Funcs[0].first, U"xy"sv);
        EXPECT_THAT(replacer.Funcs[0].second, testing::ElementsAre(UR"kk((1+("(2)")))kk"sv, U"2"sv));
    }
    {
        Replacer replacer(U"func()"sv);
        const auto result = replacer.ProcessFunction(UR"(y = $$!xy( 12, 34 )$ +$$!y(56 + "" / 7)$)"sv, U"$$!"sv, U"$"sv);
        EXPECT_EQ(result, U"y = func() +func()"sv);
        ASSERT_EQ(replacer.Funcs.size(), 2u);
        EXPECT_EQ(replacer.Funcs[0].first, U"xy"sv);
        EXPECT_THAT(replacer.Funcs[0].second, testing::ElementsAre(U"12"sv, U"34"sv));
        EXPECT_EQ(replacer.Funcs[1].first, U"y"sv);
        EXPECT_THAT(replacer.Funcs[1].second, testing::ElementsAre(UR"(56 + "" / 7)"sv));
    }
}

