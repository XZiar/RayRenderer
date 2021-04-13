#pragma once
#include "NailangStruct.h"
#include "common/Exceptions.hpp"
#include "common/parser/ParserBase.hpp"
#include <optional>
#include <any>

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace xziar::nailang
{

class NAILANGAPI NailangParseException : public common::BaseException
{
    friend class NailangParser;
    PREPARE_EXCEPTION(NailangParseException, BaseException,
        std::u16string File;
        std::pair<size_t, size_t> Position;
        ExceptionInfo(const std::u16string_view msg, const std::u16string_view file, std::pair<size_t, size_t> pos = { 0,0 }) noexcept
            : ExceptionInfo(TYPENAME, msg, file, pos)
        { }
    protected:
        ExceptionInfo(const char* type, const std::u16string_view msg, const std::u16string_view file, 
            std::pair<size_t, size_t> pos = { 0,0 }) : TPInfo(type, msg), File(file), Position(pos)
        { }
    );
    NailangParseException(const std::u16string_view msg, const std::u16string_view file = {}, std::pair<size_t, size_t> pos = { 0,0 })
        : NailangParseException(T_<ExceptionInfo>{}, msg, file, pos)
    { }
    std::u16string_view GetFileName() const noexcept { return GetInfo().File; }
    virtual std::pair<size_t, size_t> GetPosition() const noexcept { return GetInfo().Position; }
};
class NAILANGAPI UnexpectedTokenException : public NailangParseException
{
    friend class NailangParser;
    PREPARE_EXCEPTION(UnexpectedTokenException, NailangParseException,
        common::parser::DetailToken Token;
        ExceptionInfo(const std::u16string_view msg, const common::parser::DetailToken& token, const std::u16string_view file) noexcept
            : TPInfo(TYPENAME, msg, file), Token(token) { }
    );
    UnexpectedTokenException(const std::u16string_view msg, const common::parser::DetailToken& token,
        const std::u16string_view file = {})
        : NailangParseException(T_<ExceptionInfo>{}, msg, token, file)
    { }
    std::pair<size_t, size_t> GetPosition() const noexcept final 
    {
        const auto& token = GetInfo().Token;
        return { token.Row + 1, token.Col };
    }
};


class NAILANGAPI NailangParser : public common::parser::ParserBase
{
private:
    MemoryPool& MemPool;
    std::u16string SubScopeName;

    constexpr std::pair<uint32_t, uint32_t> GetCurPos(const bool ignoreCol = false) const noexcept
    {
        return { gsl::narrow_cast<uint32_t>(Context.Row + 1), ignoreCol ? 0u : gsl::narrow_cast<uint32_t>(Context.Col) };
    }
    static constexpr std::pair<uint32_t, uint32_t> GetPosition(const common::parser::DetailToken& token) noexcept
    {
        return { token.Row + 1, token.Col };
    }
    [[nodiscard]] std::u16string DescribeTokenID(const uint16_t tid) const noexcept override;
    [[nodiscard]] common::str::StrVariant<char16_t> GetCurrentFileName() const noexcept override;
    virtual void HandleException(const NailangParseException& ex) const;
    common::parser::DetailToken OnUnExpectedToken(const common::parser::DetailToken& token, const std::u16string_view extraInfo = {}) const override;

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
    void EatSemiColon();

    [[nodiscard]] FuncName* CreateFuncName(std::u32string_view name, FuncName::FuncInfo info) const;
    void FillBlockName(RawBlock& block);
    void FillFileName(RawBlock& block) const noexcept;
public:
    NailangParser(MemoryPool& pool, common::parser::ParserContext& context, std::u16string subScope = u"") :
        ParserBase(context), MemPool(pool), SubScopeName(std::move(subScope)) { }
    virtual ~NailangParser() { }

    [[nodiscard]] FuncCall ParseFuncCall(std::u32string_view name, std::pair<uint32_t, uint32_t> pos, FuncName::FuncInfo info = FuncName::FuncInfo::Empty);
    [[nodiscard]] forceinline FuncCall ParseFuncCall(std::u32string_view name, FuncName::FuncInfo info = FuncName::FuncInfo::Empty)
    {
        return ParseFuncCall(name, GetCurPos(), info);
    }
    [[nodiscard]] forceinline FuncCall ParseFuncCall(const common::parser::DetailToken& token, FuncName::FuncInfo info = FuncName::FuncInfo::Empty)
    {
        return ParseFuncCall(token.GetString(), GetPosition(token), info);
    }
    [[nodiscard]] RawBlock ParseRawBlock(const std::u32string_view name, std::pair<uint32_t, uint32_t> pos);
    [[nodiscard]] forceinline RawBlock ParseRawBlock(const std::u32string_view name)
    {
        return ParseRawBlock(name, GetCurPos(true));
    }
    [[nodiscard]] forceinline RawBlock ParseRawBlock(const common::parser::DetailToken& token)
    {
        return ParseRawBlock(token.GetString(), GetPosition(token));
    }
    /*[[nodiscard]] AssignExpr ParseAssignExpr(const std::u32string_view var, std::pair<uint32_t, uint32_t> pos);
    [[nodiscard]] AssignExpr ParseAssignExpr(const std::u32string_view var)
    {
        return ParseAssignExpr(var, GetCurPos());
    }
    [[nodiscard]] AssignExpr ParseAssignExpr(const common::parser::DetailToken & token)
    {
        return ParseAssignExpr(token.GetString(), GetPosition(token));
    }*/
    enum class AssignPolicy { Disallow = 0, AllowVar = 1, AllowAny = 2 };
    [[nodiscard]] std::pair<Expr, char32_t> ParseExpr(std::string_view stopDelim, AssignPolicy policy = AssignPolicy::Disallow);
    [[nodiscard]] Expr ParseExprChecked(std::string_view stopDelims, std::u32string_view stopChecker, AssignPolicy policy = AssignPolicy::Disallow);
    void ParseContentIntoBlock(const bool allowNonBlock, Block& block, const bool tillTheEnd = true);

    [[nodiscard]] static Expr ProcessString(std::u32string_view str, MemoryPool& pool);
    [[nodiscard]] static FuncCall ParseFuncBody(std::u32string_view name, MemoryPool& pool, common::parser::ParserContext& context,
        FuncName::FuncInfo info = FuncName::FuncInfo::Empty);
    [[nodiscard]] static Expr ParseSingleExpr(MemoryPool& pool, common::parser::ParserContext& context, std::string_view stopDelims, std::u32string_view stopChecker);
    [[nodiscard]] static Expr ParseSingleExpr(MemoryPool& pool, common::parser::ParserContext& context, std::string_view stopDelims)
    {
        std::u32string stopChecker(stopDelims.begin(), stopDelims.end());
        return ParseSingleExpr(pool, context, stopDelims, stopChecker);
    }
    [[nodiscard]] static Expr ParseSingleExpr(MemoryPool& pool, common::parser::ParserContext& context)
    {
        return ParseSingleExpr(pool, context, ";", U";");
    }

    [[nodiscard]] RawBlockWithMeta GetNextRawBlock();
    [[nodiscard]] std::vector<RawBlockWithMeta> GetAllRawBlocks();

    [[nodiscard]] static Block ParseRawBlock(const RawBlock& block, MemoryPool& pool);
    [[nodiscard]] static Block ParseAllAsBlock(MemoryPool& pool, common::parser::ParserContext& context);

    [[nodiscard]] static std::optional<size_t> VerifyVariableName(const std::u32string_view name) noexcept;
};


class NAILANGAPI ReplaceEngine
{
protected:
    static std::u32string_view TrimStrBlank(const std::u32string_view str) noexcept;
    [[noreturn]] virtual void HandleException(const NailangParseException& ex) const;
    virtual void OnReplaceOptBlock(std::u32string& output, void* cookie, std::u32string_view cond, std::u32string_view content);
    virtual void OnReplaceVariable(std::u32string& output, void* cookie, std::u32string_view var);
    virtual void OnReplaceFunction(std::u32string& output, void* cookie, std::u32string_view func, common::span<const std::u32string_view> args);
    std::u32string ProcessOptBlock(const std::u32string_view source, const std::u32string_view prefix, const std::u32string_view suffix, void* cookie = nullptr);
    std::u32string ProcessVariable(const std::u32string_view source, const std::u32string_view prefix, const std::u32string_view suffix, void* cookie = nullptr);
    std::u32string ProcessFunction(const std::u32string_view source, const std::u32string_view prefix, const std::u32string_view suffix, void* cookie = nullptr);
public:
    virtual ~ReplaceEngine();
};


}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif
