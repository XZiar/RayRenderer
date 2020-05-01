#include "NailangParser.h"
#include "ParserRely.h"

namespace xziar::nailang
{
using tokenizer::SectorLangToken;

struct ExpectParentheseL
{
    static constexpr ParserToken Token = ParserToken(SectorLangToken::Parenthese, U'(');
};
struct ExpectParentheseR
{
    static constexpr ParserToken Token = ParserToken(SectorLangToken::Parenthese, U')');
};
struct ExpectCurlyBraceL
{
    static constexpr ParserToken Token = ParserToken(SectorLangToken::CurlyBrace, U'{');
};
struct ExpectCurlyBraceR
{
    static constexpr ParserToken Token = ParserToken(SectorLangToken::CurlyBrace, U'}');
};


std::u16string NailangParser::DescribeTokenID(const uint16_t tid) const noexcept
{
#define RET_TK_ID(type) case SectorLangToken::type:        return u ## #type
    switch (static_cast<SectorLangToken>(tid))
    {
        RET_TK_ID(Raw);
        RET_TK_ID(Block);
        RET_TK_ID(MetaFunc);
        RET_TK_ID(Func);
        RET_TK_ID(Var);
        RET_TK_ID(EmbedOp);
        RET_TK_ID(Parenthese);
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


struct FuncEndDelimer
{
    static constexpr DelimTokenizer Stopper() noexcept { return ",)"sv; }
};
struct GroupEndDelimer
{
    static constexpr DelimTokenizer Stopper() noexcept { return ")"sv; }
};
struct StatementEndDelimer
{
    static constexpr DelimTokenizer Stopper() noexcept { return ";"sv; }
};

template<typename StopDelimer>
std::pair<std::optional<RawArg>, char32_t> ComplexArgParser::ParseArg()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    
    constexpr DelimTokenizer StopDelim = StopDelimer::Stopper();
    constexpr auto ArgLexer = ParserLexerBase<CommentTokenizer, DelimTokenizer, 
        tokenizer::ParentheseTokenizer, tokenizer::NormalFuncPrefixTokenizer,
        StringTokenizer, IntTokenizer, FPTokenizer, BoolTokenizer, 
        tokenizer::VariableTokenizer, tokenizer::EmbedOpTokenizer>(StopDelim);
    constexpr auto OpLexer = ParserLexerBase<CommentTokenizer, DelimTokenizer,
        tokenizer::EmbedOpTokenizer>(StopDelim);
    
    std::optional<RawArg> oprend1, oprend2;
    std::optional<EmbedOps> op;
    char32_t stopChar = common::parser::special::CharEnd;

    while (true) 
    {
        const bool isAtOp = oprend1.has_value() && !op.has_value() && !oprend2.has_value();
        ParserToken token = isAtOp ?
            GetNextToken(OpLexer,  IgnoreBlank, IgnoreCommentToken) :
            GetNextToken(ArgLexer, IgnoreBlank, IgnoreCommentToken);

        {
            const auto type = token.GetIDEnum();
            if (type == BaseToken::Unknown || type == BaseToken::Error)
            {
                if (isAtOp)
                    throw U"expect an operator"sv;
                else
                    throw U"unknown or error token"sv;
            }
            if (type == BaseToken::Delim || type == BaseToken::End)
            {
                stopChar = token.GetChar();
                break;
            }
        }

        if (token.GetIDEnum<SectorLangToken>() == SectorLangToken::EmbedOp)
        {
            if (op.has_value())
                throw U"Already has op"sv;
            const auto opval = static_cast<EmbedOps>(token.GetUInt());
            const bool isUnary = EmbedOpHelper::IsUnaryOp(opval);
            if (isUnary && oprend1.has_value())
                throw U"Expect no operand before unary operator"sv;
            if (!isUnary && !oprend1.has_value())
                throw U"Expect operand before binary operator"sv;
            op = opval;
        }
        else
        {
            auto& target = op.has_value() ? oprend2 : oprend1;
            if (target.has_value())
                throw U"Already has oprend"sv;
#define EID(id) case common::enum_cast(id)
            switch (token.GetID())
            {
            EID(BaseToken::Uint)        : target = token.GetUInt();                     break;
            EID(BaseToken::Int)         : target = token.GetInt();                      break;
            EID(BaseToken::FP)          : target = token.GetDouble();                   break;
            EID(BaseToken::Bool)        : target = token.GetBool();                     break;
            EID(SectorLangToken::Var)   : target = LateBindVar{ token.GetString() };    break;
            EID(BaseToken::String)      : 
                target = ProcessString(token.GetString(), MemPool); 
                break;
            EID(SectorLangToken::Func)  :
                target = MemPool.Create<FuncCall>(
                    ParseFuncBody(token.GetString(), MemPool, Context));                
                break;
            EID(SectorLangToken::Parenthese):
                if (token.GetChar() == U'(')
                    target = ParseArg<GroupEndDelimer>().first;
                else
                    throw U"Unexpected right parenthese"sv;
                break; 
            default                     : throw U"Unexpected token"sv;
            }
#undef EID
        }
    }
    // exit from delim or end

    if (!op.has_value())
    {
        Expects(!oprend2.has_value());
        return { std::move(oprend1), stopChar };
    }
    else if (EmbedOpHelper::IsUnaryOp(*op))
    {
        Expects(!oprend1.has_value());
        if (!oprend2.has_value())
            throw U"Lack oprend for unary operator"sv;
        return { MemPool.Create<UnaryExpr>(*op, std::move(*oprend2)), stopChar };

    }
    else
    {
        Expects(oprend1.has_value());
        if (!oprend2.has_value())
            throw U"Lack 2nd oprend for binary operator"sv;
        return { MemPool.Create<BinaryExpr>(*op, std::move(*oprend1), std::move(*oprend2)), stopChar };
    }
}

RawArg ComplexArgParser::ProcessString(const std::u32string_view str, MemoryPool& pool)
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

FuncCall ComplexArgParser::ParseFuncBody(std::u32string_view funcName, MemoryPool& pool, common::parser::ParserContext& context)
{
    ComplexArgParser parser(pool, context);
    parser.EatLeftParenthese();
    std::vector<RawArg> args;
    while (true)
    {
        auto [arg, delim] = parser.ParseArg<FuncEndDelimer>();
        if (arg.has_value())
            args.emplace_back(std::move(*arg));
        else
            if (delim != U')')
                throw U"Does not allow empty argument"sv;
        if (delim == U')')
            break;
        if (delim == common::parser::special::CharEnd)
            throw U"Expect ')' before reaching end"sv;
    }
    const auto sp = pool.CreateArray(args);
    return { funcName,sp };
}

std::optional<RawArg> ComplexArgParser::ParseSingleStatement(MemoryPool& pool, common::parser::ParserContext& context)
{
    ComplexArgParser parser(pool, context);
    auto [arg, delim] = parser.ParseArg<StatementEndDelimer>();
    if (delim != U';')
        throw U"Expected end with ;"sv;
    return arg;
}


struct ExpectSemoColon
{
    static constexpr ParserToken Token = ParserToken(BaseToken::Delim, U';');
};
void RawBlockParser::EatSemiColon()
{
    using SemiColonTokenizer = tokenizer::KeyCharTokenizer<BaseToken::Delim, U';'>;
    EatSingleToken<ExpectSemoColon, SemiColonTokenizer>();
}

void RawBlockParser::FillBlockName(RawBlock& block)
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

void RawBlockParser::FillBlockInfo(RawBlock& block)
{
    block.Position = { Context.Row, Context.Col };
    block.FileName = Context.SourceName;
}

RawBlock RawBlockParser::FillRawBlock(const std::u32string_view name)
{
    RawBlock block;
    block.Type = name;

    FillBlockName(block);

    EatLeftCurlyBrace();

    ContextReader reader(Context);
    auto guardString = std::u32string(reader.ReadLine());
    FillBlockInfo(block);

    guardString.append(U"}");
    block.Source = reader.ReadUntil(guardString);
    block.Source.remove_suffix(guardString.size());
    return block;
}

RawBlockWithMeta RawBlockParser::GetNextRawBlock()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
       
    constexpr auto ExpectRawOrMeta = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, SectorLangToken::Raw, SectorLangToken::MetaFunc);

    constexpr auto MainLexer = ParserLexerBase<CommentTokenizer, tokenizer::MetaFuncPrefixTokenizer, tokenizer::BlockPrefixTokenizer>();

    std::vector<FuncCall> metaFuncs;
    while (true)
    {
        const auto token = ExpectNextToken(MainLexer, IgnoreBlank, IgnoreCommentToken, ExpectRawOrMeta);
        switch (token.GetIDEnum<SectorLangToken>())
        {
        case SectorLangToken::Raw:
        {
            RawBlockWithMeta block;
            static_cast<RawBlock&>(block) = FillRawBlock(token.GetString());
            block.MetaFunctions = MemPool.CreateArray(metaFuncs);
            return block;
        } break;
        case SectorLangToken::MetaFunc:
            metaFuncs.emplace_back(ComplexArgParser::ParseFuncBody(token.GetString(), MemPool, Context));
            break;
        default:
            Expects(false);
            break;
        }
    }
}

std::vector<RawBlockWithMeta> RawBlockParser::GetAllRawBlocks()
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


void BlockParser::ThrowNonSupport(ParserToken token, std::u16string_view detail)
{
    throw common::parser::detail::ParsingError(GetCurrentFileName(), GetCurrentPosition(), token, detail);
}

Assignment BlockParser::ParseAssignment(const std::u32string_view var)
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    using tokenizer::AssignOps;

    constexpr auto AssignOpLexer = ParserLexerBase<CommentTokenizer, tokenizer::AssignOpTokenizer>();
    constexpr auto ExpectAssignOp = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, SectorLangToken::Assign);

    Assignment assign;
    assign.Variable.Name = var;

    const auto opToken = ExpectNextToken(AssignOpLexer, IgnoreBlank, IgnoreCommentToken, ExpectAssignOp);
    Expects(opToken.GetIDEnum<SectorLangToken>() == SectorLangToken::Assign);

    std::optional<EmbedOps> selfOp;
    switch (static_cast<AssignOps>(opToken.GetUInt()))
    {
    case AssignOps::   Assign:                         break;
    case AssignOps::AndAssign: selfOp = EmbedOps::And; break;
    case AssignOps:: OrAssign: selfOp = EmbedOps:: Or; break;
    case AssignOps::AddAssign: selfOp = EmbedOps::Add; break;
    case AssignOps::SubAssign: selfOp = EmbedOps::Sub; break;
    case AssignOps::MulAssign: selfOp = EmbedOps::Mul; break;
    case AssignOps::DivAssign: selfOp = EmbedOps::Div; break;
    case AssignOps::RemAssign: selfOp = EmbedOps::Rem; break;
    default: OnUnExpectedToken(opToken, u"expect assign op"sv); break;
    }

    const auto stmt = ComplexArgParser::ParseSingleStatement(MemPool, Context);
    if (!stmt.has_value())
        throw u"expect statement"sv;
    //const auto stmt_ = MemPool.Create<FuncArgRaw>(*stmt);

    if (!selfOp.has_value())
    {
        assign.Statement = *stmt;
    }
    else
    {
        BinaryExpr statement(*selfOp, assign.Variable, *stmt);
        assign.Statement = MemPool.Create<BinaryExpr>(statement);
    }
    return assign;
}

template<bool AllowNonBlock>
void BlockParser::ParseContentIntoBlock(Block& block, const bool tillTheEnd)
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    using tokenizer::AssignOps;

    constexpr auto MainLexer = ParserLexerBase<CommentTokenizer, 
        tokenizer::MetaFuncPrefixTokenizer, tokenizer::BlockPrefixTokenizer, tokenizer::NormalFuncPrefixTokenizer, 
        tokenizer::VariableTokenizer, tokenizer::CurlyBraceTokenizer>();

    std::vector<BlockContentItem> contents;
    std::vector<FuncCall> allMetaFuncs;
    std::vector<FuncCall> metaFuncs;
    const auto AppendMetaFuncs = [&]() 
    {
        const auto offset = gsl::narrow_cast<uint32_t>(allMetaFuncs.size());
        const auto count  = gsl::narrow_cast<uint32_t>(   metaFuncs.size());
        allMetaFuncs.reserve(static_cast<size_t>(offset) + count);
        std::move(metaFuncs.begin(), metaFuncs.end(), std::back_inserter(allMetaFuncs));
        metaFuncs.clear();
        return std::pair{ offset, count };
    };
    while (true)
    {
        const auto token = GetNextToken(MainLexer, IgnoreBlank, IgnoreCommentToken);
        switch (token.GetIDEnum<SectorLangToken>())
        {
        case SectorLangToken::MetaFunc:
        {
            metaFuncs.emplace_back(ComplexArgParser::ParseFuncBody(token.GetString(), MemPool, Context));
        } continue;
        case SectorLangToken::Raw:
        {
            const auto target = MemPool.Create<RawBlock>(FillRawBlock(token.GetString()));
            const auto [offset, count] = AppendMetaFuncs();
            contents.push_back(BlockContentItem::Generate(target, offset, count));
            metaFuncs.clear();
        } continue;
        case SectorLangToken::Block:
        {
            Block inlineBlk;
            FillBlockName(inlineBlk);
            inlineBlk.Type = token.GetString();
            EatLeftCurlyBrace();
            {
                ContextReader reader(Context);
                reader.ReadLine();
            }
            FillBlockInfo(inlineBlk);
            {
                const auto idxBegin = Context.Index;
                ParseContentIntoBlock<true>(inlineBlk, false);
                const auto idxEnd = Context.Index;
                inlineBlk.Source = Context.Source.substr(idxBegin, idxEnd - idxBegin - 1);
            }
            const auto target = MemPool.Create<Block>(inlineBlk);
            const auto [offset, count] = AppendMetaFuncs();
            contents.push_back(BlockContentItem::Generate(target, offset, count));
            metaFuncs.clear();
        } continue;
        case SectorLangToken::Func:
        {
            if constexpr (!AllowNonBlock)
                ThrowNonSupport(token, u"Function call not supportted here"sv);
            FuncCall funccall;
            static_cast<FuncCall&>(funccall) = ComplexArgParser::ParseFuncBody(token.GetString(), MemPool, Context);
            EatSemiColon();
            const auto target = MemPool.Create<FuncCall>(funccall);
            const auto [offset, count] = AppendMetaFuncs();
            contents.push_back(BlockContentItem::Generate(target, offset, count));
            metaFuncs.clear();
        } continue;
        case SectorLangToken::Var:
        {
            if constexpr (!AllowNonBlock)
                ThrowNonSupport(token, u"Variable assignment not supportted here"sv);
            Assignment assign = ParseAssignment(token.GetString());
            const auto target = MemPool.Create<Assignment>(assign);
            const auto [offset, count] = AppendMetaFuncs();
            contents.push_back(BlockContentItem::Generate(target, offset, count));
            metaFuncs.clear();
        } continue;
        case SectorLangToken::CurlyBrace:
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
template NAILANGAPI void BlockParser::ParseContentIntoBlock<true >(Block& block, const bool tillTheEnd);
template NAILANGAPI void BlockParser::ParseContentIntoBlock<false>(Block& block, const bool tillTheEnd);

Block BlockParser::ParseRawBlock(const RawBlock& block, MemoryPool& pool)
{
    common::parser::ParserContext context(block.Source, block.FileName);
    std::tie(context.Row, context.Col) = block.Position;
    BlockParser parser(pool, context);

    Block ret;
    static_cast<RawBlock&>(ret) = block;
    parser.ParseContentIntoBlock<true>(ret);
    return ret;
}

Block BlockParser::ParseAllAsBlock(MemoryPool& pool, common::parser::ParserContext& context)
{
    BlockParser parser(pool, context);

    Block ret;
    parser.FillBlockInfo(ret);
    parser.ParseContentIntoBlock<false>(ret);
    return ret;
}

}
