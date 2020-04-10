#include "rely.h"
#include "Nailang/ParserRely.h"
#include "Nailang/NailangParser.h"
#include "Nailang/NailangRuntime.h"


using namespace std::string_view_literals;
using common::parser::ParserContext;
using common::parser::ContextReader;
using xziar::nailang::MemoryPool;
using xziar::nailang::BlockParser;
using xziar::nailang::ComplexArgParser;
using xziar::nailang::EmbedOps;
using xziar::nailang::LateBindVar;
using xziar::nailang::BinaryExpr;
using xziar::nailang::UnaryExpr;
using xziar::nailang::FuncCall;
using xziar::nailang::RawArg;
using xziar::nailang::Arg;
using xziar::nailang::Assignment;
using xziar::nailang::Block;
using xziar::nailang::NailangRuntimeBase;
using xziar::nailang::EvaluateContext;
using xziar::nailang::BasicEvaluateContext;
using xziar::nailang::LargeEvaluateContext;
using xziar::nailang::CompactEvaluateContext;
using xziar::nailang::EmbedOpEval;

class EvalCtx : public CompactEvaluateContext
{
public:
    EvalCtx()
    {
        SetArgInside(U"valU64"sv, Arg(uint64_t(1)), true);
        SetArgInside(U"valI64"sv, Arg(int64_t(2)), true);
        SetArgInside(U"valF64"sv, Arg(3.0), true);
        SetArgInside(U"valStr"sv, Arg(U"txt"sv), true);
    }

    using CompactEvaluateContext::ParentContext;
};
class EvalCtx2 : public CompactEvaluateContext
{
public:
    EvalCtx2() { }

    using CompactEvaluateContext::ParentContext;
};
class NailangRT : public NailangRuntimeBase
{
public:
    NailangRT() : NailangRuntimeBase(std::make_shared<EvalCtx>())
    {
    }

    auto GetCtx() const { return std::dynamic_pointer_cast<EvalCtx>(EvalContext); }

    using NailangRuntimeBase::EvalContext;

    using NailangRuntimeBase::EvaluateFunc;
    using NailangRuntimeBase::EvaluateArg;
    using NailangRuntimeBase::ExecuteAssignment;
    using NailangRuntimeBase::ExecuteContent;
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


//class NailangRuntime : public ::testing::Test 
//{
//protected:
//    MemoryPool Pool;
//    NailangRT Runtime;
//    void SetUp() override { }
//    void TearDown() override { }
//};


TEST(NailangRuntime, VarLookup)
{
    constexpr auto var = U"a.b..c"sv;
    xziar::nailang::VarLookup lookup(var);
    EXPECT_EQ(lookup.Name, var);
    EXPECT_EQ(lookup.Part(), U"a"sv);
    EXPECT_TRUE(lookup.Next());
    EXPECT_EQ(lookup.Part(), U"b"sv);
    EXPECT_TRUE(lookup.Next());
    EXPECT_EQ(lookup.Part(), U""sv);
    EXPECT_TRUE(lookup.Next());
    EXPECT_EQ(lookup.Part(), U"c"sv);
    EXPECT_FALSE(lookup.Next());
    EXPECT_EQ(lookup.Part(), U""sv);
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



#define LOOKUP_ARG(rt, name, type, val) do      \
{                                               \
    const LateBindVar var{ name };              \
    const auto arg = runtime.EvaluateArg(var);  \
    CHECK_ARG(arg, type, val);                  \
} while(0)                                      \


TEST(NailangRuntime, Variable)
{
    NailangRT runtime;

    LOOKUP_ARG(runtime, U"valU64"sv, Uint,   1u);
    LOOKUP_ARG(runtime, U"valI64"sv, Int,    2);
    LOOKUP_ARG(runtime, U"valF64"sv, FP,     3.0);
    LOOKUP_ARG(runtime, U"valStr"sv, U32Sv,  U"txt"sv);

    {
        // _
        const auto arg = runtime.EvalContext->LookUpArg(U"test"sv);
        EXPECT_EQ(arg.TypeData, Arg::InternalType::Empty);
    }
    {
        EXPECT_EQ(runtime.EvalContext->SetArg(U"test"sv, Arg(uint64_t(512))), false);
        // 512
        const auto arg = runtime.EvalContext->LookUpArg(U"test"sv);
        CHECK_ARG(arg, Uint, 512);
    }
    const auto ctx2 = std::make_shared<EvalCtx>();
    ctx2->ParentContext = runtime.EvalContext;
    {
        // 512, _
        const auto arg1 = ctx2->LookUpArg(U"test"sv);
        CHECK_ARG(arg1, Uint, 512);
        const auto arg2 = ctx2->LookUpArg(U".test"sv);
        EXPECT_EQ(arg2.TypeData, Arg::InternalType::Empty);
    }
    {
        EXPECT_EQ(ctx2->SetArg(U"test"sv, Arg(uint64_t(256))), true);
        // 256, _
        const auto arg1 = ctx2->LookUpArg(U"test"sv);
        CHECK_ARG(arg1, Uint, 256);
        const auto arg2 = runtime.EvalContext->LookUpArg(U"test"sv);
        CHECK_ARG(arg2, Uint, 256);
        const auto arg3 = ctx2->LookUpArg(U".test"sv);
        EXPECT_EQ(arg3.TypeData, Arg::InternalType::Empty);
    }
    {
        EXPECT_EQ(ctx2->SetArg({ U".test"sv }, Arg(uint64_t(128))), false);
        // 256, 128
        const auto arg1 = ctx2->LookUpArg(U"test"sv);
        CHECK_ARG(arg1, Uint, 128);
        const auto arg2 = runtime.EvalContext->LookUpArg(U"test"sv);
        CHECK_ARG(arg2, Uint, 256);
        const auto arg3 = ctx2->LookUpArg(U".test"sv);
        CHECK_ARG(arg3, Uint, 128);
    }
    const auto ctx3 = std::make_shared<EvalCtx>();
    ctx3->ParentContext = ctx2;
    {
        // 256, 128, _
        const auto arg = ctx3->LookUpArg(U"test"sv);
        CHECK_ARG(arg, Uint, 128);
    }
    {
        EXPECT_EQ(ctx2->SetArg({ U".test"sv }, {}), true);
        // 256, _, _
        const auto arg1 = ctx3->LookUpArg(U"test"sv);
        CHECK_ARG(arg1, Uint, 256);
        const auto arg2 = ctx2->LookUpArg(U"test"sv);
        CHECK_ARG(arg2, Uint, 256);
        const auto arg3 = runtime.EvalContext->LookUpArg(U"test"sv);
        CHECK_ARG(arg3, Uint, 256);
    }
    {
        EXPECT_EQ(ctx3->SetArg({ U".test"sv }, Arg(uint64_t(64))), false);
        // 256, _, 64
        const auto arg1 = ctx3->LookUpArg(U"test"sv);
        CHECK_ARG(arg1, Uint, 64);
        const auto arg2 = ctx2->LookUpArg(U"test"sv);
        CHECK_ARG(arg2, Uint, 256);
        const auto arg3 = ctx2->LookUpArg(U".test"sv);
        EXPECT_EQ(arg3.TypeData, Arg::InternalType::Empty);
        const auto arg4 = runtime.EvalContext->LookUpArg(U"test"sv);
        CHECK_ARG(arg4, Uint, 256);
    }
}


struct BlkParser : public BlockParser
{
    using BlockParser::BlockParser;
    static Assignment GetAssignment(MemoryPool& pool, const std::u32string_view var, const std::u32string_view src)
    {
        ParserContext context(src);
        BlkParser parser(pool, context);
        return parser.ParseAssignment(var);
    }
    static Block GetBlock(MemoryPool& pool, const std::u32string_view src)
    {
        common::parser::ParserContext context(src);
        BlkParser parser(pool, context);
        Block ret;
        parser.ParseContentIntoBlock<true>(ret);
        return ret;
    }
};

static void PEAssign(NailangRT& runtime, MemoryPool& pool, 
    const std::u32string_view var, const std::u32string_view src,
    const common::span<const FuncCall>& metas = {})
{
    const Assignment assign = BlkParser::GetAssignment(pool, var, src);
    runtime.ExecuteAssignment(assign, metas);
}

TEST(NailangRuntime, Assign)
{
    MemoryPool pool;
    NailangRT runtime;
    {
        PEAssign(runtime, pool, U"str"sv, U"=\"Hello \";"sv);
        LOOKUP_ARG(runtime, U"str"sv, U32Sv, U"Hello "sv);
    }
    {
        PEAssign(runtime, pool, U"str"sv, U"+= \"World\";"sv);
        LOOKUP_ARG(runtime, U"str"sv, U32Str, U"Hello World"sv);
    }
    {
        constexpr auto ans = (63 % 4) * (3 + 5.0);
        PEAssign(runtime, pool, U"ans"sv, U"= (63 % 4) * (3 + 5.0);"sv);
        LOOKUP_ARG(runtime, U"ans"sv, FP, ans);
    }
    {
        const auto ans = std::fmod(24.0, (2 + 3.0)/2);
        PEAssign(runtime, pool, U"ans"sv, U"%= (valI64 + valF64)/valI64;"sv);
        LOOKUP_ARG(runtime, U"ans"sv, FP, ans);
    }
}


TEST(NailangRuntime, DefFunc)
{
    MemoryPool pool;
    NailangRT runtime;
    const auto evalCtx = runtime.GetCtx();
    const auto PEDefFunc = [&](const std::u32string_view src)
    {
        const auto block = BlkParser::GetBlock(pool, src);
        runtime.ExecuteBlock(block, {});
    };

    {
        constexpr auto txt = UR"(
@DefFunc()
#Block("abc")
{
}
)"sv;
        PEDefFunc(txt);
        EXPECT_EQ(evalCtx->GetFuncCount(), 1);
        const auto func = evalCtx->LookUpFunc(U"abc"sv);
        ASSERT_TRUE(func);
        EXPECT_EQ(func.ArgNames.size(), 0);
    }

    {
        constexpr auto txt = UR"(
@DefFunc(num, length)
#Block("func2")
{
}
)"sv;
        PEDefFunc(txt);
        EXPECT_EQ(evalCtx->GetFuncCount(), 2);
        const auto func = evalCtx->LookUpFunc(U"func2"sv);
        ASSERT_TRUE(func);
        EXPECT_THAT(func.ArgNames, testing::ElementsAre(U"num"sv, U"length"sv));
    }
}


TEST(NailangRuntime, gcd1)
{
    MemoryPool pool;
    NailangRT runtime;

    constexpr auto gcdTxt = UR"(
tmp = 1;
@While(tmp != 0)
#Block("")
{
    tmp = m % n;
    m = n;
    n = tmp;
}
)"sv;

    constexpr auto refgcd = [](uint64_t m, uint64_t n)
    {
        uint64_t tmp = 1;
        while (tmp != 0)
        {
            tmp = m % n;
            m = n;
            n = tmp;
        }
        return m;
    };

    EXPECT_EQ(refgcd(5, 5),  std::gcd(5, 5));
    EXPECT_EQ(refgcd(15, 5), std::gcd(15, 5));
    EXPECT_EQ(refgcd(17, 5), std::gcd(17, 5));

    const auto algoBlock = BlkParser::GetBlock(pool, gcdTxt);

    EXPECT_EQ(algoBlock.Size(), 2);

    const auto gcd = [&](uint64_t m, uint64_t n)
    {
        runtime.EvalContext->SetArg(U"m"sv, m);
        runtime.EvalContext->SetArg(U"n"sv, n);
        runtime.ExecuteBlock(algoBlock, {});
        const auto ans = runtime.EvalContext->LookUpArg(U"m");
        EXPECT_EQ(ans.TypeData, Arg::InternalType::Uint);
        return *ans.GetUint();
    };
    EXPECT_EQ(gcd(5, 5), std::gcd(5, 5));
    EXPECT_EQ(gcd(15, 5), std::gcd(15, 5));
    EXPECT_EQ(gcd(17, 5), std::gcd(17, 5));
}

TEST(NailangRuntime, gcd2)
{
    MemoryPool pool;
    NailangRT runtime;

    constexpr auto gcdTxt = UR"(
@While(true)
#Block("")
{
    tmp = m % n;
    m = n;
    n = tmp;
    @If(n==0)
    $Break();
}
)"sv;

    constexpr auto refgcd = [](auto m, auto n)
    {
        while (true)
        {
            const auto tmp = m % n;
            m = n;
            n = tmp;
            if (n == 0)
                break;
        }
        return m;
    };

    EXPECT_EQ(refgcd(5, 5),  std::gcd(5, 5));
    EXPECT_EQ(refgcd(15, 5), std::gcd(15, 5));
    EXPECT_EQ(refgcd(17, 5), std::gcd(17, 5));

    const auto algoBlock = BlkParser::GetBlock(pool, gcdTxt);

    EXPECT_EQ(algoBlock.Size(), 1);

    const auto gcd = [&](uint64_t m, uint64_t n)
    {
        runtime.EvalContext->SetArg(U"m"sv, m);
        runtime.EvalContext->SetArg(U"n"sv, n);
        runtime.ExecuteBlock(algoBlock, {});
        const auto ans = runtime.EvalContext->LookUpArg(U"m");
        EXPECT_EQ(ans.TypeData, Arg::InternalType::Uint);
        return *ans.GetUint();
    };
    EXPECT_EQ(gcd(5, 5), std::gcd(5, 5));
    EXPECT_EQ(gcd(15, 5), std::gcd(15, 5));
    EXPECT_EQ(gcd(17, 5), std::gcd(17, 5));
}


TEST(NailangRuntime, gcd3)
{
    MemoryPool pool;
    NailangRT runtime;

    constexpr auto gcdTxt = UR"(
@DefFunc(m,n)
#Block("gcd")
{
    tmp = m % n;
    @If(tmp==0)
    $Return(n);

    $Return($gcd(n, tmp));
}
m = $gcd(m,n);
)"sv;

    struct Temp
    {
        static constexpr auto refgcd(int64_t m, int64_t n)
        {
            const uint64_t tmp = m % n;
            if (tmp == 0)
                return n;
            return refgcd(n, tmp);
        }
    };

    EXPECT_EQ(Temp::refgcd(5, 5), std::gcd(5, 5));
    EXPECT_EQ(Temp::refgcd(15, 5), std::gcd(15, 5));
    EXPECT_EQ(Temp::refgcd(17, 5), std::gcd(17, 5));

    const auto algoBlock = BlkParser::GetBlock(pool, gcdTxt);

    EXPECT_EQ(algoBlock.Size(), 2);

    const auto gcd = [&](uint64_t m, uint64_t n)
    {
        runtime.EvalContext->SetArg(U"m"sv, m);
        runtime.EvalContext->SetArg(U"n"sv, n);
        runtime.ExecuteBlock(algoBlock, {});
        const auto ans = runtime.EvalContext->LookUpArg(U"m");
        EXPECT_EQ(ans.TypeData, Arg::InternalType::Uint);
        return *ans.GetUint();
    };
    EXPECT_EQ(gcd(5, 5), std::gcd(5, 5));
    EXPECT_EQ(gcd(15, 5), std::gcd(15, 5));
    EXPECT_EQ(gcd(17, 5), std::gcd(17, 5));
}