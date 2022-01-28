#include "NailangPch.h"
#include "NailangParser.h"
#include "NailangParserRely.h"
#include "SystemCommon/StringFormat.h"
#include <boost/container/small_vector.hpp>

namespace xziar::nailang
{
using tokenizer::NailangToken;

COMMON_EXCEPTION_IMPL(NailangParseException)
COMMON_EXCEPTION_IMPL(NailangPartedNameException)
COMMON_EXCEPTION_IMPL(UnexpectedTokenException)


void DoThrow()
{
    throw NailangPartedNameException(u"a", U"b", U"c");
}

std::u16string NailangParser::DescribeTokenID(const uint16_t tid) const noexcept
{
#define RET_TK_ID(type) case NailangToken::type:        return u ## #type
    switch (static_cast<NailangToken>(tid))
    {
        RET_TK_ID(Raw);
        RET_TK_ID(Block);
        RET_TK_ID(MetaFunc);
        RET_TK_ID(Func);
        RET_TK_ID(Var);
        RET_TK_ID(SubField);
        RET_TK_ID(OpSymbol);
        RET_TK_ID(Parenthese);
        RET_TK_ID(SquareBracket);
        RET_TK_ID(CurlyBrace);
        RET_TK_ID(Assign);
#undef RET_TK_ID
    default:
        return ParserBase::DescribeTokenID(tid);
    }
}

common::str::StrVariant<char16_t> NailangParser::GetCurrentFileName() const noexcept
{
    if (SubScopeName.empty())
        return ParserBase::GetCurrentFileName();

    std::u16string fileName;
    fileName.reserve(Context.SourceName.size() + SubScopeName.size() + 3);
    fileName.append(Context.SourceName).append(u" ["sv).append(SubScopeName).append(u"]");
    return fileName;
}

void NailangParser::HandleException(const NailangParseException& ex) const
{
    auto& info = ex.GetInfo();
    info.File = GetCurrentFileName().StrView();
    if (info.Position.first == 0 && info.Position.second == 0)
        info.Position = { Context.Row + 1, Context.Col };
    if (ex.GetDetailMessage().empty())
        ex.Attach("detail", FMTSTR(u"at row[{}] col[{}], file [{}]", info.Position.first, info.Position.second, info.File));
    ex.ThrowSelf();
}

#define NLPS_THROW_EX(...) HandleException(CREATE_EXCEPTION(NailangParseException, __VA_ARGS__))

common::parser::DetailToken NailangParser::OnUnExpectedToken(const common::parser::DetailToken& token, const std::u16string_view extraInfo) const
{
    const auto msg = fmt::format(FMT_STRING(u"Unexpected token [{}] at [{},{}]{}{}"sv),
        DescribeToken(token), token.Row, token.Col, extraInfo.empty() ? u' ' : u',', extraInfo);
    HandleException(UnexpectedTokenException(msg, token));
    return token;
}

struct ExpectParentheseL
{
    static constexpr ParserToken Token = ParserToken(NailangToken::Parenthese, U'(');
};
struct ExpectParentheseR
{
    static constexpr ParserToken Token = ParserToken(NailangToken::Parenthese, U')');
};
struct ExpectCurlyBraceL
{
    static constexpr ParserToken Token = ParserToken(NailangToken::CurlyBrace, U'{');
};
struct ExpectCurlyBraceR
{
    static constexpr ParserToken Token = ParserToken(NailangToken::CurlyBrace, U'}');
};
struct ExpectSemoColon
{
    static constexpr ParserToken Token = ParserToken(BaseToken::Delim, U';');
};
void NailangParser::EatLeftParenthese()
{
    EatSingleToken<ExpectParentheseL, tokenizer::ParentheseTokenizer>();
}
void NailangParser::EatRightParenthese()
{
    EatSingleToken<ExpectParentheseR, tokenizer::ParentheseTokenizer>();
}
void NailangParser::EatLeftCurlyBrace()
{
    EatSingleToken<ExpectCurlyBraceL, tokenizer::CurlyBraceTokenizer>();
}
void NailangParser::EatRightCurlyBrace()
{
    EatSingleToken<ExpectCurlyBraceR, tokenizer::CurlyBraceTokenizer>();
}
void NailangParser::EatSemiColon()
{
    using SemiColonTokenizer = tokenizer::KeyCharTokenizer<true, BaseToken::Delim, U';'>;
    EatSingleToken<ExpectSemoColon, SemiColonTokenizer>();
}

FuncName* NailangParser::CreateFuncName(std::u32string_view name, FuncName::FuncInfo info) const
{
    info = FuncName::PrepareFuncInfo(name, info);
    if (name.empty())
        NLPS_THROW_EX(u"Does not allow empty function name"sv);
    try
    {
        return FuncName::Create(MemPool, name, info);
    }
    catch (const NailangPartedNameException&)
    {
        HandleException(NailangParseException(u"FuncCall's name not valid"sv));
    }
    return nullptr;
}

void NailangParser::FillBlockName(RawBlock& block)
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    constexpr auto ExpectString = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, BaseToken::String);

    EatLeftParenthese();
    constexpr auto NameLexer = ParserLexerBase<CommentTokenizer, StringTokenizer>();
    const auto sectorNameToken = ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectString);
    block.Name = sectorNameToken.GetString();
    EatRightParenthese();
}

void NailangParser::FillFileName(RawBlock& block) const noexcept
{
    block.FileName = Context.SourceName;
}

FuncCall NailangParser::ParseFuncCall(std::u32string_view name, std::pair<uint32_t, uint32_t> pos, FuncName::FuncInfo info)
{
    const auto funcName = CreateFuncName(name, info);
    EatLeftParenthese();
    std::vector<Expr> args;
    while (true)
    {
        const auto [arg, delim] = ParseExpr(",)"sv);
        if (arg)
            args.emplace_back(arg);
        else
            if (delim != U')')
                NLPS_THROW_EX(u"Does not allow empty argument"sv);
        if (delim == U')')
            break;
        if (delim == common::parser::special::CharEnd)
            NLPS_THROW_EX(u"Expect ')' before reaching end"sv);
    }
    const auto sp = MemPool.CreateArray(args);
    return { funcName, sp, pos };
}

RawBlock NailangParser::ParseRawBlock(const std::u32string_view name, std::pair<uint32_t, uint32_t> pos)
{
    RawBlock block;
    block.Position = pos;
    block.Type = name;

    FillBlockName(block);

    EatLeftCurlyBrace();

    ContextReader reader(Context);
    auto guardString = std::u32string(reader.ReadLine());
    FillFileName(block);

    guardString.append(U"}");
    block.Source = reader.ReadUntil(guardString);
    block.Source.remove_suffix(guardString.size());
    return block;
}

struct ExprOp
{
    uint32_t Val = UINT32_MAX;
    [[nodiscard]] constexpr explicit operator bool() const noexcept 
    { 
        return Val != UINT32_MAX;
    }
    [[nodiscard]] constexpr EmbedOps GetEmbedOp() const noexcept 
    {
        Expects(Val <= 255);
        return static_cast<EmbedOps>(Val);
    }
    [[nodiscard]] constexpr ExtraOps GetExtraOp() const noexcept
    {
        Expects(Val >= 256 && Val <= 383);
        return static_cast<ExtraOps>(Val);
    }
    [[nodiscard]] constexpr AssignOps GetAssignOp() const noexcept
    {
        Expects(Val >= 384 && Val <= 511);
        return static_cast<AssignOps>(Val);
    }
    template<typename T>
    [[nodiscard]] constexpr bool operator==(T val) const noexcept
    {
        static_assert(std::is_enum_v<T>);
        return Val == common::enum_cast(val);
    }
    template<typename T>
    [[nodiscard]] constexpr bool operator!=(T val) const noexcept
    {
        static_assert(std::is_enum_v<T>);
        return Val != common::enum_cast(val);
    }
    constexpr void operator=(uint64_t val) noexcept 
    {
        Val = static_cast<uint32_t>(val);
    }
    enum class Category : uint32_t { Binary = 0, Unary = 1, Ternary = 2, Assign = 3, Empty };
    [[nodiscard]] static constexpr Category ParseCategory(uint32_t val) noexcept
    {
        return static_cast<Category>(std::min(val >> 7, 4u));
    }
    [[nodiscard]] constexpr Category GetCategory() const noexcept
    {
        return ParseCategory(Val);
    }

};

[[nodiscard]] static constexpr bool CheckExprLiteral(const Expr& opr)
{
    switch (opr.TypeData)
    {
    case Expr::Type::Query:
    case Expr::Type::Func:
    case Expr::Type::Unary:
    case Expr::Type::Binary:
    case Expr::Type::Var:
        return false;
    default: // including assign since should not be here
        return true;
    }
}


struct ExprStack
{
    struct ExprFrame
    {
        Expr Vals[2];
        ExprOp Op;
        uint8_t OprIdx = 0;
    };
    MemoryPool& Pool;
    boost::container::small_vector<ExprFrame, 4> Stack;
    boost::container::small_vector<Expr, 4> Queries;
    Expr* TargetExpr = nullptr; // insert new frame always causes a flush, or the expr is in pool, so safe to use ptr
    QueryExpr::QueryType QType = QueryExpr::QueryType::Index;
    ExprStack(MemoryPool& pool) : Pool(pool) { }
    void DoCommitQuery()
    {
        Expects(TargetExpr && *TargetExpr);
        const auto sp = Pool.CreateArray(Queries);
        *TargetExpr = Pool.Create<QueryExpr>(*TargetExpr, sp, QType);
        Queries.clear();
    }
    [[nodiscard]] bool PrecheckQuery() const noexcept
    {
        Expects(TargetExpr);
        if (Queries.empty()) // first query
        {
            Expects(*TargetExpr);
            return !CheckExprLiteral(*TargetExpr);
        }
        return true;
    }
    void PushSubField(std::u32string_view subField)
    {
        Expects(TargetExpr);
        if (!Queries.empty() && QType != QueryExpr::QueryType::Sub)
            DoCommitQuery();
        Queries.emplace_back(subField);
        QType = QueryExpr::QueryType::Sub;
    }
    void PushIndexer(const Expr& indexer)
    {
        Expects(TargetExpr);
        if (!Queries.empty() && QType != QueryExpr::QueryType::Index)
            DoCommitQuery();
        Queries.emplace_back(indexer);
        QType = QueryExpr::QueryType::Index;
    }
    void CommitQuery()
    {
        if (!Queries.empty())
            DoCommitQuery();
    }
    [[nodiscard]] std::u16string_view CheckExpr() const
    {
        if (Stack.empty())
            return {};
        const auto& frame = Stack.back();
        if (!Queries.empty())
            return u"need an operator before another operand"sv;
        switch (frame.Op.GetCategory())
        {
        case ExprOp::Category::Empty:
            Expects(frame.OprIdx == 1);
            return u"need an operator before another operand"sv;
        case ExprOp::Category::Unary:
            if (frame.OprIdx > 0)
                return u"already has 1 operand for unary operator"sv;
            break;
        case ExprOp::Category::Binary:
            if (frame.OprIdx > 1)
                return u"already has 2 operands for binary operator"sv;
            Expects(frame.OprIdx == 1);
            break;
        case ExprOp::Category::Ternary:
            if (frame.Op == ExtraOps::Quest)
            {
                if (frame.OprIdx > 1)
                    return u"expect ':' before 3rd operand for ternary operator"sv;
                Expects(frame.OprIdx == 1);
                break;
            }
            else
            {
                if (frame.OprIdx > 2)
                    return u"already has 3 operands for ternary operator"sv;
                Expects(frame.Op == ExtraOps::Colon && frame.OprIdx == 2);
                break;
            }
        default:
            Expects(false);
        }
        return {};
    }
    void CommitExpr(Expr expr)
    {
        auto updExpr = [&, upd = true](Expr* target) mutable
        {
            if (upd)
                TargetExpr = target, upd = false;
        };
        while (true)
        {
            if (Stack.empty())
            {
                auto& frame = Stack.emplace_back();
                frame.Vals[0] = expr;
                frame.OprIdx = 1;
                updExpr(&frame.Vals[0]);
                return;
            }
            Expects(Stack.back().Op);
            auto& frame = Stack.back();
            switch (frame.Op.GetCategory())
            {
            case ExprOp::Category::Unary:
            {
                Expects(frame.OprIdx == 0);
                const auto texpr = Pool.Create<UnaryExpr>(frame.Op.GetEmbedOp(), expr);
                updExpr(&texpr->Operand);
                expr = texpr;
                Stack.pop_back();
            } continue;
            case ExprOp::Category::Binary:
            {
                Expects(frame.OprIdx == 1);
                const auto texpr = Pool.Create<BinaryExpr>(frame.Op.GetEmbedOp(), frame.Vals[0], expr);
                updExpr(&texpr->RightOperand);
                expr = texpr;
                Stack.pop_back();
            } continue;
            case ExprOp::Category::Ternary:
                if (frame.Op == ExtraOps::Quest)
                {
                    Expects(frame.OprIdx == 1);
                    auto& target = frame.Vals[frame.OprIdx++];
                    target = expr;
                    updExpr(&target);
                    return;
                }
                else
                {
                    Expects(frame.Op == ExtraOps::Colon && frame.OprIdx == 2);
                    const auto texpr = Pool.Create<TernaryExpr>(frame.Vals[0], frame.Vals[1], expr);
                    updExpr(&texpr->RightOperand);
                    expr = texpr;
                    Stack.pop_back();
                    continue;
                }
            default:
                Expects(false);
            }
        }
    }
    [[nodiscard]] std::u16string_view AllocUnary(ExprOp op)
    {
        if (op == ExtraOps::Quest)
            op = common::enum_cast(EmbedOps::CheckExist);
        else if (op.GetCategory() != ExprOp::Category::Unary)
            return u"only unary operator allowed here"sv;
        auto& frame = Stack.emplace_back();
        frame.Op = op;
        return {};
    }
    [[nodiscard]] std::u16string_view CheckAssignOp() const
    {
        if (Stack.size() > 1)
            return u"assign operator not allowed in expr"sv;
        if (!TargetExpr)
            return u"expect an expr before assign operator"sv;
        Expects(!Stack.empty());
        auto& frame = Stack.back();
        if (frame.OprIdx != 1 || frame.Op)
            return u"assign operator not allowed in expr"sv;
        Expects(*TargetExpr);
        if (CheckExprLiteral(*TargetExpr))
            return u"Assign should not follow a litteral type"sv;
        return {};
    }
    [[nodiscard]] std::u16string_view DoCheckCommitOp(ExprOp op)
    {
        if (Stack.empty())
            return AllocUnary(op);
        const auto cat = op.GetCategory();
        auto& frame = Stack.back();
        switch (frame.Op.GetCategory())
        {
        case ExprOp::Category::Empty:
            Expects(frame.OprIdx == 1); // has 1 operand
            if (cat != ExprOp::Category::Binary && cat != ExprOp::Category::Ternary)
                return u"only binary/ternary operator allowed when there's already 1 operand"sv;
            CommitQuery();
            frame.Op = op; // simply assign op
            break;
        case ExprOp::Category::Unary:
            Expects(frame.OprIdx == 0); // should be 0 operand
            return AllocUnary(op);
        case ExprOp::Category::Binary:
            Expects(frame.OprIdx == 1); // should be already 1 operand
            return AllocUnary(op);
        case ExprOp::Category::Ternary:
        {
            const uint8_t oprCnt = frame.Op == ExtraOps::Quest ? 1 : 2;
            if (frame.OprIdx == oprCnt) // waiting for an operand
                return AllocUnary(op);
            else // already has an operand
            {
                Expects(frame.OprIdx == oprCnt + 1);
                if (cat != ExprOp::Category::Binary && cat != ExprOp::Category::Ternary)
                    return u"only binary/ternary operator allowed when there's already 1 operand"sv;
                CommitQuery();
                if (op == ExtraOps::Colon)
                {
                    if (frame.Op == ExtraOps::Quest)
                        frame.Op = op; // simply assign op
                    else
                        return u"colon need to follow an existsing terary '?' operator"sv;
                }
                else
                {
                    // move current to next level
                    const auto expr = frame.Vals[--frame.OprIdx];
                    frame.Vals[frame.OprIdx] = {};
                    auto& newFrame = Stack.emplace_back();
                    newFrame.Op = op;
                    newFrame.Vals[0] = expr;
                    newFrame.OprIdx = 1;
                }
            }
        } break;
        default:
            Expects(false);
        }
        return {};
    }
    [[nodiscard]] std::u16string_view CheckCommitOp(ExprOp op)
    {
        const auto ret = DoCheckCommitOp(op);
        TargetExpr = nullptr;
        return ret;
    }
};


std::pair<Expr, char32_t> NailangParser::ParseExpr(std::string_view stopDelim, AssignPolicy policy)
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;

    const DelimTokenizer StopTokenizer = stopDelim;
    const auto ArgLexer = ParserLexerBase<CommentTokenizer,
        DelimTokenizer, tokenizer::ParentheseTokenizer, tokenizer::NormalFuncPrefixTokenizer,
        StringTokenizer, IntTokenizer, FPTokenizer, BoolTokenizer,
        tokenizer::VariableTokenizer, tokenizer::OpSymbolTokenizer,
        tokenizer::SubFieldTokenizer, tokenizer::SquareBracketTokenizer>(StopTokenizer);
    const auto OpLexer = ParserLexerBase<CommentTokenizer, DelimTokenizer,
        tokenizer::OpSymbolTokenizer, tokenizer::SubFieldTokenizer, tokenizer::SquareBracketTokenizer>(StopTokenizer);
    
    
    ExprStack stack(MemPool);
    char32_t stopChar = common::parser::special::CharEnd;

    bool shouldContinue = true;
    while (shouldContinue)
    {
        auto token = stack.TargetExpr ?
            GetNextToken(OpLexer, IgnoreBlank, IgnoreCommentToken) :
            GetNextToken(ArgLexer, IgnoreBlank, IgnoreCommentToken);

#define EID(id) common::enum_cast(id)
        switch (token.GetID())
        {
        case EID(BaseToken::Unknown):
        case EID(BaseToken::Error):
        {
            OnUnExpectedToken(token, stack.TargetExpr ? u"expect an operator"sv : u"unknown or error token"sv);
        } break;
        case EID(BaseToken::Delim):
        case EID(BaseToken::End):
        {
            stopChar = token.GetChar();
            shouldContinue = false;
        } break;
        case EID(NailangToken::Parenthese):
        {
            if (const auto err = stack.CheckExpr(); !err.empty())
                OnUnExpectedToken(token, err);
            if (token.GetChar() == U')')
                OnUnExpectedToken(token, u"Unexpected right parenthese"sv);
            stack.CommitExpr(ParseExprChecked(")"sv, U")"sv));
        } break;
        case EID(NailangToken::OpSymbol):
        {
            ExprOp op{ static_cast<uint32_t>(token.GetUInt()) };
            if (op.GetCategory() == ExprOp::Category::Assign)
            {
                if (policy == AssignPolicy::Disallow)
                    OnUnExpectedToken(token, u"does not allow assign operator here"sv);
                if (const auto err = stack.CheckAssignOp(); !err.empty())
                    OnUnExpectedToken(token, err);
                stack.CommitQuery();
                Expects(stack.Stack.size() == 1 && stack.Stack.back().OprIdx == 1);
                const auto& opr = stack.Stack.back().Vals[0];
                if (opr.TypeData != Expr::Type::Var)
                {
                    if (policy == AssignPolicy::AllowVar)
                        OnUnExpectedToken(token, FMTSTR(u"assign operator is only allowed after a var, get [{}]"sv, opr.GetTypeName()));
                    if (op == AssignOps::NewCreate || op == AssignOps::NilAssign)
                        OnUnExpectedToken(token, FMTSTR(u"NewCreate and NilAssign can only be applied after a var, get [{}]"sv, opr.GetTypeName()));
                }
                // direct construct AssignOp
                auto [stmt, ch] = ParseExpr(stopDelim);
                if (!stmt)
                    NLPS_THROW_EX(u"Lack operand for assign expr"sv);
                using Behavior = NilCheck::Behavior;
                bool isSelfAssign = false;
                uint8_t info = 0;
                switch (op.GetAssignOp())
                {
                case AssignOps::Assign:    info = NilCheck(Behavior::Pass,  Behavior::Pass).Value;          break;
                case AssignOps::NewCreate: info = NilCheck(Behavior::Throw, Behavior::Pass).Value;          break;
                case AssignOps::NilAssign: info = NilCheck(Behavior::Skip,  Behavior::Pass).Value;          break;
#define SELF_ASSIGN(tname) case AssignOps::tname##Assign: isSelfAssign = true; info = common::enum_cast(EmbedOps::tname); break
                SELF_ASSIGN(Add);
                SELF_ASSIGN(Sub);
                SELF_ASSIGN(Mul);
                SELF_ASSIGN(Div);
                SELF_ASSIGN(Rem);
                SELF_ASSIGN(BitAnd);
                SELF_ASSIGN(BitOr);
                SELF_ASSIGN(BitXor);
                SELF_ASSIGN(BitShiftLeft);
                SELF_ASSIGN(BitShiftRight);
#undef SELF_ASSIGN
                default: NLPS_THROW_EX(u"Unrecoginzied assign operator"sv);                                 break;
                }
                return { MemPool.Create<AssignExpr>(opr, stmt, info, isSelfAssign), ch };
            }
            if (const auto err = stack.CheckCommitOp(op); !err.empty())
                OnUnExpectedToken(token, err);
        } break;
        case EID(NailangToken::SquareBracket): // Indexer
        {
            if (token.GetChar() == U']')
                OnUnExpectedToken(token, u"Unexpected right square bracket"sv);
            if (!stack.TargetExpr)
                OnUnExpectedToken(token, u"Indexer should follow a Expr"sv);
            if (!stack.PrecheckQuery())
                OnUnExpectedToken(token, u"SubQuery should not follow a litteral type"sv);
            auto index = ParseExprChecked("]"sv, U"]"sv);
            if (!index)
                OnUnExpectedToken(token, u"lack of index"sv);
            stack.PushIndexer(index);
        } break;
        case EID(NailangToken::SubField): // SubField
        {
            if (!stack.TargetExpr)
                OnUnExpectedToken(token, u"SubField should follow a Expr"sv);
            if (!stack.PrecheckQuery())
                OnUnExpectedToken(token, u"SubQuery should not follow a litteral type"sv);
            stack.PushSubField(token.GetString());
        } break;
        default:
        {
            if (const auto err = stack.CheckExpr(); !err.empty())
                OnUnExpectedToken(token, err);
            Expr expr;
            switch (token.GetID())
            {
            case EID(BaseToken::Uint)    : expr = token.GetUInt();      break;
            case EID(BaseToken::Int)     : expr = token.GetInt();       break;
            case EID(BaseToken::FP)      : expr = token.GetDouble();    break;
            case EID(BaseToken::Bool)    : expr = token.GetBool();      break;
            case EID(NailangToken::Var)  :
                expr = LateBindVar(token.GetString());
            break;
            case EID(BaseToken::String)  :
                expr = ProcessString(token.GetString(), MemPool);
            break;
            case EID(NailangToken::Func) :
                expr = MemPool.Create<FuncCall>(ParseFuncCall(token, FuncName::FuncInfo::ExprPart));
            break;
            default: OnUnExpectedToken(token, u"Unrecognized token"sv);
            }
            stack.CommitExpr(expr);
        } break;
        }
#undef EID
    }
    switch (stack.Stack.size())
    {
    case 0:
        break;
    case 1:
        if (const auto& frame = stack.Stack.back(); frame.OprIdx == 1 && !frame.Op)
        {
            stack.CommitQuery();
            return { frame.Vals[0], stopChar };
        }
        [[fallthrough]];
    default:
        NLPS_THROW_EX(u"Incomplete expr"sv);
    }
    return { {}, stopChar };
}

Expr NailangParser::ParseExprChecked(std::string_view stopDelims, std::u32string_view stopChecker, AssignPolicy policy)
{
    auto [arg, delim] = ParseExpr(stopDelims, policy);
    if (!stopChecker.empty() && stopChecker.find(delim) == std::u32string_view::npos)
        NLPS_THROW_EX(FMTSTR(u"Ends with unexpected delim [{}], expects [{}]", delim, stopChecker));
    return arg;
}

void NailangParser::ParseContentIntoBlock(const bool allowNonBlock, Block& block, const bool tillTheEnd)
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;

    std::vector<Statement> contents;
    std::vector<FuncCall> allMetaFuncs;
    std::vector<FuncCall> metaFuncs;
    const auto AppendMetaFuncs = [&]() -> std::pair<uint32_t, uint16_t>
    {
        const auto offset = gsl::narrow_cast<uint32_t>(allMetaFuncs.size());
        const auto count  = gsl::narrow_cast<uint16_t>(   metaFuncs.size());
        allMetaFuncs.reserve(static_cast<size_t>(offset) + count);
        std::move(metaFuncs.begin(), metaFuncs.end(), std::back_inserter(allMetaFuncs));
        metaFuncs.clear();
        return { offset, count };
    };
    while (true)
    {
        using common::parser::DetailToken;
        const auto [expr, ch] = ParseExpr("@#};", AssignPolicy::AllowAny);
        Expects(Context.Col > 0u || ch == common::parser::special::CharEnd);
        const auto row = Context.Row, col = Context.Col - (ch == common::parser::special::CharEnd ? 0u : 1u);
        if (!expr)
        {
            switch (ch)
            {
            case U'@': // metafunc
            {
                ContextReader reader(Context);
                const DetailToken token = { row, col, tokenizer::MetaFuncPrefixTokenizer::GetToken(reader, U"@") };
                Ensures(token.GetIDEnum<NailangToken>() == NailangToken::MetaFunc);
                metaFuncs.emplace_back(ParseFuncCall(token, FuncName::FuncInfo::Meta));
            } continue;
            case U'#': // block/rawblock
            {
                ContextReader reader(Context);
                const DetailToken token = { row, col, tokenizer::BlockPrefixTokenizer::GetToken(reader, U"#") };
                if (token.GetIDEnum<NailangToken>() == NailangToken::Raw)
                {
                    const auto target = MemPool.Create<RawBlock>(ParseRawBlock(token));
                    contents.emplace_back(target, AppendMetaFuncs());
                }
                else
                {
                    Ensures(token.GetIDEnum<NailangToken>() == NailangToken::Block);
                    Block inlineBlk;
                    inlineBlk.Position = GetPosition(token);
                    FillBlockName(inlineBlk);
                    inlineBlk.Type = token.GetString();
                    EatLeftCurlyBrace();
                    {
                        ContextReader reader_(Context);
                        reader_.ReadLine();
                    }
                    FillFileName(inlineBlk);
                    {
                        const auto idxBegin = Context.Index;
                        ParseContentIntoBlock(true, inlineBlk, false);
                        const auto idxEnd = Context.Index;
                        inlineBlk.Source = Context.Source.substr(idxBegin, idxEnd - idxBegin - 1);
                    }
                    const auto target = MemPool.Create<Block>(inlineBlk);
                    contents.emplace_back(target, AppendMetaFuncs());
                }
            } continue;
            case U';':
            {
                const DetailToken token = { row, col, ParserToken(BaseToken::Delim, ch) };
                OnUnExpectedToken(token, u"empty statement not allowed"sv);
            } break;
            case U'}':
            {
                const DetailToken token = { row, col, ParserToken(BaseToken::Delim, ch) };
                if (tillTheEnd)
                    OnUnExpectedToken(token, u"when parsing block contents"sv);
                if (metaFuncs.size() > 0)
                    OnUnExpectedToken(token, u"expect block/assignment/funccall after metafuncs"sv);
            } break;
            default:
            {
                Expects(ch == common::parser::special::CharEnd);
                const DetailToken token = { row, col, ParserToken(BaseToken::End, ch) };
                /*if (token.GetIDEnum() != BaseToken::End)
                    OnUnExpectedToken(token, u"when parsing block contents"sv);*/
                if (metaFuncs.size() > 0)
                    OnUnExpectedToken(token, u"expect block/assignment/funccall after metafuncs"sv);
                if (!tillTheEnd)
                    OnUnExpectedToken(token, u"expect '}' to close the scope"sv);
            } break;
            }
        }
        else
        {
            if (ch != U';')
            {
                const DetailToken token = { row, col, ParserToken(BaseToken::Delim, ch) };
                OnUnExpectedToken(token, u"expect '}' to finish a statement"sv);
            }
            if (expr.TypeData == Expr::Type::Func)
            {
                const auto* funccall = expr.GetVar<Expr::Type::Func>();
                if (!allowNonBlock)
                {
                    const DetailToken token = { funccall->Position.first, funccall->Position.second,
                        ParserToken(NailangToken::Func, funccall->FullFuncName()) };
                    OnUnExpectedToken(token, u"Function call not supported here"sv);
                }
                const_cast<FuncName*>(funccall->Name)->Info() = FuncName::FuncInfo::Empty;
                contents.emplace_back(funccall, AppendMetaFuncs());
            }
            else if (expr.TypeData == Expr::Type::Assign)
            {
                const auto* assignexpr = expr.GetVar<Expr::Type::Assign>();
                if (!allowNonBlock)
                {
                    const DetailToken token = { assignexpr->Position.first, assignexpr->Position.second,
                        ParserToken(NailangToken::Func, U'=') };
                    OnUnExpectedToken(token, u"Variable assignment not supported here"sv);
                }
                contents.emplace_back(assignexpr, AppendMetaFuncs());
            }
            else
            {
                NLPS_THROW_EX(FMTSTR(u"Only support block/rawblock{} here, get [{}]"sv, allowNonBlock ? u"/assign/func"sv : u""sv,
                    expr.GetTypeName()));
            }
            continue;
        }
        break;
    }
    block.Content = MemPool.CreateArray(contents);
    block.MetaFuncations = MemPool.CreateArray(allMetaFuncs);
}

Expr NailangParser::ProcessString(const std::u32string_view str, MemoryPool& pool)
{
    Expects(str.size() == 0 || str.back() != U'\\');
    const auto idx = str.find(U'\\');
    if (idx == std::u32string_view::npos)
        return str;
    std::u32string output; output.reserve(str.size() - 1);
    output.assign(str.data(), idx);

    for (size_t i = idx; i < str.size(); ++i)
    {
        if (str[i] != U'\\')
            output.push_back(str[i]);
        else
        {
            switch (str[++i]) // promise '\' won't be last element.
            {
            case U'\\': output.push_back(U'\\');  break;
            case U'0' : output.push_back(U'\0');  break;
            case U'r' : output.push_back(U'\r');  break;
            case U'n' : output.push_back(U'\n');  break;
            case U't' : output.push_back(U'\t');  break;
            case U'"' : output.push_back(U'"' );  break;
            default   : output.push_back(str[i]); break;
            }
        }
    }
    const auto space = pool.CreateArray(output);
    return std::u32string_view(space.data(), space.size());
}

FuncCall NailangParser::ParseFuncBody(std::u32string_view name, MemoryPool& pool, common::parser::ParserContext& context, 
    FuncName::FuncInfo info)
{
    NailangParser parser(pool, context);
    return parser.ParseFuncCall(name, info);
}

Expr NailangParser::ParseSingleExpr(MemoryPool& pool, common::parser::ParserContext& context, std::string_view stopDelims, std::u32string_view stopChecker)
{
    NailangParser parser(pool, context);
    return parser.ParseExprChecked(stopDelims, stopChecker);
}

RawBlockWithMeta NailangParser::GetNextRawBlock()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
       
    constexpr auto ExpectRawOrMeta = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, NailangToken::Raw, NailangToken::MetaFunc);

    constexpr auto MainLexer = ParserLexerBase<CommentTokenizer, tokenizer::MetaFuncPrefixTokenizer, tokenizer::BlockPrefixTokenizer>();

    std::vector<FuncCall> metaFuncs;
    while (true)
    {
        const auto token = ExpectNextToken(MainLexer, IgnoreBlank, IgnoreCommentToken, ExpectRawOrMeta);
        switch (token.GetIDEnum<NailangToken>())
        {
        case NailangToken::Raw:
        {
            RawBlockWithMeta block;
            static_cast<RawBlock&>(block) = ParseRawBlock(token);
            block.MetaFunctions = MemPool.CreateArray(metaFuncs);
            return block;
        } break;
        case NailangToken::MetaFunc:
            metaFuncs.emplace_back(ParseFuncCall(token, FuncName::FuncInfo::Meta));
            break;
        default:
            Expects(false);
            break;
        }
    }
}

std::vector<RawBlockWithMeta> NailangParser::GetAllRawBlocks()
{
    std::vector<RawBlockWithMeta> sectors;
    while (true)
    {
        ContextReader reader(Context);
        reader.ReadWhile(IgnoreBlank);
        if (reader.PeekNext() == common::parser::special::CharEnd)
            break;
        sectors.emplace_back(GetNextRawBlock());
    }
    return sectors;
}

Block NailangParser::ParseRawBlock(const RawBlock& block, MemoryPool& pool)
{
    common::parser::ParserContext context(block.Source, block.FileName);
    std::tie(context.Row, context.Col) = block.Position;
    NailangParser parser(pool, context);

    Block ret;
    static_cast<RawBlock&>(ret) = block;
    parser.ParseContentIntoBlock(true, ret);
    return ret;
}

Block NailangParser::ParseAllAsBlock(MemoryPool& pool, common::parser::ParserContext& context)
{
    NailangParser parser(pool, context);

    Block ret;
    ret.Position = parser.GetCurPos(true);
    parser.FillFileName(ret);
    parser.ParseContentIntoBlock(false, ret);
    return ret;
}

std::optional<size_t> NailangParser::VerifyVariableName(const std::u32string_view name) noexcept
{
    constexpr tokenizer::VariableTokenizer Self = {};
    if (name.empty())
        return SIZE_MAX;
    uint32_t state = 0;
    for (size_t i = 0; i < name.size(); ++i)
    {
        const auto [newState, ret] = Self.OnChar(state, name[i], i);
        if (ret == TokenizerResult::NotMatch)
            return i;
        state = newState;
    }
    if (state == 0)
        return name.size() - 1;
    return {};
}


std::u32string_view ReplaceEngine::TrimStrBlank(const std::u32string_view str) noexcept
{
    size_t len = str.size();
    while (len--)
    {
        if (!IgnoreBlank(str[len]))
            break;
    }
    return str.substr(0, len + 1);
}

void ReplaceEngine::HandleException(const NailangParseException& ex) const
{
    /*ex.File = GetCurrentFileName().StrView();
    ex.Position = GetCurrentPosition();*/
    ex.ThrowSelf();
}

void ReplaceEngine::OnReplaceOptBlock(std::u32string&, void*, std::u32string_view, std::u32string_view)
{
    NLPS_THROW_EX(u"ReplaceOptBlock unimplemented"sv);
}
void ReplaceEngine::OnReplaceVariable(std::u32string&, void*, std::u32string_view)
{
    NLPS_THROW_EX(u"ReplaceVariable unimplemented"sv);
}
void ReplaceEngine::OnReplaceFunction(std::u32string&, void*, std::u32string_view, common::span<const std::u32string_view>)
{
    NLPS_THROW_EX(u"ReplaceFunction unimplemented"sv);
}

std::u32string ReplaceEngine::ProcessOptBlock(const std::u32string_view source, const std::u32string_view prefix, const std::u32string_view suffix, void* cookie)
{
    Expects(!prefix.empty() && !suffix.empty()); // Illegal prefix/suffix
    common::parser::ParserContext context(source);
    ContextReader reader(context);
    std::u32string output;
    while (true)
    {
        auto before = reader.ReadUntil(prefix);
        if (before.empty()) // reaching end
        {
            output.append(reader.ReadAll());
            break;
        }
        {
            before.remove_suffix(prefix.size());
            output.append(before);
        }
        reader.ReadWhile(IgnoreBlank);
        if (reader.ReadNext() != U'{')
            NLPS_THROW_EX(u"Expect '{' for cond"sv);
        reader.ReadWhile(IgnoreBlank);
        auto cond = reader.ReadUntil(U"}");
        cond.remove_suffix(1);
        reader.ReadLine();
        auto str = reader.ReadUntil(suffix);
        if (str.empty())
        {
            if (reader.IsEnd())
                NLPS_THROW_EX(u"End before block content"sv);
            else
                NLPS_THROW_EX(u"No suffix found!"sv);
        }
        str.remove_suffix(suffix.size());
        // find a opt block replacement
        OnReplaceOptBlock(output, cookie, TrimStrBlank(cond), str);
        reader.ReadLine();
    }
    return output;
}

std::u32string ReplaceEngine::ProcessVariable(const std::u32string_view source, const std::u32string_view prefix, const std::u32string_view suffix, void* cookie)
{
    Expects(!prefix.empty() && !suffix.empty()); // Illegal prefix/suffix
    common::parser::ParserContext context(source);
    ContextReader reader(context);
    std::u32string output;
    while (true)
    {
        auto before = reader.ReadUntil(prefix);
        if (before.empty()) // reaching end
        {
            output.append(reader.ReadAll());
            break;
        }
        {
            before.remove_suffix(prefix.size());
            output.append(before);
        }
        reader.ReadWhile(IgnoreBlank);
        auto var = reader.ReadUntil(suffix);
        if (var.empty())
        {
            if (reader.IsEnd())
                NLPS_THROW_EX(u"End before variable name"sv);
            else
                NLPS_THROW_EX(u"No suffix found!"sv);
        }
        var.remove_suffix(suffix.size());
        // find a variable replacement
        OnReplaceVariable(output, cookie, TrimStrBlank(var));
    }
    return output;
}

std::u32string ReplaceEngine::ProcessFunction(const std::u32string_view source, const std::u32string_view prefix, const std::u32string_view suffix, void* cookie)
{
    Expects(!prefix.empty()); // Illegal suffix
    common::parser::ParserContext context(source);
    ContextReader reader(context);
    std::u32string output;
    while (true)
    {
        auto before = reader.ReadUntil(prefix);
        if (before.empty()) // reaching end
        {
            output.append(reader.ReadAll());
            break;
        }
        {
            before.remove_suffix(prefix.size());
            output.append(before);
        }
        reader.ReadWhile(IgnoreBlank);
        auto funcName = reader.ReadUntil(U"("sv);
        if (funcName.empty())
        {
            if (reader.IsEnd())
                NLPS_THROW_EX(u"End before func name"sv);
            else
                NLPS_THROW_EX(u"No '(' found!"sv);
        }
        funcName.remove_suffix(1);

        std::vector<std::u32string_view> args;
        enum class States : uint32_t { Init, Pending, InComma, InSlash, End };
        States state = States::Init;
        uint32_t nestedLevel = 0;
        const auto PushArg = [&]() 
        {
            Expects(nestedLevel == 0);
            auto part = reader.CommitRead();
            part.remove_suffix(1);
            args.push_back(TrimStrBlank(part));
        };
        reader.ReadWhile(IgnoreBlank);
        while (state != States::End)
        {
            const auto ch = reader.PeekNext();
            if (ch == common::parser::special::CharEnd)
                break;
            reader.MoveNext();
            switch (state)
            {
            case States::Init:
            case States::Pending:
                switch (ch)
                {
                case U'"':
                    state = States::InComma;
                    break;
                case U'(':
                    state = States::Pending;
                    nestedLevel++;
                    break;
                case U')':
                    if (nestedLevel)
                    {
                        state = States::Pending;
                        nestedLevel--;
                    }
                    else
                    {
                        if (state == States::Init)
                        {
                            if (args.size() > 0)
                                NLPS_THROW_EX(u"empty arg not allowed"sv);
                            reader.CommitRead();
                        }
                        else
                            PushArg();
                        state = States::End;
                    }
                    break;
                case U',':
                    if (state == States::Init)
                        NLPS_THROW_EX(u"empty arg not allowed"sv);
                    else if (nestedLevel == 0)
                    {
                        PushArg();
                        state = States::Init;
                        reader.ReadWhile(IgnoreBlank);
                    }
                    break;
                default:
                    state = States::Pending;
                    break;
                }
                break;
            case States::InComma:
                if (ch == U'"')
                    state = States::Pending;
                else if (ch == U'\\')
                    state = States::InSlash;
                break;
            case States::InSlash:
                state = States::InComma;
                break;
            case States::End:
                Expects(false);
                break;
            }
        }
        if (state != States::End)
            NLPS_THROW_EX(u"End before arg list finishes"sv);
        if (!suffix.empty())
        {
            reader.ReadWhile(IgnoreBlank);
            if (!reader.ReadMatch(suffix))
            {
                if (reader.IsEnd())
                    NLPS_THROW_EX(u"End before suffix"sv);
                else
                    NLPS_THROW_EX(u"Unexpeccted char before suffix!"sv);
            }
        }
        // find a function replacement
        OnReplaceFunction(output, cookie, TrimStrBlank(funcName), args);
    }
    return output;
}

ReplaceEngine::~ReplaceEngine()
{
}

}
