#include "rely.h"
#include "Nailang/NailangParserRely.h"
#include "Nailang/NailangParser.h"
#include "Nailang/NailangRuntime.h"
#include "SystemCommon/MiscIntrins.h"
#include "StringUtil/Convert.h"
#include "StringUtil/Format.h"
#include <cmath>



using namespace std::string_view_literals;
using common::parser::ParserContext;
using common::parser::ContextReader;
using xziar::nailang::MemoryPool;
using xziar::nailang::NailangParser;
using xziar::nailang::EmbedOps;
using xziar::nailang::LateBindVar;
using xziar::nailang::ArrarRef;
using xziar::nailang::BinaryExpr;
using xziar::nailang::UnaryExpr;
using xziar::nailang::SubQuery;
using xziar::nailang::QueryExpr;
using xziar::nailang::FuncCall;
using xziar::nailang::Expr;
using xziar::nailang::CustomVar;
using xziar::nailang::Arg;
using xziar::nailang::NativeWrapper;
using xziar::nailang::NilCheck;
using xziar::nailang::AssignExpr;
using xziar::nailang::Block;
using xziar::nailang::EvalTempStore;
using xziar::nailang::NailangFrame;
using xziar::nailang::NailangExecutor;
using xziar::nailang::NailangRuntime;
using xziar::nailang::EvaluateContext;
using xziar::nailang::BasicEvaluateContext;
using xziar::nailang::LargeEvaluateContext;
using xziar::nailang::CompactEvaluateContext;


testing::AssertionResult CheckArg(const Arg& arg, const Arg::Type type)
{
    if (arg.TypeData == type)
        return testing::AssertionSuccess();
    else
    {
        const auto detail = common::str::to_string(FMTSTR(U"arg is [{}]({:#04x})"sv,
            arg.GetTypeName(), common::enum_cast(arg.TypeData)), common::str::Charset::ASCII);
        return testing::AssertionFailure() << detail;
    }
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
class NailangRT : public xziar::nailang::NailangBasicRuntime
{
public:
    xziar::nailang::NailangFrameStack::FrameHolder<xziar::nailang::NailangFrame> BaseFrame;
    NailangRT() : NailangBasicRuntime(std::make_shared<EvalCtx>()),
        BaseFrame(Executor.PushFrame(RootContext, NailangFrame::FrameFlags::Empty))
    { }

    using NailangRuntime::RootContext;

    using NailangRuntime::LookUpArg;

    auto GetCtx() const { return std::dynamic_pointer_cast<EvalCtx>(RootContext); }
    auto SetRootArg(std::u32string_view name, Arg val)
    {
        return RootContext->LocateArg(name, true).Set(std::move(val));
        //return *RootContext->LocateArg(name, true) = std::move(val);
    }
    void Assign(const AssignExpr& assign)
    {
        EvalTempStore store;
        Executor.EvaluateAssign(assign, store, nullptr);
    }
    void QuickSetArg(std::u32string_view name, Expr val, bool create = false)
    {
        ParserContext context(name);
        NailangParser parser(MemPool, context);
        const auto var = parser.ParseExprChecked(""sv, U""sv, NailangParser::AssignPolicy::AllowAny);
        NilCheck::Behavior notnull = create ? NilCheck::Behavior::Throw : NilCheck::Behavior::Pass,
            null = var.TypeData == Expr::Type::Var ? NilCheck::Behavior::Pass : NilCheck::Behavior::Throw;
        AssignExpr assign(var, val, NilCheck(notnull, null).Value, false);
        Assign(assign);
    }
    Arg QuickGetArg(std::u32string_view name)
    {
        ParserContext context(name);
        const auto var = xziar::nailang::NailangParser::ParseSingleExpr(MemPool, context, ""sv, U""sv);
        EvalTempStore store;
        return Executor.EvaluateExpr(var, store);
    }
    Arg EvaluateExpr(const Expr& arg) 
    {
        EvalTempStore store;
        return Executor.EvaluateExpr(arg, store);
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
#define TEST_BIN(l, r, op, type, ans) do                        \
{                                                               \
    Arg left(l), right(r);                                      \
    const auto ret = left.HandleBinary(EmbedOps::op, right);    \
    CHECK_ARG(ret, type, ans);                                  \
} while(0)                                          

#define TEST_UN(val, op, type, ans) do              \
{                                                   \
    Arg arg(val);                                   \
    const auto ret = arg.HandleUnary(EmbedOps::op); \
    CHECK_ARG(ret, type, ans);                      \
} while(0)                                          \

    const Arg infP =  std::numeric_limits<double>::infinity();
    const Arg infN = -std::numeric_limits<double>::infinity();
    const Arg nanQ =  std::numeric_limits<double>::quiet_NaN();

    TEST_BIN(uint64_t(1), uint64_t(2), Equal,          Bool, false);
    TEST_BIN(uint64_t(1), uint64_t(1), Equal,          Bool, true);
    TEST_BIN(       nanQ, int64_t(-2), Equal,          Bool, false);
    TEST_BIN(       nanQ,        nanQ, Equal,          Bool, false);
    TEST_BIN(       true,        true, Equal,          Bool, true);
    TEST_BIN(uint64_t(0), uint64_t(0), NotEqual,       Bool, false);
    TEST_BIN(uint64_t(0), uint64_t(1), NotEqual,       Bool, true);
    TEST_BIN(       nanQ,        nanQ, NotEqual,       Bool, true);
    TEST_BIN(      false,        true, NotEqual,       Bool, true);
    TEST_BIN(uint64_t(1), uint64_t(2), Add,            Uint, 3u);
    TEST_BIN(uint64_t(1), int64_t(-1), Add,            Uint, 0u);
    TEST_BIN(uint64_t(1),        true, Add,            Uint, 2u);
    TEST_BIN(uint64_t(1),         0.5, Sub,              FP, 0.5);
    TEST_BIN( int64_t(5), int64_t(-1), Mul,             Int, -5);
    TEST_BIN(uint64_t(3), uint64_t(2), Div,            Uint, 1u);
    TEST_BIN(uint64_t(3),         0.5, Div,              FP, 6);
    TEST_BIN(uint64_t(3), uint64_t(2), Rem,            Uint, 1u);
    TEST_BIN(        2.5,         2.0, Rem,              FP, 0.5);
    TEST_BIN(   U"abc"sv,    U"ABC"sv, Add,          U32Str, U"abcABC"sv);
    TEST_BIN(       infP, uint64_t(2), Less,           Bool, false);
    TEST_BIN(       infN, int64_t(-2), Less,           Bool, true);
    TEST_BIN(uint64_t(1), uint64_t(2), Less,           Bool, true);
    TEST_BIN(int64_t(-1), uint64_t(2), Less,           Bool, true);
    TEST_BIN(uint64_t(1), int64_t(-2), Less,           Bool, false);
    TEST_BIN(uint64_t(1), int64_t(-2), LessEqual,      Bool, false);
    TEST_BIN(       -2.0, int64_t(-2), GreaterEqual,   Bool, true);
    TEST_BIN(       -2.0,       U""sv, And,            Bool, false);
    TEST_BIN(       true,       U""sv, Or,             Bool, true);
    TEST_BIN(       true,       false, And,            Bool, false);
    TEST_BIN(       true,       false, Or,             Bool, true);
    TEST_BIN(uint64_t(0x8), int64_t(0x4), BitOr,       Uint, 0xc);
    TEST_BIN(uint64_t(0xc), uint64_t(0x4), BitXor,     Uint, 0x8);
    TEST_UN (-2.0,  Not, Bool, false);
    TEST_UN (U""sv, Not, Bool, true);
    TEST_UN (uint64_t(0x80706050), BitNot, Uint, ~uint64_t(0x80706050));
    EXPECT_EQ(nanQ.HandleBinary(EmbedOps::Less, int64_t(-2)).TypeData, Arg::Type::Empty);
    EXPECT_EQ(Arg(true).HandleBinary(EmbedOps::Less, false).TypeData, Arg::Type::Empty);

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
    NailangRT runtime;
    const auto ParseEval = [&](const std::u32string_view src)
    {
        ParserContext context(src);
        NailangParser parser(pool, context);
        const auto [rawarg, delim] = parser.ParseExpr("");
        EXPECT_EQ(delim, common::parser::special::CharEnd);
        return runtime.EvaluateExpr(rawarg);
    };
    {
        const auto arg = ParseEval(U"!false"sv);
        CHECK_ARG(arg, Bool, true);
    }
    {
        const auto arg = ParseEval(U"!3.5"sv);
        CHECK_ARG(arg, Bool, false);
    }
    {
        const auto arg = ParseEval(U"!\"hero\""sv);
        CHECK_ARG(arg, Bool, false);
    }
    {
        const auto arg = ParseEval(U"1+2"sv);
        CHECK_ARG(arg, Int, 3);
    }
    {
        const auto arg = ParseEval(U"4u-4.5"sv);
        CHECK_ARG(arg, FP, -0.5);
    }
    {
        const auto arg = ParseEval(U"3*-3"sv);
        CHECK_ARG(arg, Int, -9);
    }
    {
        const auto arg = ParseEval(U"5/6"sv);
        CHECK_ARG(arg, Int, 0);
    }
    {
        const auto arg = ParseEval(U"7%3"sv);
        CHECK_ARG(arg, Int, 1);
    }
    { 
        const auto arg = ParseEval(U"12.5 % 6"sv);
        CHECK_ARG(arg, FP, 0.5);
    }
    {
        const auto arg = ParseEval(U"(\"abc\" + \"ABC\")"sv);
        CHECK_ARG(arg, U32Str, U"abcABC"sv);
    }
    {
        const auto arg = ParseEval(U"true ? 1 :2"sv);
        CHECK_ARG(arg, Int, 1);
    }
    {
        const auto arg = ParseEval(U"notexists??3"sv);
        CHECK_ARG(arg, Int, 3);
    }
    {
        const auto arg1 = ParseEval(U"?notexist"sv);
        CHECK_ARG(arg1, Bool, false);
        runtime.SetRootArg(U"notexist", false);
        const auto arg2 = ParseEval(U"?notexist"sv);
        CHECK_ARG(arg2, Bool, true);
        runtime.SetRootArg(U"notexist", {});
    }
    {
        runtime.SetRootArg(U"tmp1", int64_t(-512));
        const auto arg1 = ParseEval(U"tmp1 ?? notexist"sv);
        CHECK_ARG(arg1, Int, -512);
        const auto arg2 = ParseEval(U"notexist ?? \"Error\""sv);
        CHECK_ARG(arg2, U32Sv, U"Error"sv);
        runtime.SetRootArg(U"notexist", 3.0);
        const auto arg3 = ParseEval(U"notexist ?? tmp2"sv);
        CHECK_ARG(arg3, FP, 3.0);
        runtime.SetRootArg(U"notexist", {});
    }
}


struct ArrayCustomVar final : public xziar::nailang::CustomVar::Handler
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
    void IncreaseRef(const CustomVar& var) noexcept override
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
    [[nodiscard]] Arg HandleSubFields(const CustomVar& var, SubQuery<const Expr>& subfields) override
    {
        Expects(subfields.Size() > 0);
        ArrRef(arr, var);
        const auto field = subfields[0].GetVar<Expr::Type::Str>();
        if (field == U"Length"sv)
        {
            subfields.Consume();
            return static_cast<uint64_t>(arr.Data.size());
        }
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
            subfields.Consume();
            return NativeWrapper::GetLocator(NativeWrapper::Type::F32, reinterpret_cast<uintptr_t>(ptr), false, 0);
        }
        return {};
    }
    [[nodiscard]] Arg HandleIndexes(const CustomVar& var, SubQuery<Arg>& indexes) override
    {
        Expects(indexes.Size() > 0);
        ArrRef(arr, var);
        const auto idx = xziar::nailang::NailangHelper::BiDirIndexCheck(arr.Data.size(), indexes[0], &indexes.GetRaw(0));
        indexes.Consume();
        return NativeWrapper::GetLocator(NativeWrapper::Type::F32, reinterpret_cast<uintptr_t>(arr.Data.data()), false, idx);
    }
    common::str::StrVariant<char32_t> ToString(const CustomVar&) noexcept override { return U"{ArrayCustomVar}"; };
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

// wrapper for gsl::span due to lack of const_iterator
// https://github.com/google/googletest/issues/3194
template<typename T, size_t N>
struct SpanWrapper : public common::span<T, N>
{
    using const_iterator = typename common::span<T, N>::iterator;
};
template<typename T, size_t N>
static constexpr SpanWrapper<T, N> Span(const common::span<T, N> span) noexcept 
{ return {span}; }


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
        const auto data = Span(ArrayCustomVar::GetData(var));
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
        const auto data = Span(ArrayCustomVar::GetData(var));

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
        const auto arg = ParseEval(U"tmp[-2][0];"sv);
        CHECK_ARG(arg, U32Sv, U"l"sv);
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


TEST(NailangRuntime, ArrarRef)
{
    MemoryPool pool;
    NailangRT runtime;
    constexpr float dummy1[] = { 0.f,1.f,4.f,-2.f };
    int8_t dummy2[] = { 0,1,4,-2 };
    runtime.SetRootArg(U"arr1", ArrarRef::Create<const float>(dummy1));
    runtime.SetRootArg(U"arr2", ArrarRef::Create<int8_t>(dummy2));
    {
        const auto arg = runtime.QuickGetArg(U"arr1"sv);
        ASSERT_TRUE(CheckArg(arg, Arg::Type::Array | Arg::Type::ConstBit));
        const auto var = arg.GetVar<Arg::Type::Array>();
        ASSERT_EQ(var.ElementType, ArrarRef::Type::F32);
        ASSERT_TRUE(var.IsReadOnly);
        const auto sp  = var.GetSpan();
        ASSERT_TRUE(std::holds_alternative<common::span<const float>>(sp));
        const auto data = Span(std::get<common::span<const float>>(sp));
        EXPECT_THAT(data, testing::ElementsAreArray(dummy1));
    }
    {
        const auto arg = runtime.QuickGetArg(U"arr2"sv);
        ASSERT_TRUE(CheckArg(arg, Arg::Type::Array));
        const auto var = arg.GetVar<Arg::Type::Array>();
        ASSERT_EQ(var.ElementType, ArrarRef::Type::I8);
        ASSERT_FALSE(var.IsReadOnly);
        const auto sp = var.GetSpan();
        ASSERT_TRUE(std::holds_alternative<common::span<int8_t>>(sp));
        const auto data = Span(std::get<common::span<int8_t>>(sp));
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
        const auto data = Span(std::get<common::span<int8_t>>(var.GetSpan()));
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
        const auto arg1 = ParseEval(U"$Exists(\"notexist\");"sv);
        CHECK_ARG(arg1, Bool, false);
        runtime.SetRootArg(U"notexist", false);
        const auto arg2 = ParseEval(U"$Exists(\"notexist\");"sv);
        CHECK_ARG(arg2, Bool, true);
        runtime.SetRootArg(U"notexist", {});
        const auto arg3 = ParseEval(U"$Exists(\"notexist\");"sv);
        CHECK_ARG(arg3, Bool, false);
        const auto arg4 = ParseEval(U"$Exists(\"tmp\" + \"1\");"sv);
        CHECK_ARG(arg4, Bool, true);
        const auto arg5 = ParseEval(U"$Exists(\"tmp\" + \"3\");"sv);
        CHECK_ARG(arg5, Bool, false);
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
    static Block GetBlock(MemoryPool& pool, const std::u32string_view src)
    {
        common::parser::ParserContext context(src);
        BlkParser parser(pool, context);
        Block ret;
        parser.ParseContentIntoBlock(true, ret);
        return ret;
    }
};

static void PEAssign(NailangRT& runtime, MemoryPool& pool, const std::u32string_view src)
{
    ParserContext context(src);
    NailangParser parser(pool, context);
    const auto var = parser.ParseExprChecked(""sv, U""sv, NailangParser::AssignPolicy::AllowAny);
    if (var.TypeData != Expr::Type::Assign)
        throw "need assign expr";
    runtime.Assign(*var.GetVar<Expr::Type::Assign>());
}

TEST(NailangRuntime, Assign)
{
    MemoryPool pool;
    NailangRT runtime;
    {
        PEAssign(runtime, pool, U"str :=\"Hello \""sv);
        LOOKUP_ARG(runtime, U"str"sv, U32Sv, U"Hello "sv);
    }
    {
        EXPECT_THROW(PEAssign(runtime, pool, U"str :=1"sv), xziar::nailang::NailangRuntimeException);
    }
    {
        EXPECT_NO_THROW(PEAssign(runtime, pool, U"str ?=$Throw()"sv)); // should ignore the evaluation so won't throw
    }
    {
        PEAssign(runtime, pool, U"str += \"World\""sv);
        LOOKUP_ARG(runtime, U"str"sv, U32Str, U"Hello World"sv);
    }
    {
        constexpr auto ans = (63 % 4) * (3 + 5.0);
        PEAssign(runtime, pool, U"ans := (63 % 4) * (3 + 5.0)"sv);
        LOOKUP_ARG(runtime, U"ans"sv, FP, ans);
    }
    {
        const auto ans = std::fmod(24.0, (2 + 3.0)/2);
        PEAssign(runtime, pool, U"ans %= (valI64 + valF64)/valI64"sv);
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
        EXPECT_THAT(Span(func.ArgNames), testing::ElementsAre(U"num"sv, U"length"sv));
    }
}


uint64_t Run2Arg(NailangRT& runtime, const Block& algoBlock, uint64_t m, uint64_t n)
{
    auto ctx = std::make_shared<CompactEvaluateContext>();
    ctx->LocateArg(U"m"sv, true).Set(m);
    ctx->LocateArg(U"n"sv, true).Set(n);
    runtime.ExecuteBlock(algoBlock, {}, ctx);
    auto ans = ctx->LocateArg(U"m"sv, false);
    ans.Decay();
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

    EXPECT_EQ(Run2Arg(runtime, algoBlock,  5u, 5u), std::gcd( 5u, 5u));
    EXPECT_EQ(Run2Arg(runtime, algoBlock, 15u, 5u), std::gcd(15u, 5u));
    EXPECT_EQ(Run2Arg(runtime, algoBlock, 17u, 5u), std::gcd(17u, 5u));
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

    EXPECT_EQ(Run2Arg(runtime, algoBlock,  5u, 5u), std::gcd(5u, 5u));
    EXPECT_EQ(Run2Arg(runtime, algoBlock, 15u, 5u), std::gcd(15u, 5u));
    EXPECT_EQ(Run2Arg(runtime, algoBlock, 17u, 5u), std::gcd(17u, 5u));
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

    EXPECT_EQ(Run2Arg(runtime, algoBlock,  5u, 5u), std::gcd(5u, 5u));
    EXPECT_EQ(Run2Arg(runtime, algoBlock, 15u, 5u), std::gcd(15u, 5u));
    EXPECT_EQ(Run2Arg(runtime, algoBlock, 17u, 5u), std::gcd(17u, 5u));
}

TEST(NailangRuntime, sumOdd)
{
    MemoryPool pool;
    NailangRT runtime;

    constexpr auto sumTxt = UR"(
:sum := 0;
//:txt := "";
@While(m < n)
#Block("")
{
    @If(m % 2)
    #Block("")
    {
        m += 1;
        $Continue();
    }
    //txt += $Format("sum[{}] + m[{}]\n", sum, m);
    sum += m;
    m += 1;
}
m = sum;
)"sv;

    constexpr auto refSum = [](auto m, auto n)
    {
        uint32_t sum = 0;
        while (m < n)
        {
            if (m % 2)
            {
                m += 1;
                continue;
            }
            sum += m;
            m += 1;
        }
        m = sum;
        return m;
    };

    EXPECT_EQ(refSum(0u, 10u), 20u);
    EXPECT_EQ(refSum(1u, 10u), 20u);
    EXPECT_EQ(refSum(3u, 10u), 18u);

    const auto algoBlock = BlkParser::GetBlock(pool, sumTxt);

    ASSERT_EQ(algoBlock.Size(), 3u);

    EXPECT_EQ(Run2Arg(runtime, algoBlock, 0u, 10u), 20u);
    EXPECT_EQ(Run2Arg(runtime, algoBlock, 1u, 10u), 20u);
    EXPECT_EQ(Run2Arg(runtime, algoBlock, 3u, 10u), 18u);
}

TEST(NailangRuntime, NOr2M)
{
    MemoryPool pool;
    NailangRT runtime;

    constexpr auto NOr2MTxt = UR"(
@If(m > n)
    m = n;
@Else()
    m = 2 * m;
)"sv;

    constexpr auto refNOr2M = [](auto m, auto n)
    {
        if (m > n)
            m = n;
        else
            m = 2 * m;
        return m;
    };

    EXPECT_EQ(refNOr2M(5u, 4u),  4u);
    EXPECT_EQ(refNOr2M(5u, 5u), 10u);
    EXPECT_EQ(refNOr2M(4u, 5u),  8u);

    const auto algoBlock = BlkParser::GetBlock(pool, NOr2MTxt);

    ASSERT_EQ(algoBlock.Size(), 2u);

    EXPECT_EQ(Run2Arg(runtime, algoBlock, 5u, 4u),  4u);
    EXPECT_EQ(Run2Arg(runtime, algoBlock, 5u, 5u), 10u);
    EXPECT_EQ(Run2Arg(runtime, algoBlock, 4u, 5u),  8u);
}