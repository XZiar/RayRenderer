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

AssignmentWithMeta BlockParser::ParseAssignment(const std::u32string_view var)
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    using tokenizer::AssignOps;

    constexpr auto AssignOpLexer = ParserLexerBase<CommentTokenizer, tokenizer::AssignOpTokenizer>();
    constexpr auto ExpectAssignOp = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, SectorLangToken::Assign);

    AssignmentWithMeta assign;
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
    const auto stmt_ = MemPool.Create<FuncArgRaw>(*stmt);

    if (!selfOp.has_value())
    {
        assign.Statement = stmt_;
    }
    else
    {
        BinaryStatement statement(*selfOp, assign.Variable, stmt_);
        assign.Statement = MemPool.Create<BinaryStatement>(statement);
    }
    return assign;
}

Block BlockParser::ParseBlockContent()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    using tokenizer::AssignOps;

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
            AssignmentWithMeta assign = ParseAssignment(token.GetString());
            assign.MetaFunctions = MemPool.CreateArray(metaFuncs);
            const auto target = MemPool.Create<AssignmentWithMeta>(assign);
            contents.push_back(BlockContent::Generate(target));
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
