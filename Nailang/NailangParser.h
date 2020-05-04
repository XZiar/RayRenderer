#pragma once
#include "NailangStruct.h"
#include "common/parser/ParserBase.hpp"
#include <optional>

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace xziar::nailang
{


class NAILANGAPI NailangParser : public common::parser::ParserBase
{
private:
protected:
    MemoryPool& MemPool;
    std::u16string SubScopeName;

    [[nodiscard]] std::u16string DescribeTokenID(const uint16_t tid) const noexcept override;
    [[nodiscard]] common::str::StrVariant<char16_t> GetCurrentFileName() const noexcept override;

    template<typename ExpectHolder, typename... TKs>
    void EatSingleToken()
    {
        //constexpr common::parser::ParserToken ExpectToken = ExpectHolder::Token;
        constexpr auto ExpectMatcher = common::parser::detail::TokenMatcherHelper::GetMatcher(std::array{ ExpectHolder::Token });
        constexpr auto IgnoreChar = common::parser::ASCIIChecker("\r\n\v\t ");
        constexpr auto Lexer = common::parser::ParserLexerBase<common::parser::tokenizer::CommentTokenizer, TKs...>();
        ExpectNextToken(Lexer, IgnoreChar, IgnoreCommentToken, ExpectMatcher);
    }

    void EatLeftParenthese();
    void EatRightParenthese();
    void EatLeftCurlyBrace();
    void EatRightCurlyBrace();
public:
    NailangParser(MemoryPool& pool, common::parser::ParserContext& context, std::u16string subScope = u"") :
        ParserBase(context), MemPool(pool), SubScopeName(std::move(subScope)) { }
    virtual ~NailangParser() { }
};


class NAILANGAPI ComplexArgParser : public NailangParser
{
private:
    using NailangParser::NailangParser;

    template<typename StopDelimer>
    [[nodiscard]] std::pair<std::optional<RawArg>, char32_t> ParseArg();
public:
    [[nodiscard]] static RawArg ProcessString(std::u32string_view str, MemoryPool& pool);
    [[nodiscard]] static FuncCall ParseFuncBody(std::u32string_view funcName, MemoryPool& pool, common::parser::ParserContext& context);
    [[nodiscard]] static std::optional<RawArg> ParseSingleStatement(MemoryPool& pool, common::parser::ParserContext& context);
};


class NAILANGAPI RawBlockParser : public NailangParser
{
protected:
    void EatSemiColon();
    void FillBlockName(RawBlock& block);
    void FillBlockInfo(RawBlock& block);
    [[nodiscard]] RawBlock FillRawBlock(const std::u32string_view name);
public:
    using NailangParser::NailangParser;
    
    [[nodiscard]] RawBlockWithMeta GetNextRawBlock();
    [[nodiscard]] std::vector<RawBlockWithMeta> GetAllRawBlocks();
};


class NAILANGAPI BlockParser : public RawBlockParser
{
protected:
    void ThrowNonSupport(common::parser::ParserToken token, std::u16string_view detail);
    [[nodiscard]] Assignment ParseAssignment(const std::u32string_view var);
    template<bool AllowNonBlock>
    void ParseContentIntoBlock(Block& block, const bool tillTheEnd = true);
    using RawBlockParser::RawBlockParser;
public:
    [[nodiscard]] static Block ParseRawBlock(const RawBlock& block, MemoryPool& pool);
    [[nodiscard]] static Block ParseAllAsBlock(MemoryPool& pool, common::parser::ParserContext& context);
};


class NAILANGAPI ReplaceEngine
{
private:
protected:
    common::parser::ParserContext Context;
    std::u32string Output;

    ReplaceEngine(std::u32string_view source) : Context(source) { }
    static std::u32string_view TrimStrBlank(const std::u32string_view str) noexcept;
    virtual void OnReplaceVariable(const std::u32string_view var) = 0;
    virtual void OnReplaceFunction(const std::u32string_view func, const common::span<std::u32string_view> args) = 0;
    virtual void ProcessVariable(const std::u32string_view prefix, const std::u32string_view suffix);
    virtual void ProcessFunction(const std::u32string_view prefix, const std::u32string_view suffix);
public:
    virtual ~ReplaceEngine();
    std::u32string_view GetOutput() const noexcept { return Output; }
    std::u32string ExtractOutput() noexcept { return std::move(Output); }
};


}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif
