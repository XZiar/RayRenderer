#include "NailangParser.h"
#include "ParserRely.h"
#include "StringUtil/Format.h"
#include <boost/range/adaptor/reversed.hpp>

namespace xziar::nailang
{
using tokenizer::NailangToken;


std::vector<PartedName::PartType> PartedName::GetParts(std::u32string_view name)
{
    std::vector<PartType> parts;
    for (const auto piece : common::str::SplitStream(name, U'.', true))
    {
        const auto offset = piece.data() - name.data();
        if (offset > UINT16_MAX)
            COMMON_THROW(NailangPartedNameException, u"Name too long"sv, name, piece);
        if (piece.size() > UINT16_MAX)
            COMMON_THROW(NailangPartedNameException, u"Name part too long"sv, name, piece);
        if (piece.size() == 0)
            COMMON_THROW(NailangPartedNameException, u"Empty name part"sv, name, piece);
        parts.emplace_back(static_cast<uint16_t>(offset), static_cast<uint16_t>(piece.size()));
    }
    return parts;
}
PartedName* PartedName::Create(MemoryPool& pool, std::u32string_view name, uint16_t exinfo)
{
    Expects(name.size() > 0);
    const auto parts = GetParts(name);
    if (parts.size() == 1)
    {
        const auto space = pool.Alloc<PartedName>();
        const auto ptr = reinterpret_cast<PartedName*>(space.data());
        new (ptr) PartedName(name, parts, exinfo);
        return ptr;
    }
    else
    {
        const auto partSize = parts.size() * sizeof(PartType);
        const auto space = pool.Alloc(sizeof(PartedName) + partSize, alignof(PartedName));
        const auto ptr = reinterpret_cast<PartedName*>(space.data());
        new (ptr) PartedName(name, parts, exinfo);
        memcpy_s(ptr + 1, partSize, parts.data(), partSize);
        return ptr;
    }
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
        RET_TK_ID(EmbedOp);
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

AssignExpr NailangParser::ParseAssignExpr(const std::u32string_view var, std::pair<uint32_t, uint32_t> pos)
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    using tokenizer::AssignOps;
    using Behavior = NilCheck::Behavior;

    constexpr auto FirstLexer = ParserLexerBase<CommentTokenizer,
        tokenizer::SubFieldTokenizer, tokenizer::SquareBracketTokenizer, tokenizer::AssignOpTokenizer>();

    std::vector<Expr> tmpQueries;
    AssignExpr assign(var);
    assign.Position = pos;

    for (bool shouldCont = true; shouldCont;)
    {
        auto token = GetNextToken(FirstLexer, IgnoreBlank, IgnoreCommentToken);
        switch (token.GetIDEnum<NailangToken>())
        {
        case NailangToken::SquareBracket: // Indexer
        {
            if (token.GetChar() == U']')
                OnUnExpectedToken(token, u"Unexpected right square bracket"sv);
            const auto index = ParseExprChecked("]"sv, U"]"sv);
            if (!index)
                OnUnExpectedToken(token, u"lack of index"sv);
            SubQuery::PushQuery(tmpQueries, index);
        } break;
        case NailangToken::SubField: // SubField
        {
            SubQuery::PushQuery(tmpQueries, token.GetString());
        } break;
        case NailangToken::Assign:
        {
            std::optional<EmbedOps> selfOp;
            Behavior notnull = Behavior::Pass, null = Behavior::Pass;
            const auto aop = static_cast<AssignOps>(token.GetUInt());
            switch (aop)
            {
            case AssignOps::Assign:                                                         break;
            case AssignOps::NewCreate: notnull = Behavior::Throw;                           break;
            case AssignOps::NilAssign:    null = Behavior::Skip;                            break;
            case AssignOps::AndAssign:    null = Behavior::Throw; selfOp = EmbedOps::And;   break;
            case AssignOps::OrAssign:     null = Behavior::Throw; selfOp = EmbedOps::Or;    break;
            case AssignOps::AddAssign:    null = Behavior::Throw; selfOp = EmbedOps::Add;   break;
            case AssignOps::SubAssign:    null = Behavior::Throw; selfOp = EmbedOps::Sub;   break;
            case AssignOps::MulAssign:    null = Behavior::Throw; selfOp = EmbedOps::Mul;   break;
            case AssignOps::DivAssign:    null = Behavior::Throw; selfOp = EmbedOps::Div;   break;
            case AssignOps::RemAssign:    null = Behavior::Throw; selfOp = EmbedOps::Rem;   break;
            default: OnUnExpectedToken(token, u"expect assign op"sv);                       break;
            }
            if (!tmpQueries.empty() && (aop == AssignOps::NewCreate || aop == AssignOps::NilAssign))
                OnUnExpectedToken(token, u"NewCreate and NilAssign cannot be applied with query"sv);
            assign.Check = { notnull, null };
            const auto stmt = ParseExprChecked(";", U";");
            if (!stmt)
                NLPS_THROW_EX(u"expect statement"sv);
            if (!selfOp.has_value())
            {
                assign.Statement = stmt;
            }
            else
            {
                BinaryExpr statement(*selfOp, assign.Target, stmt);
                assign.Statement = MemPool.Create<BinaryExpr>(statement);
            }
            assign.Queries.Queries = MemPool.CreateArray(tmpQueries);
            tmpQueries.clear();
            shouldCont = false;
        } break;
        default:
            OnUnExpectedToken(token, u"expect [assign op] or [indexer] or [subfield]"sv);
            break;
        }
    }

    return assign;
}

struct ExprOp
{
    enum class Type : uint8_t { Empty, Unary, Binary, Ternary };
    Type TypeData = Type::Empty;
    uint8_t Val = 0;
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return TypeData != Type::Empty; }
    [[nodiscard]] constexpr EmbedOps GetEmbedOp() const noexcept { return static_cast<EmbedOps>(Val); }
    constexpr void operator=(EmbedOps op) noexcept 
    {
        TypeData = EmbedOpHelper::IsUnaryOp(op) ? Type::Unary : Type::Binary;
        Val = common::enum_cast(op);
    }
    constexpr void SetTernary(uint8_t stage) noexcept
    {
        TypeData = Type::Ternary;
        Val = stage;
    }
};

struct QueriesHolder
{
    std::vector<Expr> Queries;
    void PushSubField(std::u32string_view subField)
    {
        SubQuery::PushQuery(Queries, subField);
    }
    void PushIndexer(const Expr& indexer)
    {
        SubQuery::PushQuery(Queries, indexer);
    }
    void CommitTo(MemoryPool& pool, Expr& opr)
    {
        if (!Queries.empty())
        {
            Expects(opr);
            Expects(opr.TypeData != Expr::Type::Query);
            const auto sp = pool.CreateArray(Queries);
            opr = pool.Create<QueryExpr>(opr, sp);
            Queries.clear();
        }
    }
    bool CheckNonLiteralLimit(const Expr& opr) const noexcept
    {
        if (Queries.empty()) // first query
        {
            switch (opr.TypeData)
            {
            case Expr::Type::Func:
            case Expr::Type::Unary:
            case Expr::Type::Binary:
            case Expr::Type::Var:
                return true;
            default:
                return false;
            }
        }
        return true;
    }
};

std::pair<Expr, char32_t> NailangParser::ParseExpr(std::string_view stopDelim)
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;

    const DelimTokenizer StopTokenizer = stopDelim;
    const auto ArgLexer = ParserLexerBase<CommentTokenizer,
        DelimTokenizer, tokenizer::ParentheseTokenizer, tokenizer::NormalFuncPrefixTokenizer,
        StringTokenizer, IntTokenizer, FPTokenizer, BoolTokenizer,
        tokenizer::VariableTokenizer, tokenizer::TernaryCondTokenizer, tokenizer::EmbedOpTokenizer,
        tokenizer::SubFieldTokenizer, tokenizer::SquareBracketTokenizer>(StopTokenizer);
    const auto OpLexer = ParserLexerBase<CommentTokenizer, DelimTokenizer, tokenizer::TernaryCondTokenizer,
        tokenizer::EmbedOpTokenizer, tokenizer::SubFieldTokenizer, tokenizer::SquareBracketTokenizer>(StopTokenizer);

    Expr oprend[3];
    QueriesHolder query;
    uint32_t oprIdx = 0;
    ExprOp op;
    char32_t stopChar = common::parser::special::CharEnd;

    bool shouldContinue = true;
    while (shouldContinue)
    {
        auto& targetOpr = oprend[oprIdx];
        const bool isAtOp = targetOpr && !op;
        auto token = isAtOp ?
            GetNextToken(OpLexer, IgnoreBlank, IgnoreCommentToken) :
            GetNextToken(ArgLexer, IgnoreBlank, IgnoreCommentToken);

#define EID(id) common::enum_cast(id)
        switch (token.GetID())
        {
        case EID(BaseToken::Unknown):
        case EID(BaseToken::Error):
        {
            OnUnExpectedToken(token, isAtOp ? u"expect an operator"sv : u"unknown or error token"sv);
        } break;
        case EID(BaseToken::Delim):
        case EID(BaseToken::End):
        {
            stopChar = token.GetChar();
            shouldContinue = false;
        } break;
        case EID(NailangToken::Parenthese):
        {
            if (targetOpr)
                OnUnExpectedToken(token, u"already has oprend"sv);
            if (token.GetChar() == U')')
                OnUnExpectedToken(token, u"Unexpected right parenthese"sv);
            targetOpr = ParseExprChecked(")"sv, U")"sv);
        } break;
        case EID(NailangToken::CondOp):
        {
            if (token.GetChar() == U'?')
            {
                if (op) // requires explicit "()" when combined with ther ops
                    OnUnExpectedToken(token, u"already has op"sv);
                if (!targetOpr)
                    OnUnExpectedToken(token, u"expect operand before ?(conditional) operator"sv);
                op.SetTernary(1);
            }
            else if (token.GetChar() == U':')
            {
                if (!op || op.Val != 1)
                    OnUnExpectedToken(token, op ? u"already has ':'"sv : u"expect '?' before ':'"sv);
                if (!targetOpr)
                    OnUnExpectedToken(token, u"expect left operand before :(conditional) operator"sv);
                op.SetTernary(2);
            }
            query.CommitTo(MemPool, targetOpr);
            oprIdx++;
        } break;
        case EID(NailangToken::EmbedOp):
        {
            if (op)
                OnUnExpectedToken(token, u"already has op"sv);
            const auto opval = static_cast<EmbedOps>(token.GetUInt());
            switch (opval)
            {
            case EmbedOps::Not:
                if (oprIdx != 0)
                    OnUnExpectedToken(token, u"expect no operand before ! operator"sv);
                break;
            default:
                if (oprIdx != 0)
                    OnUnExpectedToken(token, u"no support for nested operator"sv);
                if (!targetOpr)
                    OnUnExpectedToken(token, u"expect 1 operand before operator"sv);
                break;
            }
            op = opval;
            query.CommitTo(MemPool, targetOpr);
            oprIdx++;
        } break;
        case EID(NailangToken::SquareBracket): // Indexer
        {
            if (token.GetChar() == U']')
                OnUnExpectedToken(token, u"Unexpected right square bracket"sv);
            if (!targetOpr)
                OnUnExpectedToken(token, u"Indexer should follow a Expr"sv);
            if (!query.CheckNonLiteralLimit(targetOpr))
                OnUnExpectedToken(token, u"SubQuery should not follow a litteral type"sv);
            auto index = ParseExprChecked("]"sv, U"]"sv);
            if (!index)
                OnUnExpectedToken(token, u"lack of index"sv);
            query.PushIndexer(index);
        } break;
        case EID(NailangToken::SubField): // SubField
        {
            if (!targetOpr)
                OnUnExpectedToken(token, u"SubField should follow a Expr"sv);
            if (!query.CheckNonLiteralLimit(targetOpr))
                OnUnExpectedToken(token, u"SubQuery should not follow a litteral type"sv);
            query.PushSubField(token.GetString());
        } break;
        default:
        {
            if (targetOpr)
                OnUnExpectedToken(token, u"already has oprend"sv);
            switch (token.GetID())
            {
                case EID(BaseToken::Uint)    : targetOpr = token.GetUInt();      break;
                case EID(BaseToken::Int)     : targetOpr = token.GetInt();       break;
                case EID(BaseToken::FP)      : targetOpr = token.GetDouble();    break;
                case EID(BaseToken::Bool)    : targetOpr = token.GetBool();      break;
                case EID(NailangToken::Var)  :
                    targetOpr = LateBindVar(token.GetString());
                break;
                case EID(BaseToken::String)  :
                    targetOpr = ProcessString(token.GetString(), MemPool);
                break;
                case EID(NailangToken::Func) :
                    targetOpr = MemPool.Create<FuncCall>(ParseFuncCall(token, FuncName::FuncInfo::ExprPart));
                break;
            default: OnUnExpectedToken(token, u"Unexpected token"sv);
            }
#undef EID
        } break;
        }
    }
    // exit from delim or end
    query.CommitTo(MemPool, oprend[oprIdx]);
    switch (op.TypeData)
    {
    case ExprOp::Type::Empty:
        Ensures(oprIdx == 0);
        return { oprend[0], stopChar };
    case ExprOp::Type::Ternary:
        if (op.Val != 2)
            NLPS_THROW_EX(u"Incomplete ternary operator"sv);
        if (!oprend[2])
            NLPS_THROW_EX(u"Lack right oprend for ternary operator"sv);
        return { MemPool.Create<TernaryExpr>(oprend[0], oprend[1], oprend[2]), stopChar };
    case ExprOp::Type::Binary:
        Ensures(oprIdx == 1);
        if (!oprend[1])
            NLPS_THROW_EX(u"Lack 2nd oprend for binary operator"sv);
        return { MemPool.Create<BinaryExpr>(op.GetEmbedOp(), oprend[0], oprend[1]), stopChar };
    case ExprOp::Type::Unary:
        Ensures(oprIdx == 1);
        if (!oprend[1])
            NLPS_THROW_EX(u"Lack oprend for unary operator"sv);
        return { MemPool.Create<UnaryExpr>(op.GetEmbedOp(), oprend[1]), stopChar };
    default:
        Ensures(false);
        return { {}, stopChar };
    }
}

Expr NailangParser::ParseExprChecked(std::string_view stopDelims, std::u32string_view stopChecker)
{
    auto [arg, delim] = ParseExpr(stopDelims);
    if (!stopChecker.empty() && stopChecker.find(delim) == std::u32string_view::npos)
        NLPS_THROW_EX(FMTSTR(u"Ends with unexpected delim [{}], expects [{}]", delim, stopChecker));
    return arg;
}

void NailangParser::ParseContentIntoBlock(const bool allowNonBlock, Block& block, const bool tillTheEnd)
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    using tokenizer::AssignOps;

    constexpr auto MainLexer = ParserLexerBase<CommentTokenizer, 
        tokenizer::MetaFuncPrefixTokenizer, tokenizer::BlockPrefixTokenizer, tokenizer::NormalFuncPrefixTokenizer, 
        tokenizer::VariableTokenizer, tokenizer::CurlyBraceTokenizer>();

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
        const auto token = GetNextToken(MainLexer, IgnoreBlank, IgnoreCommentToken);
        switch (token.GetIDEnum<NailangToken>())
        {
        case NailangToken::MetaFunc:
        {
            metaFuncs.emplace_back(ParseFuncCall(token, FuncName::FuncInfo::Meta));
        } continue;
        case NailangToken::Raw:
        {
            const auto target = MemPool.Create<RawBlock>(ParseRawBlock(token));
            contents.emplace_back(target, AppendMetaFuncs());
            metaFuncs.clear();
        } continue;
        case NailangToken::Block:
        {
            Block inlineBlk;
            inlineBlk.Position = GetPosition(token);
            FillBlockName(inlineBlk);
            inlineBlk.Type = token.GetString();
            EatLeftCurlyBrace();
            {
                ContextReader reader(Context);
                reader.ReadLine();
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
            metaFuncs.clear();
        } continue;
        case NailangToken::Func:
        {
            if (!allowNonBlock)
                OnUnExpectedToken(token, u"Function call not supported here"sv);
            FuncCall funccall = ParseFuncCall(token);
            EatSemiColon();
            const auto target = MemPool.Create<FuncCall>(funccall);
            contents.emplace_back(target, AppendMetaFuncs());
            metaFuncs.clear();
        } continue;
        case NailangToken::Var:
        {
            if (!allowNonBlock)
                OnUnExpectedToken(token, u"Variable assignment not supported here"sv);
            AssignExpr assign = ParseAssignExpr(token);
            const auto target = MemPool.Create<AssignExpr>(assign);
            contents.emplace_back(target, AppendMetaFuncs());
            metaFuncs.clear();
        } continue;
        case NailangToken::CurlyBrace:
        {
            if (tillTheEnd || token.GetChar() != U'}')
                OnUnExpectedToken(token, u"when parsing block contents"sv);
            if (metaFuncs.size() > 0)
                OnUnExpectedToken(token, u"expect block/assignment/funccall after metafuncs"sv);
        } break;
        default:
        {
            if (token.GetIDEnum() != BaseToken::End)
                OnUnExpectedToken(token, u"when parsing block contents"sv);
            if (metaFuncs.size() > 0)
                OnUnExpectedToken(token, u"expect block/assignment/funccall after metafuncs"sv);
            if (!tillTheEnd)
                OnUnExpectedToken(token, u"expect '}' to close the scope"sv);
        } break;
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


std::u32string_view ReplaceEngine::TrimStrBlank(const std::u32string_view str) noexcept
{
    size_t len = str.size();
    for (const auto ch : boost::adaptors::reverse(str))
    {
        if (!IgnoreBlank(ch))
            break;
        len--;
    }
    return str.substr(0, len);
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
