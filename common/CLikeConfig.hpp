#pragma once

#include "CommonRely.hpp"
#include <string>
#include <string_view>
#include <map>
#include <variant>
#include <optional>


namespace common
{

class CLikeDefines
{
public:
    using ValType = std::variant<std::monostate, int64_t, uint64_t, double, std::string_view>;
private:
    using DefineVal = std::variant<std::monostate, int64_t, uint64_t, double, std::string, std::string_view>;

    std::map<std::string, DefineVal, std::less<>> Data;

    static constexpr ValType ToReadonlyVal(const DefineVal& val) noexcept
    {
        return std::visit([](const auto& obj) noexcept -> ValType
            {
                if constexpr (std::is_same_v<common::remove_cvref_t<decltype(obj)>, std::string>)
                    return std::string_view(obj);
                else
                    return obj;
            }, val);
    }
    template<typename T>
    static constexpr decltype(auto) TypeCast(T&& val) noexcept
    {
        using RealT = remove_cvref_t<T>;
        if constexpr (std::is_integral_v<RealT>)
        {
            if constexpr (std::is_unsigned_v<RealT>)
                return static_cast<uint64_t>(val);
            else
                return static_cast<int64_t>(val);
        }
        else
        {
            return std::forward<T>(val);
        }
    }
    struct ValStub
    {
        DefineVal& Val;

        constexpr ValStub(DefineVal& val) : Val(val) { }
        template<typename T>
        constexpr DefineVal& operator=(T&& val)
        {
            Val = TypeCast(std::forward<T>(val));
            return Val;
        }
    };
    struct NodeStub
    {
        std::string_view Key;
        ValType Val;
        NodeStub(const std::pair<const std::string, DefineVal>& node)
            : Key(node.first), Val(ToReadonlyVal(node.second)) { }
    };
    struct NodeIterator
    {
    private:
        using NodeType = typename decltype(Data)::const_iterator;
        NodeType Node;
    public:
        NodeIterator(NodeType&& node) : Node(std::move(node)) { }
        NodeStub operator*() const
        {
            return *Node;
        }
        bool operator!=(const NodeIterator& other) const
        {
            return Node != other.Node;
        }
        bool operator==(const NodeIterator& other) const
        {
            return Node == other.Node;
        }
        NodeIterator& operator++()
        {
            ++Node;
            return *this;
        }
    };
public:
    ValStub operator[](const std::string_view key)
    {
        auto node = Data.find(key);
        if (node == Data.end())
        {
            return Data[std::string(key)];
        }
        else
        {
            return node->second;
        }
    }
    template<typename T>
    bool AddIfNotExist(const std::string_view key, T&& val)
    {
        return Data.try_emplace(key.data(), std::forward<T>(val)).second;
    }
    bool AddIfNotExist(const std::string_view key)
    {
        return Data.try_emplace(key.data(), std::monostate{}).second;
    }
    template<typename T>
    void Add(const std::string_view key, T&& val)
    {
        Data[std::string(key)] = std::forward<T>(val);
    }
    void Add(const std::string_view key)
    {
        Data[std::string(key)] = std::monostate{};
    }
    void Remove(const std::string_view key)
    {
        if (auto node = Data.find(key); node != Data.end())
            Data.erase(node);
    }

    std::optional<ValType> operator[](const std::string_view key) const noexcept
    {
        return TryGet(key);
    }
    bool Has(const std::string_view key) const noexcept
    {
        return Data.find(key) != Data.cend();
    }
    std::optional<ValType> TryGet(const std::string_view key) const noexcept
    {
        auto node = Data.find(key);
        if (node == Data.cend())
            return {};
        else
            return ToReadonlyVal(node->second);
    }
    NodeIterator begin() const noexcept
    {
        return Data.cbegin();
    }
    NodeIterator end() const noexcept
    {
        return Data.cend();
    }
};


}
