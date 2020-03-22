#include "BlockParser.h"
#include "ArgParser.h"
#include "ParserRely.h"

namespace xziar::nailang
{
using tokenizer::SectorLangToken;


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
        const auto tkType = token.GetIDEnum<SectorLangToken>();
        if (tkType == SectorLangToken::Raw)
        {
            RawBlockWithMeta block;
            static_cast<RawBlock&>(block) = FillRawBlock(token.GetString());
            block.MetaFunctions = MemPool.CreateArray(metaFuncs);
            return block;
        }
        Expects(tkType == SectorLangToken::MetaFunc);
        metaFuncs.emplace_back(ComplexArgParser::ParseFuncBody(token.GetString(), MemPool, Context));
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
        BinaryStatement statement(*selfOp, assign.Variable, *stmt);
        assign.Statement = MemPool.Create<BinaryStatement>(statement);
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

    std::vector<BlockContent> contents;
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
            contents.push_back(BlockContent::Generate(target, offset, count));
            metaFuncs.clear();
        } continue;
        case SectorLangToken::Block:
        {
            Block inlineBlk;
            FillBlockName(inlineBlk);
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
            contents.push_back(BlockContent::Generate(target, offset, count));
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
            contents.push_back(BlockContent::Generate(target, offset, count));
            metaFuncs.clear();
        } continue;
        case SectorLangToken::Var:
        {
            if constexpr (!AllowNonBlock)
                ThrowNonSupport(token, u"Variable assignment not supportted here"sv);
            Assignment assign = ParseAssignment(token.GetString());
            const auto target = MemPool.Create<Assignment>(assign);
            const auto [offset, count] = AppendMetaFuncs();
            contents.push_back(BlockContent::Generate(target, offset, count));
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
