#include "BlockParser.h"
#include "ArgParser.h"
#include "ParserRely.h"

namespace xziar::sectorlang
{
using tokenizer::SectorLangToken;


RawBlockWithMeta BlockParser::FillBlock(const std::u32string_view name, const std::vector<FuncCall>& metaFuncs)
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    constexpr auto ExpectString = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, BaseToken::String);

    RawBlockWithMeta block;
    block.Type = name;

    EatLeftParenthese();
    constexpr auto NameLexer = ParserLexerBase<CommentTokenizer, StringTokenizer>();
    const auto sectorNameToken = ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectString);
    block.Name = sectorNameToken.GetString();
    EatRightParenthese();

    EatLeftCurlyBrace();

    ContextReader reader(Context);
    auto guardString = std::u32string(reader.ReadLine());
    guardString.append(U"}");
    block.Position = { Context.Row, Context.Col };
    block.Source = reader.ReadUntil(guardString);
    block.Source.remove_suffix(guardString.size());

    block.MetaFunctions = MemPool.CreateArray(metaFuncs);
    block.FileName = Context.SourceName;
    return block;
}

Block BlockParser::ParseBlockContent()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;

    constexpr auto MainLexer = ParserLexerBase<CommentTokenizer, tokenizer::MetaFuncPrefixTokenizer, tokenizer::BlockPrefixTokenizer, tokenizer::NormalFuncPrefixTokenizer, tokenizer::VariableTokenizer>();

    Block block;
    std::vector<BlockContent> contents;
    std::vector<FuncCall> metaFuncs;
    while (true)
    {
        const auto token = GetNextToken(MainLexer, IgnoreBlank, IgnoreCommentToken);
        switch (token.GetIDEnum<SectorLangToken>())
        {
        case SectorLangToken::MetaFunc:
        {
            metaFuncs.emplace_back(ComplexArgParser::ParseFuncBody(token.GetString(), MemPool, Context));
        } continue;
        case SectorLangToken::Block:
        {
            const auto target = MemPool.Create<RawBlockWithMeta>(FillBlock(token.GetString(), metaFuncs));
            contents.push_back(BlockContent::Generate(target));
            metaFuncs.clear();
        } continue;
        case SectorLangToken::Func:
        {
            FuncCallWithMeta funccall;
            static_cast<FuncCall&>(funccall) = ComplexArgParser::ParseFuncBody(token.GetString(), MemPool, Context);
            funccall.MetaFunctions = MemPool.CreateArray(metaFuncs);
            const auto target = MemPool.Create<FuncCallWithMeta>(funccall);
            contents.push_back(BlockContent::Generate(target));
            metaFuncs.clear();
        } continue;
        case SectorLangToken::Var:
        {
            // not implemented
            metaFuncs.clear();
        } continue;
        default:
            if (token.GetIDEnum() == BaseToken::End)
            {
                if (metaFuncs.size() > 0)
                    OnUnExpectedToken(token, u"expect block/assignment/funccall after metafuncs"sv);
                break;
            }
            else
                OnUnExpectedToken(token, u"when parsing block contents"sv);
        }
    }
}

RawBlockWithMeta BlockParser::GetNextBlock()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
       
    constexpr auto ExpectBlockOrMeta = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, SectorLangToken::Block, SectorLangToken::MetaFunc);

    constexpr auto MainLexer = ParserLexerBase<CommentTokenizer, tokenizer::MetaFuncPrefixTokenizer, tokenizer::BlockPrefixTokenizer>();

    std::vector<FuncCall> metaFuncs;
    while (true)
    {
        const auto token = ExpectNextToken(MainLexer, IgnoreBlank, IgnoreCommentToken, ExpectBlockOrMeta);
        const auto tkType = token.GetIDEnum<SectorLangToken>();
        if (tkType == SectorLangToken::Block)
        {
            return FillBlock(token.GetString(), metaFuncs);
        }
        Expects(tkType == SectorLangToken::MetaFunc);
        metaFuncs.emplace_back(ComplexArgParser::ParseFuncBody(token.GetString(), MemPool, Context));
    }
}

std::vector<RawBlockWithMeta> BlockParser::GetAllBlocks()
{
    std::vector<RawBlockWithMeta> sectors;
    while (true)
    {
        ContextReader reader(Context);
        reader.ReadWhile(IgnoreBlank);
        if (reader.PeekNext() == common::parser::special::CharEnd)
            break;
        sectors.emplace_back(GetNextBlock());
    }
    return sectors;
}


Block BlockParser::ParseBlockRaw(const RawBlock& block, MemoryPool& pool)
{
    common::parser::ParserContext context(block.Source, block.FileName);
    std::tie(context.Row, context.Col) = block.Position;
    BlockParser parser(pool, context);

    return parser.ParseBlockContent();
}

}
