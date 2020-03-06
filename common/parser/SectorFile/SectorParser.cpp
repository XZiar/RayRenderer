#include "SectorParser.h"
#include "ArgParser.h"
#include "ParserRely.h"

namespace xziar::sectorlang
{
using tokenizer::SectorLangToken;


SectorRaw SectorsParser::ParseNextSector()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
       
    constexpr auto ExpectSectorOrMeta = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, SectorLangToken::Sector, SectorLangToken::MetaFunc);
    constexpr auto ExpectString       = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, BaseToken::String);

    constexpr auto MainLexer = ParserLexerBase<CommentTokenizer, tokenizer::MetaFuncPrefixTokenizer, tokenizer::BlockPrefixTokenizer>();

    SectorRaw sector;
    std::vector<FuncCall> metaFuncs;
    while (true)
    {
        const auto token = ExpectNextToken(MainLexer, IgnoreBlank, IgnoreCommentToken, ExpectSectorOrMeta);
        const auto tkType = token.GetIDEnum<SectorLangToken>();
        if (tkType == SectorLangToken::Sector)
        {
            sector.Type = token.GetString();
            break;
        }
        Expects(tkType == SectorLangToken::MetaFunc);
        metaFuncs.emplace_back(ComplexArgParser::ParseFuncBody(token.GetString(), MemPool, Context));
    }
    
    EatLeftParenthese();
    constexpr auto NameLexer = ParserLexerBase<CommentTokenizer, StringTokenizer>();
    const auto sectorNameToken = ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectString);
    sector.Name = sectorNameToken.GetString();
    EatRightParenthese();

    EatLeftCurlyBrace();
        
    ContextReader reader(Context);
    auto guardString = std::u32string(reader.ReadLine());
    guardString.append(U"}");
    sector.Position = { Context.Row, Context.Col };
    sector.Content = reader.ReadUntil(guardString);
    sector.Content.remove_suffix(guardString.size());

    sector.MetaFunctions = MemPool.CreateArray(metaFuncs);
    sector.FileName = Context.SourceName;
    return sector;
}

std::vector<SectorRaw> SectorsParser::ParseAllSectors()
{
    std::vector<SectorRaw> sectors;
    while (true)
    {
        ContextReader reader(Context);
        reader.ReadWhile(IgnoreBlank);
        if (reader.PeekNext() == common::parser::special::CharEnd)
            break;
        sectors.emplace_back(ParseNextSector());
    }
    return sectors;
}

void SectorParser::ParseSectorRaw()
{
}

void SectorParser::ParseSector(const SectorRaw& sector, MemoryPool& pool)
{
    common::parser::ParserContext context(sector.Content, sector.FileName);
    SectorParser parser(pool, context, sector);
    return parser.ParseSectorRaw();
}

}
