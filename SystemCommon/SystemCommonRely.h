#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef SYSCOMMON_EXPORT
#   define SYSCOMMONAPI _declspec(dllexport)
# else
#   define SYSCOMMONAPI _declspec(dllimport)
# endif
#else
# define SYSCOMMONAPI [[gnu::visibility("default")]]
#endif


#include "common/CommonRely.hpp"
#include "common/Exceptions.hpp"
#include "common/EnumEx.hpp"
#include "common/StrBase.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <string_view>


namespace common
{
[[nodiscard]] SYSCOMMONAPI bool CheckCPUFeature(str::HashedStrView<char> feature) noexcept;
[[nodiscard]] SYSCOMMONAPI span<const std::string_view> GetCPUFeatures() noexcept;

class FastPathBase
{
public:
    class PathInfo
    {
        friend FastPathBase;
        class MethodInfo
        {
            friend PathInfo;
            friend FastPathBase;
            void* FuncPtr;
        public:
            str::HashedStrView<char> MethodName;
            MethodInfo(std::string_view name, void* ptr) noexcept : FuncPtr(ptr), MethodName(name) { }
        };
        void*& (*Access)(FastPathBase&) noexcept = nullptr;
    public:
        str::HashedStrView<char> FuncName;
        std::vector<MethodInfo> Variants;
        PathInfo(std::string_view name, void*& (*access)(FastPathBase&) noexcept) noexcept : Access(access), FuncName(name) { }
    };
    using VarItem = std::pair<std::string_view, std::string_view>;
    COMMON_NO_COPY(FastPathBase)
    COMMON_NO_MOVE(FastPathBase)
    [[nodiscard]] virtual bool IsComplete() const noexcept = 0;
    [[nodiscard]] common::span<const VarItem> GetIntrinMap() const noexcept
    {
        return VariantMap;
    }
protected:
    FastPathBase() noexcept {}
    SYSCOMMONAPI void Init(common::span<const PathInfo> info, common::span<const VarItem> requests) noexcept;
private:
    std::vector<VarItem> VariantMap;
};
namespace fastpath
{
struct PathHack;
}
template<typename T>
class RuntimeFastPath : public FastPathBase
{
private:
    using FastPathBase::Init;
protected:
    void Init(common::span<const VarItem> requests) noexcept { Init(T::GetSupportMap(), requests); }
};
}