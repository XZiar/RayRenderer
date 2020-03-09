#include "BlockParser.h"
#include "ArgParser.h"
#include "ParserRely.h"

namespace xziar::sectorlang
{
using tokenizer::SectorLangToken;


RawBlock BlockParser::FillBlock(const std::u32string_view name, const std::vector<FuncCall>& metaFuncs)
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    constexpr auto ExpectString = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, BaseToken::String);

    RawBlock block;
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
    return Block();
}

RawBlock BlockParser::GetNextBlock()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
       
    constexpr auto ExpectSectorOrMeta = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, SectorLangToken::Block, SectorLangToken::MetaFunc);

    constexpr auto MainLexer = ParserLexerBase<CommentTokenizer, tokenizer::MetaFuncPrefixTokenizer, tokenizer::BlockPrefixTokenizer>();

    RawBlock sector;
    std::vector<FuncCall> metaFuncs;
    while (true)
    {
        const auto token = ExpectNextToken(MainLexer, IgnoreBlank, IgnoreCommentToken, ExpectSectorOrMeta);
        const auto tkType = token.GetIDEnum<SectorLangToken>();
        if (tkType == SectorLangToken::Block)
        {
            return FillBlock(token.GetString(), metaFuncs);
        }
        Expects(tkType == SectorLangToken::MetaFunc);
        metaFuncs.emplace_back(ComplexArgParser::ParseFuncBody(token.GetString(), MemPool, Context));
    }
}

std::vector<RawBlock> BlockParser::GetAllBlocks()
{
    std::vector<RawBlock> sectors;
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
