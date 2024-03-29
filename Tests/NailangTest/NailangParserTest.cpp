#include "rely.h"
#include "Nailang/NailangParserRely.h"
#include "Nailang/NailangParser.h"
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/size.hpp>

using namespace std::string_view_literals;
using common::parser::ParserContext;
using common::parser::ContextReader;
using xziar::nailang::NailangParser;
using xziar::nailang::NailangParser;
using xziar::nailang::NailangParser;
using xziar::nailang::EmbedOps;
using xziar::nailang::SubQuery;
using xziar::nailang::LateBindVar;
using xziar::nailang::BinaryExpr;
using xziar::nailang::UnaryExpr;
using xziar::nailang::QueryExpr;
using xziar::nailang::FuncCall;
using xziar::nailang::Expr;
using xziar::nailang::Block;
using xziar::nailang::RawBlock;
using xziar::nailang::NilCheck;
using xziar::nailang::AssignExpr;


testing::AssertionResult CheckExpr(const Expr& arg, const Expr::Type type)
{
    if (arg.TypeData == type)
        return testing::AssertionSuccess();
    else
        return testing::AssertionFailure() << "rawarg is [" << WideToChar(arg.GetTypeName()) << "](" << common::enum_cast(arg.TypeData) << ")";
}

#define CHECK_DIRECT_ARG(target, type, val)             \
do                                                      \
{                                                       \
    ASSERT_TRUE(CheckExpr(target, Expr::Type::type));   \
    EXPECT_EQ(target.GetVar<Expr::Type::type>(), val);  \
} while(0)

#define CHECK_VAR_ARG(target, val, info)                    \
do                                                          \
{                                                           \
    ASSERT_TRUE(CheckExpr(target, Expr::Type::Var));        \
    const auto& _var = target.GetVar<Expr::Type::Var>();    \
    EXPECT_EQ(_var.Name, val);                              \
    EXPECT_EQ(_var.Info, LateBindVar::VarInfo::info);       \
} while(0)


#define CHECK_QUERY_SUBFIELD(r, x, i, name) CHECK_DIRECT_ARG(x.QueryPtr[i], Str, name);
#define CHECK_QUERY_SUBFIELDS(x, names) BOOST_PP_SEQ_FOR_EACH_I(CHECK_QUERY_SUBFIELD, x, names)
#define CHECK_FIELD_QUERY(target, var, info, ...)                       \
do                                                                      \
{                                                                       \
    ASSERT_EQ(target.TypeData, Expr::Type::Query);                      \
    const auto& query = *target.GetVar<Expr::Type::Query>();            \
    ASSERT_EQ(query.TypeData, QueryExpr::QueryType::Sub);               \
    constexpr size_t qcnt = BOOST_PP_VARIADIC_SIZE(__VA_ARGS__);        \
    ASSERT_EQ(query.Count, qcnt);                                       \
    CHECK_VAR_ARG(query.Target, var, info);                             \
    CHECK_QUERY_SUBFIELDS(query, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
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
        const auto dst = NailangParser::ProcessString(src, pool);
        EXPECT_EQ(dst.GetVar<Expr::Type::Str>(), U"Hello"sv);
    }
    {
        constexpr auto src = U"Hello\\tWorld"sv;
        const auto dst = NailangParser::ProcessString(src, pool);
        EXPECT_EQ(dst.GetVar<Expr::Type::Str>(), U"Hello\tWorld"sv);
    }
    {
        constexpr auto src = U"Hello\tWorld\\0"sv;
        const auto dst = NailangParser::ProcessString(src, pool);
        EXPECT_EQ(dst.GetVar<Expr::Type::Str>(), U"Hello\tWorld\0"sv);
    }
}

TEST(NailangParser, ParseExpr)
{
    xziar::nailang::MemoryPool pool;
    {
        constexpr auto src = U""sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        EXPECT_FALSE(expr);
    }
    {
        constexpr auto src = U"()"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        EXPECT_FALSE(expr);
    }
    {
        constexpr auto src = U"(((())))"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        EXPECT_FALSE(expr);
    }
    {
        constexpr auto src = U"\"Hello\""sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        CHECK_DIRECT_ARG(expr, Str, U"Hello"sv);
    }
    {
        constexpr auto src = UR"("Hello\t\"World\"")"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        CHECK_DIRECT_ARG(expr, Str, U"Hello\t\"World\""sv);
    }
    {
        constexpr auto src = U"val.xxx.y.len"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Query);
        const auto& query = *expr.GetVar<Expr::Type::Query>();
        ASSERT_EQ(query.TypeData, QueryExpr::QueryType::Sub);
        ASSERT_EQ(query.Count, 3u);
        CHECK_VAR_ARG(query.Target, U"val"sv, Empty);
        CHECK_DIRECT_ARG(query.QueryPtr[0], Str, U"xxx"sv);
        CHECK_DIRECT_ARG(query.QueryPtr[1], Str, U"y"sv);
        CHECK_DIRECT_ARG(query.QueryPtr[2], Str, U"len"sv);
    }
    {
        constexpr auto src = U"(val[5])"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Query);
        const auto& query = *expr.GetVar<Expr::Type::Query>();
        ASSERT_EQ(query.TypeData, QueryExpr::QueryType::Index);
        ASSERT_EQ(query.Count, 1u);
        CHECK_VAR_ARG(query.Target, U"val"sv, Empty);
        CHECK_DIRECT_ARG(query.QueryPtr[0], Int, 5);
    }
    {
        constexpr auto src = U"1u + 2"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Binary);
        const auto& stmt = *expr.GetVar<Expr::Type::Binary>();
        CHECK_DIRECT_ARG(stmt.LeftOperand, Uint, 1u);
        EXPECT_EQ(stmt.Operator, EmbedOps::Add);
        CHECK_DIRECT_ARG(stmt.RightOperand, Int, 2);
    }
    {
        constexpr auto src = U"here??not"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Binary);
        const auto& stmt = *expr.GetVar<Expr::Type::Binary>();
        CHECK_VAR_ARG(stmt.LeftOperand, U"here"sv, Empty);
        EXPECT_EQ(stmt.Operator, EmbedOps::ValueOr);
        CHECK_VAR_ARG(stmt.RightOperand, U"not"sv, Empty);
    }
    {
        constexpr auto src = U"?here"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Unary);
        const auto& stmt = *expr.GetVar<Expr::Type::Unary>();
        EXPECT_EQ(stmt.Operator, EmbedOps::CheckExist);
        CHECK_VAR_ARG(stmt.Operand, U"here"sv, Empty);
    }
    {
        constexpr auto src = U"1?2:3"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Ternary);
        const auto& stmt = *expr.GetVar<Expr::Type::Ternary>();
        CHECK_DIRECT_ARG(stmt.Condition,   Int, 1);
        CHECK_DIRECT_ARG(stmt.LeftOperand,  Int, 2);
        CHECK_DIRECT_ARG(stmt.RightOperand, Int, 3);
    }
    {
        constexpr auto src = U"1?:here:3"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Ternary);
        const auto& stmt = *expr.GetVar<Expr::Type::Ternary>();
        CHECK_DIRECT_ARG(stmt.Condition,   Int, 1);
        CHECK_VAR_ARG(stmt.LeftOperand, U"here"sv, Local);
        CHECK_DIRECT_ARG(stmt.RightOperand, Int, 3);
    }
    {
        constexpr auto src = U"val[x+y]+ 12.8"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Binary);
        const auto& stmt = *expr.GetVar<Expr::Type::Binary>();
        EXPECT_EQ(stmt.LeftOperand.TypeData, Expr::Type::Query);
        EXPECT_EQ(stmt.Operator, EmbedOps::Add);
        CHECK_DIRECT_ARG(stmt.RightOperand, FP, 12.8);

        const auto& stmt1 = *stmt.LeftOperand.GetVar<Expr::Type::Query>();
        ASSERT_EQ(stmt1.TypeData, QueryExpr::QueryType::Index);
        ASSERT_EQ(stmt1.Count, 1u);
        CHECK_VAR_ARG(stmt1.Target, U"val"sv, Empty);
        ASSERT_EQ(stmt1.QueryPtr[0].TypeData, Expr::Type::Binary);

        const auto& stmt2 = *stmt1.QueryPtr[0].GetVar<Expr::Type::Binary>();
        CHECK_VAR_ARG(stmt2.LeftOperand, U"x"sv, Empty);
        EXPECT_EQ(stmt2.Operator, EmbedOps::Add);
        CHECK_VAR_ARG(stmt2.RightOperand, U"y"sv, Empty);
    }
    {
        constexpr auto src = U"!val[x[3+4][5]]"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Unary);
        const auto& stmt = *expr.GetVar<Expr::Type::Unary>();
        EXPECT_EQ(stmt.Operator, EmbedOps::Not);
        ASSERT_EQ(stmt.Operand.TypeData, Expr::Type::Query);

        const auto& stmt1 = *stmt.Operand.GetVar<Expr::Type::Query>();
        ASSERT_EQ(stmt1.TypeData, QueryExpr::QueryType::Index);
        ASSERT_EQ(stmt1.Count, 1u);
        CHECK_VAR_ARG(stmt1.Target, U"val"sv, Empty);
        ASSERT_EQ(stmt1.QueryPtr[0].TypeData, Expr::Type::Query);

        const auto& stmt2 = *stmt1.QueryPtr[0].GetVar<Expr::Type::Query>();
        ASSERT_EQ(stmt2.TypeData, QueryExpr::QueryType::Index);
        ASSERT_EQ(stmt2.Count, 2u);
        CHECK_VAR_ARG(stmt2.Target, U"x"sv, Empty);
        ASSERT_EQ(stmt2.QueryPtr[0].TypeData, Expr::Type::Binary);
        CHECK_DIRECT_ARG(stmt2.QueryPtr[1], Int, 5);

        const auto& stmt3 = *stmt2.QueryPtr[0].GetVar<Expr::Type::Binary>();
        CHECK_DIRECT_ARG(stmt3.LeftOperand, Int, 3);
        EXPECT_EQ(stmt3.Operator, EmbedOps::Add);
        CHECK_DIRECT_ARG(stmt3.RightOperand, Int, 4);
    }
    {
        constexpr auto src = U"!?here"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Unary);
        const auto& stmt = *expr.GetVar<Expr::Type::Unary>();
        EXPECT_EQ(stmt.Operator, EmbedOps::Not);
        ASSERT_EQ(stmt.Operand.TypeData, Expr::Type::Unary);
        const auto& stmt1 = *stmt.Operand.GetVar<Expr::Type::Unary>();
        EXPECT_EQ(stmt1.Operator, EmbedOps::CheckExist);
        CHECK_VAR_ARG(stmt1.Operand, U"here"sv, Empty);
    }
    {
        constexpr auto src = U"1 + 2 * 3"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Binary);
        const auto& stmt = *expr.GetVar<Expr::Type::Binary>();
        EXPECT_EQ(stmt.Operator, EmbedOps::Mul);
        CHECK_DIRECT_ARG(stmt.RightOperand, Int, 3);
        ASSERT_EQ(stmt.LeftOperand.TypeData, Expr::Type::Binary);
        const auto& stmt1 = *stmt.LeftOperand.GetVar<Expr::Type::Binary>();
        CHECK_DIRECT_ARG(stmt1.LeftOperand, Int, 1);
        EXPECT_EQ(stmt1.Operator, EmbedOps::Add);
        CHECK_DIRECT_ARG(stmt1.RightOperand, Int, 2);
    }
    {
        constexpr auto src = U"1 + 2 ? !a.x ?? a[3] : a + 4"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Binary);
        const auto& stmt = *expr.GetVar<Expr::Type::Binary>();
        ASSERT_EQ(stmt.LeftOperand.TypeData, Expr::Type::Ternary);
        CHECK_DIRECT_ARG(stmt.RightOperand, Int, 4);

        const auto& stmt1 = *stmt.LeftOperand.GetVar<Expr::Type::Ternary>();
        ASSERT_EQ(stmt1.Condition.TypeData, Expr::Type::Binary);
        ASSERT_EQ(stmt1.LeftOperand.TypeData, Expr::Type::Binary);
        CHECK_VAR_ARG(stmt1.RightOperand, U"a"sv, Empty);

        const auto& stmt10 = *stmt1.Condition.GetVar<Expr::Type::Binary>();
        CHECK_DIRECT_ARG(stmt10.LeftOperand, Int, 1);
        EXPECT_EQ(stmt10.Operator, EmbedOps::Add);
        CHECK_DIRECT_ARG(stmt10.RightOperand, Int, 2);

        const auto& stmt11 = *stmt1.LeftOperand.GetVar<Expr::Type::Binary>();
        ASSERT_EQ(stmt11.LeftOperand.TypeData, Expr::Type::Unary);
        EXPECT_EQ(stmt11.Operator, EmbedOps::ValueOr);
        ASSERT_EQ(stmt11.RightOperand.TypeData, Expr::Type::Query);

        const auto& stmt110 = *stmt11.LeftOperand.GetVar<Expr::Type::Unary>();
        EXPECT_EQ(stmt110.Operator, EmbedOps::Not);
        CHECK_FIELD_QUERY(stmt110.Operand, U"a"sv, Empty, U"x"sv);

        const auto& stmt111 = *stmt11.RightOperand.GetVar<Expr::Type::Query>();
        ASSERT_EQ(stmt111.TypeData, QueryExpr::QueryType::Index);
        ASSERT_EQ(stmt111.Count, 1u);
        CHECK_DIRECT_ARG(stmt111.QueryPtr[0], Int, 3);
        CHECK_VAR_ARG(stmt111.Target, U"a", Empty);
    }
    {
        constexpr auto src = U"?here.xyz"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Unary);
        const auto& stmt = *expr.GetVar<Expr::Type::Unary>();
        EXPECT_EQ(stmt.Operator, EmbedOps::CheckExist);
        CHECK_FIELD_QUERY(stmt.Operand, U"here"sv, Empty, U"xyz"sv);
    }
    {
        constexpr auto src = U"?here.xyz[3]"sv;
        ParserContext context(src);
        const auto expr = NailangParser::ParseSingleExpr(pool, context, ""sv, U""sv);
        ASSERT_EQ(expr.TypeData, Expr::Type::Unary);
        const auto& stmt = *expr.GetVar<Expr::Type::Unary>();
        EXPECT_EQ(stmt.Operator, EmbedOps::CheckExist);
        ASSERT_EQ(stmt.Operand.TypeData, Expr::Type::Query);
        const auto& stmt1 = *stmt.Operand.GetVar<Expr::Type::Query>();
        ASSERT_EQ(stmt1.TypeData, QueryExpr::QueryType::Index);
        ASSERT_EQ(stmt1.Count, 1u);
        CHECK_DIRECT_ARG(stmt1.QueryPtr[0], Int, 3);
        CHECK_FIELD_QUERY(stmt1.Target, U"here"sv, Empty, U"xyz"sv);
    }
}
TEST(NailangParser, ParseFuncBody)
{
    xziar::nailang::MemoryPool pool;
    {
        constexpr auto src = U"(123, -456)"sv;
        ParserContext context(src);
        const auto func = NailangParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 2u);
        CHECK_DIRECT_ARG(func.Args[0], Int, 123);
        CHECK_DIRECT_ARG(func.Args[1], Int, -456);
    }
    {
        constexpr auto src = U"((123u), v._456)"sv;
        ParserContext context(src);
        const auto func = NailangParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 2u);
        CHECK_DIRECT_ARG(func.Args[0], Uint, 123u);
        CHECK_FIELD_QUERY(func.Args[1], U"v"sv, Empty, U"_456"sv);
        //CHECK_VAR_ARG(func.Args[1], U"v.456"sv, Empty);
    }
    {
        constexpr auto src = U"(!false, 3.5 + var)"sv;
        ParserContext context(src);
        const auto func = NailangParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 2u);
        ASSERT_EQ(func.Args[0].TypeData, Expr::Type::Unary);
        ASSERT_EQ(func.Args[1].TypeData, Expr::Type::Binary);
        {
            const auto& stmt = *func.Args[0].GetVar<Expr::Type::Unary>();
            EXPECT_EQ(stmt.Operator, EmbedOps::Not);
            CHECK_DIRECT_ARG(stmt.Operand, Bool, false);
        }
        {
            const auto& stmt = *func.Args[1].GetVar<Expr::Type::Binary>();
            CHECK_DIRECT_ARG(stmt.LeftOperand, FP, 3.5);
            EXPECT_EQ(stmt.Operator, EmbedOps::Add);
            CHECK_VAR_ARG(stmt.RightOperand, U"var"sv, Empty);
        }
    }
    {
        constexpr auto src = U"(6 >= $foo(:bar), $foo(`bar, (4-5)==9))"sv;
        ParserContext context(src);
        const auto func = NailangParser::ParseFuncBody(U"func"sv, pool, context);
        EXPECT_EQ(*func.Name, U"func"sv);
        ASSERT_EQ(func.Args.size(), 2u);
        ASSERT_EQ(func.Args[0].TypeData, Expr::Type::Binary);
        ASSERT_EQ(func.Args[1].TypeData, Expr::Type::Func);
        {
            const auto& stmt = *func.Args[0].GetVar<Expr::Type::Binary>();
            CHECK_DIRECT_ARG(stmt.LeftOperand, Int, 6);
            EXPECT_EQ(stmt.Operator, EmbedOps::GreaterEqual);
            ASSERT_EQ(stmt.RightOperand.TypeData, Expr::Type::Func);
            const auto& fcall = *stmt.RightOperand.GetVar<Expr::Type::Func>();
            EXPECT_EQ(*fcall.Name, U"foo"sv);
            ASSERT_EQ(fcall.Args.size(), 1u);
            CHECK_VAR_ARG(fcall.Args[0], U"bar", Local);
        }
        {
            const auto& fcall = *func.Args[1].GetVar<Expr::Type::Func>();
            EXPECT_EQ(*fcall.Name, U"foo"sv);
            ASSERT_EQ(fcall.Args.size(), 2u);
            CHECK_VAR_ARG(fcall.Args[0], U"bar", Root);
            ASSERT_EQ(fcall.Args[1].TypeData, Expr::Type::Binary);
            const auto& stmt = *fcall.Args[1].GetVar<Expr::Type::Binary>();
            ASSERT_EQ(stmt.LeftOperand.TypeData, Expr::Type::Binary);
            {
                const auto& stmt2 = *stmt.LeftOperand.GetVar<Expr::Type::Binary>();
                CHECK_DIRECT_ARG(stmt2.LeftOperand, Int, 4);
                EXPECT_EQ(stmt2.Operator, EmbedOps::Sub);
                CHECK_DIRECT_ARG(stmt2.RightOperand, Int, 5);
            }
            EXPECT_EQ(stmt.Operator, EmbedOps::Equal);
            CHECK_DIRECT_ARG(stmt.RightOperand, Int, 9);
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
        return xziar::nailang::NailangParser::ParseRawBlock(block, pool);
    };
    {
        constexpr auto src = UR"(
$func(hey);
)"sv;
        const auto block = ParseAll(src);
        ASSERT_EQ(block.Content.size(), 1u);
        const auto [meta, stmt] = block[0];
        EXPECT_EQ(stmt.TypeData, xziar::nailang::Statement::Type::FuncCall);
        EXPECT_EQ(meta.size(), 0u);
        const auto& fcall = *stmt.Get<FuncCall>();
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
        EXPECT_EQ(stmt.TypeData, xziar::nailang::Statement::Type::Assign);
        ASSERT_EQ(meta.size(), 1u);
        EXPECT_EQ(*meta[0].Name, U"meta"sv);
        EXPECT_EQ(meta[0].Args.size(), 0u);
        const auto& assign = *stmt.Get<AssignExpr>();
        CHECK_VAR_ARG(assign.Target, U"hey", Empty);
        CHECK_DIRECT_ARG(assign.Statement, Int, 13);
    }
    {
        constexpr auto src = UR"(
x.y = z;
a[0][1].b[2] = ~c;
)"sv;
        const auto block = ParseAll(src);
        ASSERT_EQ(block.Content.size(), 2u);
        {
            const auto [meta, stmt] = block[0];
            EXPECT_EQ(stmt.TypeData, xziar::nailang::Statement::Type::Assign);
            ASSERT_EQ(meta.size(), 0u);
            const auto& assign = *stmt.Get<AssignExpr>();
            CHECK_FIELD_QUERY(assign.Target, U"x"sv, Empty, U"y"sv);
            CHECK_VAR_ARG(assign.Statement, U"z"sv, Empty);
        }
        {
            const auto [meta, stmt] = block[1];
            EXPECT_EQ(stmt.TypeData, xziar::nailang::Statement::Type::Assign);
            ASSERT_EQ(meta.size(), 0u);
            const auto& assign = *stmt.Get<AssignExpr>();
            ASSERT_EQ(assign.Target.TypeData, Expr::Type::Query);
            const auto& query = *assign.Target.GetVar<Expr::Type::Query>();
            ASSERT_EQ(query.TypeData, QueryExpr::QueryType::Index);
            ASSERT_EQ(query.Count, 1u);
            CHECK_DIRECT_ARG(query.QueryPtr[0], Int, 2);
            ASSERT_EQ(query.Target.TypeData, Expr::Type::Query);

            const auto& query1 = *query.Target.GetVar<Expr::Type::Query>();
            ASSERT_EQ(query1.TypeData, QueryExpr::QueryType::Sub);
            ASSERT_EQ(query1.Count, 1u);
            CHECK_DIRECT_ARG(query1.QueryPtr[0], Str, U"b");
            ASSERT_EQ(query1.Target.TypeData, Expr::Type::Query);

            const auto& query2 = *query1.Target.GetVar<Expr::Type::Query>();
            ASSERT_EQ(query2.TypeData, QueryExpr::QueryType::Index);
            ASSERT_EQ(query2.Count, 2u);
            CHECK_DIRECT_ARG(query2.QueryPtr[0], Int, 0);
            CHECK_DIRECT_ARG(query2.QueryPtr[1], Int, 1);
            CHECK_VAR_ARG(query2.Target, U"a", Empty);

            ASSERT_EQ(assign.Statement.TypeData, Expr::Type::Unary);
            const auto& stmt1 = *assign.Statement.GetVar<Expr::Type::Unary>();
            CHECK_VAR_ARG(stmt1.Operand, U"c"sv, Empty);
            ASSERT_EQ(stmt1.Operator, EmbedOps::BitNot);
        }
    }
    {
        constexpr auto src = UR"(
@meta()
#Block("13")
{
abc = 0u | 5;
}
)"sv;
        const auto block = ParseAll(src);
        ASSERT_EQ(block.Content.size(), 1u);
        const auto [meta, stmt] = block[0];
        EXPECT_EQ(stmt.TypeData, xziar::nailang::Statement::Type::Block);
        ASSERT_EQ(meta.size(), 1u);
        EXPECT_EQ(*meta[0].Name, U"meta"sv);
        EXPECT_EQ(meta[0].Args.size(), 0u);
        const auto& blk = *stmt.Get<Block>();
        EXPECT_EQ(blk.Name, U"13"sv);
        ASSERT_EQ(blk.Content.size(), 1u);
        {
            const auto [meta1, stmt1] = blk[0];
            EXPECT_EQ(stmt1.TypeData, xziar::nailang::Statement::Type::Assign);
            EXPECT_EQ(meta1.size(), 0u);
            const auto& assign = *stmt1.Get<AssignExpr>();
            CHECK_VAR_ARG(assign.Target, U"abc", Empty);

            ASSERT_EQ(assign.Statement.TypeData, Expr::Type::Binary);
            const auto& stmt2 = *assign.Statement.GetVar<Expr::Type::Binary>();
            CHECK_DIRECT_ARG(stmt2.LeftOperand, Uint, 0u);
            ASSERT_EQ(stmt2.Operator, EmbedOps::BitOr);
            CHECK_DIRECT_ARG(stmt2.RightOperand, Int, 5);
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
            EXPECT_EQ(stmt.TypeData, xziar::nailang::Statement::Type::Assign);
            EXPECT_EQ(meta.size(), 0u);
            const auto& assign = *stmt.Get<AssignExpr>();
            CHECK_VAR_ARG(assign.Target, U"hey", Empty);
            EXPECT_TRUE(assign.IsSelfAssign);
            EXPECT_EQ(assign.GetSelfAssignOp(), EmbedOps::Div);
            const auto check = assign.GetCheck();
            EXPECT_EQ(check.WhenNull(), NilCheck::Behavior::Throw);
            EXPECT_EQ(check.WhenNotNull(), NilCheck::Behavior::Pass);
            EXPECT_EQ(assign.Statement.TypeData, Expr::Type::Binary);
            {
                const auto& stmt_ = *assign.Statement.GetVar<Expr::Type::Binary>();
                CHECK_DIRECT_ARG(stmt_.LeftOperand, Uint, 9u);
                EXPECT_EQ(stmt_.Operator, EmbedOps::Add);
                CHECK_DIRECT_ARG(stmt_.RightOperand, Uint, 1u);
            }
        }
        {
            const auto [meta, stmt] = block[1];
            EXPECT_EQ(stmt.TypeData, xziar::nailang::Statement::Type::FuncCall);
            EXPECT_EQ(meta.size(), 0u);
            const auto& fcall = *stmt.Get<FuncCall>();
            EXPECT_EQ(*fcall.Name, U"func"sv);
            ASSERT_EQ(fcall.Args.size(), 1u);
            CHECK_VAR_ARG(fcall.Args[0], U"hey", Empty);
        }
        {
            const auto [meta, stmt] = block[2];
            EXPECT_EQ(stmt.TypeData, xziar::nailang::Statement::Type::RawBlock);
            ASSERT_EQ(meta.size(), 1u);
            EXPECT_EQ(*meta[0].Name, U"meta"sv);
            EXPECT_EQ(meta[0].Args.size(), 0u);
            const auto& blk = *stmt.Get<RawBlock>();
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
        NailangParser Parser(pool, context);
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
        NailangParser Parser(pool, context);

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
        NailangParser Parser(pool, context);

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
        NailangParser Parser(pool, context);

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
        NailangParser Parser(pool, context);

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

