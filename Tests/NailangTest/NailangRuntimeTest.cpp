#include "rely.h"
#include "Nailang/ParserRely.h"
#include "Nailang/NailangParser.h"
#include "Nailang/NailangRuntime.h"
#include "SystemCommon/MiscIntrins.h"
#include <cmath>



using namespace std::string_view_literals;
using common::parser::ParserContext;
using common::parser::ContextReader;
using xziar::nailang::MemoryPool;
using xziar::nailang::NailangParser;
using xziar::nailang::NailangParser;
using xziar::nailang::EmbedOps;
using xziar::nailang::LateBindVar;
using xziar::nailang::FixedArray;
using xziar::nailang::BinaryExpr;
using xziar::nailang::UnaryExpr;
using xziar::nailang::SubQuery;
using xziar::nailang::QueryExpr;
using xziar::nailang::FuncCall;
using xziar::nailang::Expr;
using xziar::nailang::CustomVar;
using xziar::nailang::Arg;
using xziar::nailang::ArgLocator;
using xziar::nailang::NilCheck;
using xziar::nailang::AssignExpr;
using xziar::nailang::Block;
using xziar::nailang::NailangRuntimeBase;
using xziar::nailang::EvaluateContext;
using xziar::nailang::BasicEvaluateContext;
using xziar::nailang::LargeEvaluateContext;
using xziar::nailang::CompactEvaluateContext;
using xziar::nailang::EmbedOpEval;


testing::AssertionResult CheckArg(const Arg& arg, const Arg::Type type)
{
    if (arg.TypeData == type)
        return testing::AssertionSuccess();
    else
        return testing::AssertionFailure() << "arg is [" << WideToChar(arg.GetTypeName()) << "](" << common::enum_cast(arg.TypeData) << ")";
}

#define CHECK_ARG(arg, type, val) do                \
{                                                   \
    ASSERT_TRUE(CheckArg(arg, Arg::Type::type));    \
    EXPECT_EQ(arg.GetVar<Arg::Type::type>(), val);  \
} while(0)                                          \


class EvalCtx : public CompactEvaluateContext
{
public:
    EvalCtx()
    {
        LocateArg(U"valU64"sv, true).Set(uint64_t(1));
        LocateArg(U"valI64"sv, true).Set(int64_t(2));
        LocateArg(U"valF64"sv, true).Set(3.0);
        LocateArg(U"valStr"sv, true).Set(U"txt"sv);
    }
};
class EvalCtx2 : public CompactEvaluateContext
{
public:
    EvalCtx2() { }
};
class NailangRT : public NailangRuntimeBase
{
public:
    BlockFrame BaseFrame;
    NailangRT() : NailangRuntimeBase(std::make_shared<EvalCtx>()), BaseFrame(nullptr, RootContext, FrameFlags::Empty)
    {
        CurFrame = &BaseFrame;
    }

    using NailangRuntimeBase::RootContext;

    using NailangRuntimeBase::EvaluateFunc;
    using NailangRuntimeBase::LookUpArg;
    using NailangRuntimeBase::EvaluateExpr;
    using NailangRuntimeBase::HandleContent;

    auto GetCtx() const { return std::dynamic_pointer_cast<EvalCtx>(RootContext); }
    auto SetRootArg(std::u32string_view name, Arg val)
    {
        return RootContext->LocateArg(name, true).Set(std::move(val));
        //return *RootContext->LocateArg(name, true) = std::move(val);
    }
    void Assign(const AssignExpr& assign)
    {
        SetArg(assign.Target, assign.Queries, assign.Statement, assign.Check);
    }
    void QuickSetArg(std::u32string_view name, Expr val, bool create = false)
    {
        ParserContext context(name);
        const auto var = xziar::nailang::NailangParser::ParseSingleExpr(MemPool, context, ""sv, U""sv);
        std::u32string_view varName;
        SubQuery query;
        if (var.TypeData == Expr::Type::Var)
            varName = var.GetVar<Expr::Type::Var>();
        else
        {
            const auto& qexpr = *var.GetVar<Expr::Type::Query>();
            varName = qexpr.Target.GetVar<Expr::Type::Var>();
            query = qexpr.Sub(0);
        }
        AssignExpr assign(varName);
        assign.Queries = query;
        assign.Statement = val;
        NilCheck::Behavior notnull = create ? NilCheck::Behavior::Throw : NilCheck::Behavior::Pass,
            null = query.Size() == 0 ? NilCheck::Behavior::Pass : NilCheck::Behavior::Throw;
        assign.Check = { notnull, null };
        Assign(assign);
    }
    Arg QuickGetArg(std::u32string_view name)
    {
        ParserContext context(name);
        const auto var = xziar::nailang::NailangParser::ParseSingleExpr(MemPool, context, ""sv, U""sv);
        return EvaluateExpr(var);
    }
};

struct BaseCustomVar : public xziar::nailang::CustomVar::Handler
{
    common::str::StrVariant<char32_t> ToString(const CustomVar&) noexcept override { return U"{BaseCustomVar}"; };
};
static BaseCustomVar BaseCustomVarHandler;


TEST(NailangBase, ArgToString)
{
    constexpr auto ChkStr = [](const Arg arg, const std::u32string_view str)
    {
        EXPECT_EQ(arg.ToString(), str);
    };
    ChkStr(int64_t(-123), U"-123"sv);
    ChkStr(uint64_t(123), U"123"sv);
    ChkStr(1.5, U"1.5"sv);
    ChkStr(U"Here"sv, U"Here"sv);
    ChkStr({}, U""sv);
    ChkStr(CustomVar{ &BaseCustomVarHandler, 0,0,0 }, U"{BaseCustomVar}"sv);
}

TEST(NailangRuntime, EvalEmbedOp)
{
    using Type = Arg::Type;
#define TEST_BIN(l, r, op, type, ans) do            \
{                                                   \
    Arg left(l), right(r);                          \
    const auto ret = EmbedOpEval::op(left, right);  \
    CHECK_ARG(ret, type, ans);                      \
} while(0)                                          

#define TEST_UN(val, op, type, ans) do      \
{                                           \
    Arg arg(val);                           \
    const auto ret = EmbedOpEval::op(arg);  \
    CHECK_ARG(ret, type, ans);              \
} while(0)                                  \

    TEST_BIN(uint64_t(1), uint64_t(2), Equal,          Bool, false);
    TEST_BIN(uint64_t(0), uint64_t(0), NotEqual,       Bool, false);
    TEST_BIN(uint64_t(1), uint64_t(2), Add,            Uint, 3u);
    TEST_BIN(uint64_t(1), int64_t(-1), Add,            Uint, 0u);
    TEST_BIN(uint64_t(1),         0.5, Sub,              FP, 0.5);
    TEST_BIN( int64_t(5), int64_t(-1), Mul,             Int, -5);
    TEST_BIN(uint64_t(3), uint64_t(2), Div,            Uint, 1u);
    TEST_BIN(uint64_t(3),         0.5, Div,              FP, 6);
    TEST_BIN(uint64_t(3), uint64_t(2), Rem,            Uint, 1u);
    TEST_BIN(        2.5,         2.0, Rem,              FP, 0.5);
    TEST_BIN(   U"abc"sv,    U"ABC"sv, Add,          U32Str, U"abcABC"sv);
    TEST_BIN(uint64_t(1), uint64_t(2), Less,           Bool, true);
    TEST_BIN(int64_t(-1), uint64_t(2), Less,           Bool, true);
    TEST_BIN(uint64_t(1), int64_t(-2), Less,           Bool, false);
    TEST_BIN(uint64_t(1), int64_t(-2), LessEqual,      Bool, false);
    TEST_BIN(       -2.0, int64_t(-2), GreaterEqual,   Bool, true);
    TEST_BIN(       -2.0,       U""sv, And,            Bool, false);
    TEST_BIN(       true,       U""sv, Or,             Bool, true);
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


TEST(NailangRuntime, ParseEvalEmbedOp)
{
    MemoryPool pool;
    const auto ParseEval = [&](const std::u32string_view src) 
    {
        ParserContext context(src);
        NailangRT runtime;
        const auto rawarg = NailangParser::ParseSingleExpr(pool, context);
        return runtime.EvaluateExpr(rawarg);
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


struct ArrayCustomVar : public xziar::nailang::CustomVar::Handler
{
    struct COMMON_EMPTY_BASES Array : public common::FixedLenRefHolder<Array, float>
    {
        common::span<float> Data;
        Array(common::span<const float> source)
        {
            Data = FixedLenRefHolder<Array, float>::Allocate(source.size());
            if (Data.size() > 0)
            {
                memcpy_s(Data.data(), sizeof(float) * Data.size(), source.data(), sizeof(float) * source.size());
            }
        }
        Array(const Array& other) noexcept : Data(other.Data)
        {
            this->Increase();
        }
        Array(Array&& other) noexcept : Data(other.Data)
        {
            other.Data = {};
        }
        ~Array()
        {
            this->Decrease();
        }
        Arg ToArg() noexcept;
        [[nodiscard]] forceinline uintptr_t GetDataPtr() const noexcept
        {
            return reinterpret_cast<uintptr_t>(Data.data());
        }
        forceinline void Destruct() noexcept { }
        using common::FixedLenRefHolder<Array, float>::Increase;
        using common::FixedLenRefHolder<Array, float>::Decrease;
    };
    static_assert(sizeof(Array) == sizeof(common::span<const float>));
#define ArrRef(out, in)                                                         \
    common::span<float> tmp_ = {reinterpret_cast<float*>(in.Meta0), in.Meta1};  \
    Array& out = *reinterpret_cast<Array*>(&tmp_);
    void IncreaseRef(CustomVar& var) noexcept override
    { 
        ArrRef(arr, var);
        arr.Increase(); 
    };
    void DecreaseRef(CustomVar& var) noexcept override
    { 
        ArrRef(arr, var);
        if (arr.Decrease())
            var.Meta0 = 0;
    };
    /*Arg IndexerGetter(const CustomVar& var, const Arg& idx, const Expr& src) override
    {
        ArrRef(arr, var);
        const auto idx_ = xziar::nailang::NailangHelper::BiDirIndexCheck(arr.Data.size(), idx, &src);
        return arr.Data[idx_];
    }
    Arg SubfieldGetter(const CustomVar& var, std::u32string_view field) override
    {
        if (field == U"Length"sv)
        {
            ArrRef(arr, var);
            return static_cast<uint64_t>(arr.Data.size());
        }
        return {};
    }*/
    ArgLocator HandleQuery(CustomVar& var, SubQuery subq, NailangRuntimeBase& runtime) override
    {
        ArrRef(arr, var);
        const auto [type, query] = subq[0];
        switch (type)
        {
        case SubQuery::QueryType::Index:
        {
            const auto val = EvaluateExpr(runtime, query);
            const auto idx = xziar::nailang::NailangHelper::BiDirIndexCheck(arr.Data.size(), val, &query);
            return
            {
                [=, dat = arr.Data] ()->Arg { return dat[idx]; },
                [=, dat = arr.Data] (Arg val) -> bool
                {
                    if (const auto real = val.GetFP(); real.has_value())
                    {
                        dat[idx] = static_cast<float>(real.value());
                        return true;
                    }
                    return false;
                }, 1 
            };
        }
        case SubQuery::QueryType::Sub:
        {
            const auto field = query.GetVar<Expr::Type::Str>();
            if (field == U"Length"sv)
                return { static_cast<uint64_t>(arr.Data.size()), 1u };
            float* ptr = nullptr;
            if (field == U"First"sv)
            {
                if (arr.Data.size() > 0)
                    ptr = arr.Data.data();
            }
            else if (field == U"Last"sv)
            {
                if (arr.Data.size() > 0)
                    ptr = arr.Data.data() + arr.Data.size() - 1;
            }
            if (ptr)
            {
                return
                {
                    [=] () -> Arg { return *ptr; },
                    [=] (Arg val) -> bool
                    {
                        if (const auto real = val.GetFP(); real.has_value())
                        {
                            *ptr = static_cast<float>(real.value());
                            return true;
                        }
                        return false;
                    }, 1
                };
            }
        } break;
        default: break;
        }
        return {};

    }
    common::str::StrVariant<char32_t> ToString(const CustomVar&) noexcept override { return U"{ArrayCustomVar}"; };
    Arg ConvertToCommon(const CustomVar&, Arg::Type) noexcept override { return {}; }
    static Arg Create(common::span<const float> source);
    static common::span<float> GetData(const CustomVar& var) 
    {
        ArrRef(arr, var);
        return arr.Data;
    }
#undef ArrRef
};
static ArrayCustomVar ArrayCustomVarHandler; 
Arg ArrayCustomVar::Array::ToArg() noexcept
{
    this->Increase();
    return CustomVar{ &ArrayCustomVarHandler,
        reinterpret_cast<uint64_t>(Data.data()), gsl::narrow_cast<uint32_t>(Data.size()), 0 };
}
Arg ArrayCustomVar::Create(common::span<const float> source)
{
    Array arr(source);
    return arr.ToArg();
}


TEST(NailangRuntime, CustomVar)
{
    MemoryPool pool;
    NailangRT runtime;
    constexpr float dummy[] = { 0.f,1.f,4.f,-2.f };
    runtime.SetRootArg(U"arr", ArrayCustomVar::Create(dummy));
    {
        const auto arg = runtime.QuickGetArg(U"arr"sv);
        ASSERT_TRUE(CheckArg(arg, Arg::Type::Var));
        const auto& var = arg.GetCustom();
        ASSERT_EQ(var.Host, &ArrayCustomVarHandler);
        const auto data = ArrayCustomVar::GetData(var);
        EXPECT_THAT(data, testing::ElementsAreArray(dummy));
    }
    {
        const auto arg = runtime.QuickGetArg(U"arr"sv);
        EXPECT_EQ(arg.ToString(), U"{ArrayCustomVar}"sv);
    }
    {
        const auto len = runtime.QuickGetArg(U"arr.Length"sv);
        CHECK_ARG(len, Uint, 4u);
    }
    {
        const auto arg  = runtime.QuickGetArg(U"arr"sv);
        const auto& var = arg.GetCustom();
        const auto data = ArrayCustomVar::GetData(var);

        EXPECT_THAT(data, testing::ElementsAreArray(dummy));
        runtime.QuickSetArg(U"arr.First"sv, 9.f);
        EXPECT_THAT(data, testing::ElementsAre(9.f, 1.f, 4.f, -2.f));
        runtime.QuickSetArg(U"arr.Last"sv, -9.f);
        EXPECT_THAT(data, testing::ElementsAre(9.f, 1.f, 4.f, -9.f));
    }
}


TEST(NailangRuntime, Indexer)
{
    MemoryPool pool;
    NailangRT runtime;
    runtime.SetRootArg(U"tmp", U"Hello"sv);
    const auto ParseEval = [&](const std::u32string_view src)
    {
        ParserContext context(src);
        const auto rawarg = NailangParser::ParseSingleExpr(pool, context);
        return runtime.EvaluateExpr(rawarg);
    };
    {
        const auto arg = ParseEval(U"tmp[0];"sv);
        CHECK_ARG(arg, U32Sv, U"H"sv);
    }
    {
        const auto arg = ParseEval(U"tmp[3];"sv);
        CHECK_ARG(arg, U32Sv, U"l"sv);
    }
    {
        const auto arg = ParseEval(U"tmp[-1];"sv);
        CHECK_ARG(arg, U32Sv, U"o"sv);        
    }
    {
        EXPECT_THROW(ParseEval(U"tmp[5];"sv), xziar::nailang::NailangRuntimeException);
    }
    {
        EXPECT_THROW(ParseEval(U"tmp[-5];"sv), xziar::nailang::NailangRuntimeException);
    }
    constexpr float dummy[] = { 0.f,1.f,4.f,-2.f };
    runtime.SetRootArg(U"arr", ArrayCustomVar::Create(dummy));
    {
        const auto arg = ParseEval(U"arr[0];"sv);
        CHECK_ARG(arg, FP, 0.f);
    }
    {
        const auto arg = ParseEval(U"arr[1];"sv);
        CHECK_ARG(arg, FP, 1.f);
    }
    {
        const auto arg = ParseEval(U"arr[2];"sv);
        CHECK_ARG(arg, FP, 4.f);
    }
    {
        const auto arg = ParseEval(U"arr[3];"sv);
        CHECK_ARG(arg, FP, -2.f);
    }
    {
        const auto arg = ParseEval(U"arr[-1];"sv);
        CHECK_ARG(arg, FP, -2.f);
    }
    {
        EXPECT_THROW(ParseEval(U"arr[-5];"sv), xziar::nailang::NailangRuntimeException);
    }
}


TEST(NailangRuntime, FixedArray)
{
    MemoryPool pool;
    NailangRT runtime;
    constexpr float dummy1[] = { 0.f,1.f,4.f,-2.f };
    int8_t dummy2[] = { 0,1,4,-2 };
    runtime.SetRootArg(U"arr1", FixedArray::Create<const float>(dummy1));
    runtime.SetRootArg(U"arr2", FixedArray::Create<int8_t>(dummy2));
    {
        const auto arg = runtime.QuickGetArg(U"arr1"sv);
        ASSERT_TRUE(CheckArg(arg, Arg::Type::Array));
        const auto var = arg.GetVar<Arg::Type::Array>();
        ASSERT_EQ(var.ElementType, FixedArray::Type::F32);
        ASSERT_TRUE(var.IsReadOnly);
        const auto sp  = var.GetSpan();
        ASSERT_TRUE(std::holds_alternative<common::span<const float>>(sp));
        const auto data = std::get<common::span<const float>>(sp);
        EXPECT_THAT(data, testing::ElementsAreArray(dummy1));
    }
    {
        const auto arg = runtime.QuickGetArg(U"arr2"sv);
        ASSERT_TRUE(CheckArg(arg, Arg::Type::Array));
        const auto var = arg.GetVar<Arg::Type::Array>();
        ASSERT_EQ(var.ElementType, FixedArray::Type::I8);
        ASSERT_FALSE(var.IsReadOnly);
        const auto sp = var.GetSpan();
        ASSERT_TRUE(std::holds_alternative<common::span<int8_t>>(sp));
        const auto data = std::get<common::span<int8_t>>(sp);
        EXPECT_THAT(data, testing::ElementsAreArray(dummy2));
    }
    {
        const auto arg = runtime.QuickGetArg(U"arr1"sv);
        EXPECT_EQ(arg.ToString(), U"[0, 1, 4, -2]"sv);
    }
    {
        const auto arg = runtime.QuickGetArg(U"arr2"sv);
        EXPECT_EQ(arg.ToString(), U"[0, 1, 4, -2]"sv);
    }
    {
        const auto len = runtime.QuickGetArg(U"arr1.Length"sv);
        CHECK_ARG(len, Uint, 4u);
    }
    {
        const auto len = runtime.QuickGetArg(U"arr2.Length"sv);
        CHECK_ARG(len, Uint, 4u);
    }
    {
        EXPECT_THROW(runtime.QuickSetArg(U"arr1[0]"sv, 2.0f), xziar::nailang::NailangRuntimeException);
    }
    {
        const auto arg = runtime.QuickGetArg(U"arr2"sv);
        const auto var = arg.GetVar<Arg::Type::Array>();
        const auto data = std::get<common::span<int8_t>>(var.GetSpan());
        EXPECT_THAT(data, testing::ElementsAreArray(dummy2));
        runtime.QuickSetArg(U"arr2[0]"sv, 2.0f);
        EXPECT_THAT(data, testing::ElementsAreArray(std::array<int8_t, 4>{2, 1, 4, -2}));
        runtime.QuickSetArg(U"arr2[-1]"sv, uint64_t(9));
        EXPECT_THAT(data, testing::ElementsAreArray(std::array<int8_t, 4>{2, 1, 4, 9}));
        // EXPECT_THROW(runtime.QuickSetArg(U"arr2[0]"sv, U""sv), xziar::nailang::NailangRuntimeException);
    }
}


TEST(NailangRuntime, MathFunc)
{
    MemoryPool pool;
    const auto ParseEval = [&](const std::u32string_view src)
    {
        ParserContext context(src);
        const auto rawarg = NailangParser::ParseSingleExpr(pool, context);
        NailangRT runtime;
        return runtime.EvaluateExpr(rawarg);
    };
    {
        const auto arg = ParseEval(U"$Math.Max(1,2);"sv);
        CHECK_ARG(arg, Int, 2);
    }
    {
        const auto arg = ParseEval(U"$Math.Max(1,2,3);"sv);
        CHECK_ARG(arg, Int, 3);
    }
    {
        const auto arg = ParseEval(U"$Math.Max(3,2,4.5);"sv);
        CHECK_ARG(arg, FP, 4.5);
    }
    {
        const auto arg = ParseEval(U"$Math.Min(3,2,4.5);"sv);
        CHECK_ARG(arg, Int, 2);
    }
    {
        const auto arg = ParseEval(U"$Math.Sqrt(4.5);"sv);
        CHECK_ARG(arg, FP, std::sqrt(4.5));
    }
    {
        const auto arg = ParseEval(U"$Math.Ceil(4.5);"sv);
        CHECK_ARG(arg, FP, std::ceil(4.5));
    }
    {
        const auto arg = ParseEval(U"$Math.Floor(4.5);"sv);
        CHECK_ARG(arg, FP, std::floor(4.5));
    }
    {
        const auto arg = ParseEval(U"$Math.Round(4.25);"sv);
        CHECK_ARG(arg, FP, std::round(4.25));
    }
    {
        const auto arg = ParseEval(U"$Math.Log(4.25);"sv);
        CHECK_ARG(arg, FP, std::log(4.25));
    }
    {
        const auto arg = ParseEval(U"$Math.Log2(16384);"sv);
        CHECK_ARG(arg, FP, std::log2(16384));
    }
    {
        const auto arg = ParseEval(U"$Math.Log10(0.5);"sv);
        CHECK_ARG(arg, FP, std::log10(0.5));
    }
    {
        const auto arg = ParseEval(U"$Math.Lerp(0, 100, 0.25);"sv);
#if defined(__cpp_lib_interpolate) && __cpp_lib_interpolate >= 201902L
        const auto ref = std::lerp(0., 100., 0.25);
#else
        const auto ref = 0. * (1. - 0.25) + 100. * 0.25;
#endif
        CHECK_ARG(arg, FP, ref);
    }
    {
        const auto arg = ParseEval(U"$Math.Pow(3.5, 4);"sv);
        CHECK_ARG(arg, FP, std::pow(3.5, 4));
    }
    {
        const auto arg = ParseEval(U"$Math.ToUint(-100);"sv);
        CHECK_ARG(arg, Uint, static_cast<uint64_t>(-100));
    }
    {
        const auto arg = ParseEval(U"$Math.ToUint(5.6);"sv);
        CHECK_ARG(arg, Uint, static_cast<uint64_t>(5.6));
    }
    {
        const auto arg = ParseEval(U"$Math.ToInt(-5.6);"sv);
        CHECK_ARG(arg, Int, static_cast<int64_t>(-5.6));
    }
}


TEST(NailangRuntime, CommonFunc)
{
    MemoryPool pool;
    NailangRT runtime;
    runtime.SetRootArg(U"tmp0", true);
    runtime.SetRootArg(U"tmp1", int64_t(-512));
    runtime.SetRootArg(U"tmp2", U"Error"sv);
    const auto ParseEval = [&](const std::u32string_view src)
    {
        ParserContext context(src);
        const auto rawarg = NailangParser::ParseSingleExpr(pool, context);
        return runtime.EvaluateExpr(rawarg);
    };
    {
        const auto arg1 = ParseEval(U"$Exists(notexist);"sv);
        CHECK_ARG(arg1, Bool, false);
        runtime.SetRootArg(U"notexist", false);
        const auto arg2 = ParseEval(U"$Exists(notexist);"sv);
        CHECK_ARG(arg2, Bool, true);
        const auto arg3 = ParseEval(U"$Exists(\"notexist\");"sv);
        CHECK_ARG(arg3, Bool, true);
        runtime.SetRootArg(U"notexist", {});
        const auto arg4 = ParseEval(U"$Exists(\"notexist\");"sv);
        CHECK_ARG(arg4, Bool, false);
    }
    {
        const auto arg1 = ParseEval(U"tmp1 ?? notexist;"sv);
        CHECK_ARG(arg1, Int, -512);
        const auto arg2 = ParseEval(U"notexist ?? tmp2;"sv);
        CHECK_ARG(arg2, U32Sv, U"Error"sv);
        runtime.SetRootArg(U"notexist", 3.0);
        const auto arg3 = ParseEval(U"notexist ?? tmp2;"sv);
        CHECK_ARG(arg3, FP, 3.0);
        runtime.SetRootArg(U"notexist", {});
    }
    {
        const auto arg1 = ParseEval(U"$ExistsDynamic(\"tmp\" + \"1\");"sv);
        CHECK_ARG(arg1, Bool, true);
        const auto arg2 = ParseEval(U"$ExistsDynamic(\"tmp\" + \"3\");"sv);
        CHECK_ARG(arg2, Bool, false);
    }
    {
        const auto arg = ParseEval(UR"($Format("Hello {}", "World");)"sv);
        CHECK_ARG(arg, U32Str, U"Hello World"sv);
    }
    {
        const auto arg = ParseEval(UR"($Format("tmp0 = [{}], tmp1 = [{}], tmp2 = [{}]", tmp0, tmp1, tmp2);)"sv);
        CHECK_ARG(arg, U32Str, U"tmp0 = [true], tmp1 = [-512], tmp2 = [Error]"sv);
    }
}


TEST(NailangRuntime, MathIntrinFunc)
{
    MemoryPool pool;
    const auto ParseEval = [&](const std::u32string_view src)
    {
        ParserContext context(src);
        const auto rawarg = NailangParser::ParseSingleExpr(pool, context);
        NailangRT runtime;
        return runtime.EvaluateExpr(rawarg);
    };
    for (const auto& [inst, var] : common::MiscIntrin.GetIntrinMap())
    {
        TestCout() << "intrin [" << inst << "] use [" << var << "]\n";
    }
    ASSERT_TRUE(common::MiscIntrin.IsComplete());
    {
        const auto arg = ParseEval(U"$Math.LeadZero(0b0001111);"sv);
        CHECK_ARG(arg, Uint, 60u);
    }
    {
        const auto arg = ParseEval(U"$Math.LeadZero(0b0);"sv);
        CHECK_ARG(arg, Uint, 64u);
    }
    {
        const auto arg = ParseEval(U"$Math.TailZero(0b110000);"sv);
        CHECK_ARG(arg, Uint, 4u);
    }
    {
        const auto arg = ParseEval(U"$Math.TailZero(0b0);"sv);
        CHECK_ARG(arg, Uint, 64u);
    }
    {
        const auto arg = ParseEval(U"$Math.PopCount(0b10101010);"sv);
        CHECK_ARG(arg, Uint, 4u);
    }
}


#define LOOKUP_ARG(rt, name, type, val) do  \
{                                           \
    const auto arg = rt.LookUpArg(name);    \
    CHECK_ARG(arg, type, val);              \
} while(0)                                  \


//TEST(NailangRuntime, Variable)
//{
//    NailangRT runtime;
//
//    LOOKUP_ARG(runtime, U"valU64"sv, Uint,   1u);
//    LOOKUP_ARG(runtime, U"valI64"sv, Int,    2);
//    LOOKUP_ARG(runtime, U"valF64"sv, FP,     3.0);
//    LOOKUP_ARG(runtime, U"valStr"sv, U32Sv,  U"txt"sv);
//
//    {
//        // _
//        const auto arg = runtime.RootContext->LookUpArg(U"test"sv);
//        EXPECT_EQ(arg.TypeData, Arg::Type::Empty);
//    }
//    {
//        EXPECT_EQ(runtime.RootContext->SetArg(U"test"sv, Arg(uint64_t(512))), false);
//        // 512
//        const auto arg = runtime.RootContext->LookUpArg(U"test"sv);
//        CHECK_ARG(arg, Uint, 512u);
//    }
//    const auto ctx2 = std::make_shared<EvalCtx>();
//    ctx2->ParentContext = runtime.RootContext;
//    {
//        // 512, _
//        const auto arg1 = ctx2->LookUpArg(U"test"sv);
//        CHECK_ARG(arg1, Uint, 512u);
//        const auto arg2 = ctx2->LookUpArg(U":test"sv);
//        EXPECT_EQ(arg2.TypeData, Arg::Type::Empty);
//    }
//    {
//        EXPECT_EQ(ctx2->SetArg(U"test"sv, Arg(uint64_t(256))), true);
//        // 256, _
//        const auto arg1 = ctx2->LookUpArg(U"test"sv);
//        CHECK_ARG(arg1, Uint, 256u);
//        const auto arg2 = runtime.RootContext->LookUpArg(U"test"sv);
//        CHECK_ARG(arg2, Uint, 256u);
//        const auto arg3 = ctx2->LookUpArg(U":test"sv);
//        EXPECT_EQ(arg3.TypeData, Arg::Type::Empty);
//    }
//    {
//        EXPECT_EQ(ctx2->SetArg({ U":test"sv }, Arg(uint64_t(128))), false);
//        // 256, 128
//        const auto arg1 = ctx2->LookUpArg(U"test"sv);
//        CHECK_ARG(arg1, Uint, 128u);
//        const auto arg2 = runtime.RootContext->LookUpArg(U"test"sv);
//        CHECK_ARG(arg2, Uint, 256u);
//        const auto arg3 = ctx2->LookUpArg(U":test"sv);
//        CHECK_ARG(arg3, Uint, 128u);
//    }
//    const auto ctx3 = std::make_shared<EvalCtx>();
//    ctx3->ParentContext = ctx2;
//    {
//        // 256, 128, _
//        const auto arg = ctx3->LookUpArg(U"test"sv);
//        CHECK_ARG(arg, Uint, 128u);
//    }
//    {
//        EXPECT_EQ(ctx2->SetArg({ U":test"sv }, {}), true);
//        // 256, _, _
//        const auto arg1 = ctx3->LookUpArg(U"test"sv);
//        CHECK_ARG(arg1, Uint, 256u);
//        const auto arg2 = ctx2->LookUpArg(U"test"sv);
//        CHECK_ARG(arg2, Uint, 256u);
//        const auto arg3 = runtime.RootContext->LookUpArg(U"test"sv);
//        CHECK_ARG(arg3, Uint, 256u);
//    }
//    {
//        EXPECT_EQ(ctx3->SetArg({ U":test"sv }, Arg(uint64_t(64))), false);
//        // 256, _, 64
//        const auto arg1 = ctx3->LookUpArg(U"test"sv);
//        CHECK_ARG(arg1, Uint, 64u);
//        const auto arg2 = ctx2->LookUpArg(U"test"sv);
//        CHECK_ARG(arg2, Uint, 256u);
//        const auto arg3 = ctx2->LookUpArg(U":test"sv);
//        EXPECT_EQ(arg3.TypeData, Arg::Type::Empty);
//        const auto arg4 = runtime.RootContext->LookUpArg(U"test"sv);
//        CHECK_ARG(arg4, Uint, 256u);
//    }
//}


struct BlkParser : public NailangParser
{
    using NailangParser::NailangParser;
    static AssignExpr GetAssignExpr(MemoryPool& pool, const std::u32string_view var, const std::u32string_view src)
    {
        ParserContext context(src);
        BlkParser parser(pool, context);
        return parser.ParseAssignExpr(var);
    }
    static Block GetBlock(MemoryPool& pool, const std::u32string_view src)
    {
        common::parser::ParserContext context(src);
        BlkParser parser(pool, context);
        Block ret;
        parser.ParseContentIntoBlock(true, ret);
        return ret;
    }
};

static void PEAssign(NailangRT& runtime, MemoryPool& pool, 
    const std::u32string_view var, const std::u32string_view src)
{
    const AssignExpr assign = BlkParser::GetAssignExpr(pool, var, src);
    runtime.Assign(assign);
}

TEST(NailangRuntime, Assign)
{
    MemoryPool pool;
    NailangRT runtime;
    {
        PEAssign(runtime, pool, U"str"sv, U":=\"Hello \";"sv);
        LOOKUP_ARG(runtime, U"str"sv, U32Sv, U"Hello "sv);
    }
    {
        EXPECT_THROW(PEAssign(runtime, pool, U"str"sv, U":=1;"sv), xziar::nailang::NailangRuntimeException);
    }
    {
        PEAssign(runtime, pool, U"str"sv, U"+= \"World\";"sv);
        LOOKUP_ARG(runtime, U"str"sv, U32Str, U"Hello World"sv);
    }
    {
        constexpr auto ans = (63 % 4) * (3 + 5.0);
        PEAssign(runtime, pool, U"ans"sv, U":= (63 % 4) * (3 + 5.0);"sv);
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
        runtime.ExecuteBlock(block, {}, true, false);
    };

    {
        constexpr auto txt = UR"(
@DefFunc()
#Block("abc")
{
}
)"sv;
        PEDefFunc(txt);
        ASSERT_EQ(evalCtx->GetFuncCount(), 1u);
        const auto func = evalCtx->LookUpFunc(U"abc"sv);
        ASSERT_TRUE(func);
        EXPECT_EQ(func.ArgNames.size(), 0u);
    }

    {
        constexpr auto txt = UR"(
@DefFunc(num, length)
#Block("func2")
{
}
)"sv;
        PEDefFunc(txt);
        ASSERT_EQ(evalCtx->GetFuncCount(), 2u);
        const auto func = evalCtx->LookUpFunc(U"func2"sv);
        ASSERT_TRUE(func);
        EXPECT_THAT(func.ArgNames, testing::ElementsAre(U"num"sv, U"length"sv));
    }
}


uint64_t RunGCD(NailangRT& runtime, const Block& algoBlock, uint64_t m, uint64_t n)
{
    auto ctx = std::make_shared<CompactEvaluateContext>();
    ctx->LocateArg(U"m"sv, true).Set(m);
    ctx->LocateArg(U"n"sv, true).Set(n);
    runtime.ExecuteBlock(algoBlock, {}, ctx);
    const auto ans = ctx->LocateArg(U"m"sv, false).Get();
    EXPECT_EQ(ans.TypeData, Arg::Type::Uint);
    return *ans.GetUint();
}

TEST(NailangRuntime, gcd1)
{
    MemoryPool pool;
    NailangRT runtime;

    constexpr auto gcdTxt = UR"(
tmp := 1;
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

    EXPECT_EQ(refgcd( 5u, 5u), std::gcd( 5u, 5u));
    EXPECT_EQ(refgcd(15u, 5u), std::gcd(15u, 5u));
    EXPECT_EQ(refgcd(17u, 5u), std::gcd(17u, 5u));

    const auto algoBlock = BlkParser::GetBlock(pool, gcdTxt);

    ASSERT_EQ(algoBlock.Size(), 2u);

    EXPECT_EQ(RunGCD(runtime, algoBlock,  5u, 5u), std::gcd( 5u, 5u));
    EXPECT_EQ(RunGCD(runtime, algoBlock, 15u, 5u), std::gcd(15u, 5u));
    EXPECT_EQ(RunGCD(runtime, algoBlock, 17u, 5u), std::gcd(17u, 5u));
}

TEST(NailangRuntime, gcd2)
{
    MemoryPool pool;
    NailangRT runtime;

    constexpr auto gcdTxt = UR"(
@While(true)
#Block("")
{
    :tmp := m % n;
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

    EXPECT_EQ(refgcd( 5u, 5u), std::gcd( 5u, 5u));
    EXPECT_EQ(refgcd(15u, 5u), std::gcd(15u, 5u));
    EXPECT_EQ(refgcd(17u, 5u), std::gcd(17u, 5u));

    const auto algoBlock = BlkParser::GetBlock(pool, gcdTxt);

    ASSERT_EQ(algoBlock.Size(), 1u);

    EXPECT_EQ(RunGCD(runtime, algoBlock,  5u, 5u), std::gcd(5u, 5u));
    EXPECT_EQ(RunGCD(runtime, algoBlock, 15u, 5u), std::gcd(15u, 5u));
    EXPECT_EQ(RunGCD(runtime, algoBlock, 17u, 5u), std::gcd(17u, 5u));
}


TEST(NailangRuntime, gcd3)
{
    MemoryPool pool;
    NailangRT runtime;

    constexpr auto gcdTxt = UR"(
@DefFunc(m,n)
#Block("gcd")
{
    tmp := m % n;
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
    
    EXPECT_EQ(Temp::refgcd( 5u, 5u), std::gcd( 5u, 5u));
    EXPECT_EQ(Temp::refgcd(15u, 5u), std::gcd(15u, 5u));
    EXPECT_EQ(Temp::refgcd(17u, 5u), std::gcd(17u, 5u));

    const auto algoBlock = BlkParser::GetBlock(pool, gcdTxt);

    ASSERT_EQ(algoBlock.Size(), 2u);

    EXPECT_EQ(RunGCD(runtime, algoBlock,  5u, 5u), std::gcd(5u, 5u));
    EXPECT_EQ(RunGCD(runtime, algoBlock, 15u, 5u), std::gcd(15u, 5u));
    EXPECT_EQ(RunGCD(runtime, algoBlock, 17u, 5u), std::gcd(17u, 5u));
}