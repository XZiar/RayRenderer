#include "rely.h"
#include "Nailang/ParserRely.h"
#include "Nailang/BlockParser.h"
#include "Nailang/ArgParser.h"
#include "Nailang/NailangRuntime.h"


using namespace std::string_view_literals;
using common::parser::ParserContext;
using common::parser::ContextReader;
using xziar::nailang::MemoryPool;
using xziar::nailang::BlockParser;
using xziar::nailang::ComplexArgParser;
using xziar::nailang::EmbedOps;
using xziar::nailang::LateBindVar;
using xziar::nailang::BinaryStatement;
using xziar::nailang::UnaryStatement;
using xziar::nailang::FuncCall;
using xziar::nailang::RawArg;
using xziar::nailang::Arg;
using xziar::nailang::NailangRuntimeBase;
using xziar::nailang::EvaluateContext;
using xziar::nailang::EmbedOpEval;

class EvalCtx : public EvaluateContext
{
public:
    EvalCtx()
    {
        ArgMap.insert_or_assign(U"valU64", Arg(uint64_t(1)));
        ArgMap.insert_or_assign(U"valI64", Arg(int64_t(2)));
        ArgMap.insert_or_assign(U"valF64", Arg(3.0));
        ArgMap.insert_or_assign(U"valStr", Arg(U"txt"sv));
    }
};
class NailangRT : public NailangRuntimeBase
{
public:
    NailangRT()
    {
        EvalContext = std::make_shared<EvalCtx>();
    }
    using NailangRuntimeBase::EvaluateFunc;
    using NailangRuntimeBase::EvaluateArg;
};


#define CHECK_ARG(arg, type, val) do                    \
{                                                       \
EXPECT_EQ(arg.TypeData, Arg::InternalType::type);       \
EXPECT_EQ(arg.GetVar<Arg::InternalType::type>(), val);  \
} while(0)                                              \

TEST(NailangRuntime, EvalEmbedOp)
{
    using Type = Arg::InternalType;
#define TEST_BIN(l, r, op, type, ans) do        \
{                                               \
Arg left(l), right(r);                          \
const auto ret = EmbedOpEval::op(left, right);  \
CHECK_ARG(ret, type, ans);                      \
} while(0)                                      \

#define TEST_UN(val, op, type, ans) do      \
{                                           \
Arg arg(val);                               \
const auto ret = EmbedOpEval::op(arg);      \
CHECK_ARG(ret, type, ans);                  \
} while(0)                                  \

    TEST_BIN(uint64_t(1), uint64_t(2), Equal,       Bool, false);
    TEST_BIN(uint64_t(0), uint64_t(0), NotEqual,    Bool, false);
    TEST_BIN(uint64_t(1), uint64_t(2), Add,         Uint, 3);
    TEST_BIN(uint64_t(1), int64_t(-1), Add,         Uint, 0);
    TEST_BIN(uint64_t(1), 0.5,         Sub,         FP,   0.5);
    TEST_BIN(int64_t(5),  int64_t(-1), Mul,         Int,  -5);
    TEST_BIN(uint64_t(3), uint64_t(2), Div,         Uint, 1);
    TEST_BIN(uint64_t(3), 0.5,         Div,         FP,   6);
    TEST_BIN(uint64_t(3), uint64_t(2), Rem,         Uint, 1);
    TEST_BIN(2.5,         2.0,         Rem,         FP,   0.5);
    TEST_BIN(U"abc"sv,    U"ABC"sv,    Add,         U32Str, U"abcABC"sv);
    TEST_BIN(uint64_t(1), uint64_t(2), Less,        Bool, true);
    TEST_BIN(int64_t(-1), uint64_t(2), Less,        Bool, true);
    TEST_BIN(uint64_t(1), int64_t(-2), Less,        Bool, false);
    TEST_BIN(uint64_t(1), int64_t(-2), LessEqual,   Bool, false);
    TEST_BIN(-2.0,        int64_t(-2), GreaterEqual, Bool, true);
    TEST_BIN(-2.0,        U""sv,       And,         Bool, false);
    TEST_BIN(true,        U""sv,       Or,          Bool, true);
    TEST_UN (-2.0,  Not, Bool, false);
    TEST_UN (U""sv, Not, Bool, true);
#undef TEST_BIN
#undef TEST_UN
}


TEST(NailangRuntime, ParseEvalEmbedOp)
{
    MemoryPool pool;
    const auto ParseEval = [&](const std::u32string_view src) 
    {
        ParserContext context(src);
        const auto rawarg = ComplexArgParser::ParseSingleStatement(pool, context);
        NailangRT runtime;
        return runtime.EvaluateArg(*rawarg);
    };
    {
        const auto arg = ParseEval(U"!false;"sv);
        CHECK_ARG(arg, Bool, true);
    }
    {
        const auto arg = ParseEval(U"!3.5;"sv);
        CHECK_ARG(arg, Bool, false);
    }
    {
        const auto arg = ParseEval(U"!\"hero\";"sv);
        CHECK_ARG(arg, Bool, false);
    }
    {
        const auto arg = ParseEval(U"1+2;"sv);
        CHECK_ARG(arg, Int, 3);
    }
    {
        const auto arg = ParseEval(U"4u-4.5;"sv);
        CHECK_ARG(arg, FP, -0.5);
    }
    {
        const auto arg = ParseEval(U"3*-3;"sv);
        CHECK_ARG(arg, Int, -9);
    }
    {
        const auto arg = ParseEval(U"5/6;"sv);
        CHECK_ARG(arg, Int, 0);
    }
    {
        const auto arg = ParseEval(U"7%3;"sv);
        CHECK_ARG(arg, Int, 1);
    }
    {
        const auto arg = ParseEval(U"12.5 % 6;"sv);
        CHECK_ARG(arg, FP, 0.5);
    }
    {
        const auto arg = ParseEval(U"\"abc\" + \"ABC\";"sv);
        CHECK_ARG(arg, U32Str, U"abcABC"sv);
    }
}

