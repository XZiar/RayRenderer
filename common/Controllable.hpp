#pragma once
#include "CommonRely.hpp"
#include "ContainerEx.hpp"
#include "Exceptions.hpp"
#include "3DBasic/miniBLAS.hpp"
#include <string>
#include <string_view>
#include <map>
#include <set>
#include <variant>
#include <any>
#include <functional>
#include <type_traits>


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4324)
#endif

namespace common
{

class Controllable
{
    friend struct ControlHelper;
public:
    using ControlArg = std::variant<bool, int32_t, uint64_t, float, std::pair<float, float>, miniBLAS::Vec3, miniBLAS::Vec4, std::string, std::u16string, std::any>;
    enum class ArgType : uint8_t { RawValue, Color, LongText };
    struct ControlItem
    {
        std::string Id;
        std::string Category;
        std::u16string Name;
        std::u16string Description;
        std::any Cookie;
        std::function<ControlArg(const Controllable&, const std::string&)> Getter;
        std::function<void(Controllable&, const std::string&, const ControlArg&)> Setter;
        size_t TypeIdx;
        ArgType Type;
        mutable bool IsEnable;
    };
private:
    std::map<std::string, std::u16string, std::less<>> Categories;
    std::map<std::string, ControlItem, std::less<>> ControlItems;
    template<typename T>
    class ItemPrep
    {
        friend class Controllable; // gcc&clang need it
        friend class ItemPrepNonType;
        ControlItem& Item;
        ItemPrep(ControlItem& item) : Item(item) {}
    public:
        template<typename Getter>
        ItemPrep<T>& RegistGetter(Getter getter) 
        { 
            using GetType = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<Getter, const Controllable&, const std::string&>>>;
            static_assert(std::is_same_v<GetType, T> || std::is_same_v<GetType, ControlArg>, "getter doesnot match item type");
            Item.Getter = getter; 
            return *this;
        }
        template<typename Setter>
        ItemPrep<T>& RegistSetter(Setter setter) { Item.Setter = setter; return *this; }
        template<typename D, typename G>
        ItemPrep<T>& RegistGetter(G(D::*getter)(void) const)
        { 
            using GetType = std::remove_cv_t<std::remove_reference_t<G>>;
            static_assert(std::is_convertible_v<GetType, T>, "getter's return cannot be convert to T");
            if constexpr (std::is_constructible_v<ControlArg, T>)
                Item.Getter = [getter](const Controllable& obj, const std::string&) { return (dynamic_cast<const D&>(obj).*getter)(); };
            else
                Item.Getter = [getter](const Controllable& obj, const std::string&) { return std::any((dynamic_cast<const D&>(obj).*getter)()); };
            return *this;
        }
        template<typename D, typename S>
        ItemPrep<T>& RegistSetter(void(D::*setter)(S))
        { 
            using SetType = std::remove_cv_t<std::remove_reference_t<S>>;
            static_assert(std::is_convertible_v<T, SetType>, "setter does not accept value of T");
            if constexpr (std::is_constructible_v<ControlArg, T>)
                Item.Setter = [setter](Controllable& obj, const std::string&, const ControlArg& arg) { (dynamic_cast<D&>(obj).*setter)(std::get<T>(arg)); };
            else
                Item.Setter = [setter](Controllable& obj, const std::string&, const ControlArg& arg) { (dynamic_cast<D&>(obj).*setter)(std::any_cast<T>(std::get<std::any>(arg))); };
            return *this;
        }
        template<bool CanWrite = true, bool CanRead = true, typename V = T>
        ItemPrep<T>& RegistObject(V& object) 
        { 
            if constexpr (CanWrite)
            {
                static_assert(std::is_convertible_v<T, V>, "object cannot be converted from value of T");
                if constexpr (std::is_constructible_v<ControlArg, T>)
                    Item.Setter = [&object](Controllable&, const std::string&, const ControlArg& arg) { object = std::get<T>(arg); };
                else
                    Item.Setter = [&object](Controllable&, const std::string&, const ControlArg& arg) { object = std::any_cast<V>(std::get<std::any>(arg)); };
            }
            if constexpr (CanRead)
            {
                static_assert(std::is_convertible_v<V, T>, "object cannot be converted to T");
                if constexpr (std::is_constructible_v<ControlArg, T>)
                    Item.Getter = [&object](const Controllable&, const std::string&) { return ControlArg(static_cast<T>(object)); };
                else
                    Item.Getter = [&object](const Controllable&, const std::string&) { return ControlArg(std::any(object)); };
            }
            return *this;
        }
        template<bool CanWrite = true, bool CanRead = true, typename D = Controllable, typename M = T>
        ItemPrep<T>& RegistMember(M D::*member)
        {
            if constexpr (CanWrite)
            {
                static_assert(std::is_convertible_v<T, M>, "object cannot be converted from value of T");
                if constexpr (std::is_constructible_v<ControlArg, T>)
                    Item.Setter = [member](Controllable& obj, const std::string&, const ControlArg& arg) { dynamic_cast<D&>(obj).*member = std::get<T>(arg); };
                else
                    Item.Setter = [member](Controllable& obj, const std::string&, const ControlArg& arg) { dynamic_cast<D&>(obj).*member = std::any_cast<T>(std::get<std::any>(arg)); };
            }
            if constexpr (CanRead)
            {
                static_assert(std::is_convertible_v<M, T>, "object cannot be converted to T");
                if constexpr (std::is_constructible_v<ControlArg, T>)
                {
                    Item.Getter = [member](const Controllable& obj, const std::string&) { return static_cast<const T&>(dynamic_cast<const D&>(obj).*member); };
                }
                else
                {
                    Item.Getter = [member](const Controllable& obj, const std::string&) { return std::any(dynamic_cast<const D&>(obj).*member); };
                }
            }
            return *this;
        }
        template<typename D, bool CanWrite = true, bool CanRead = true, typename P = T&(D&)>
        ItemPrep<T>& RegistMemberProxy(P&& proxy)
        {
            if constexpr (CanWrite)
            {
                using SetType = std::invoke_result_t<P, D&>;
                static_assert(std::is_lvalue_reference_v<SetType>, "object is not a lvalue reference when being set");
                static_assert(std::is_assignable_v<SetType, T>, "object cannot be assigned from value of T when being set");
                if constexpr (std::is_constructible_v<ControlArg, T>)
                    Item.Setter = [proxy](Controllable& obj, const std::string&, const ControlArg& arg) { proxy(dynamic_cast<D&>(obj)) = std::get<T>(arg); };
                else
                    Item.Setter = [proxy](Controllable& obj, const std::string&, const ControlArg& arg) { proxy(dynamic_cast<D&>(obj)) = std::any_cast<T>(std::get<std::any>(arg)); };
            }
            if constexpr (CanRead)
            {
                using GetType = std::invoke_result_t<P, const D&>;
                static_assert(std::is_convertible_v<GetType, T>, "object cannot be convert to T");
                if constexpr (std::is_constructible_v<ControlArg, T>)
                    Item.Getter = [proxy](const Controllable& obj, const std::string&) { return proxy(dynamic_cast<const D&>(obj)); };
                else
                    Item.Getter = [proxy](const Controllable& obj, const std::string&) { return std::any(proxy(dynamic_cast<const D&>(obj))); };
            }
            return *this;
        }
        ItemPrep<T>& SetCookie(const std::any& cookie) { Item.Cookie = cookie; return *this; }
        ItemPrep<T>& SetArgType(const ArgType argType) { Item.Type = argType; return *this; }
    };
    class ItemPrepNonType
    {
        friend class Controllable;
        ControlItem& Item;
        ItemPrepNonType(ControlItem& item) : Item(item) {}
    public:
        template<typename T>
        decltype(auto) AsType()
        {
            using RealType = std::conditional_t<std::is_constructible_v<ControlArg, T>, T, std::any>;
            Item.TypeIdx = common::get_variant_index_v<RealType, ControlArg>();
            return ItemPrep<RealType>(Item);
        }
    };
protected:
    Controllable() {}

    void AddCategory(const std::string category, const std::u16string name)
    {
        Categories[category] = name;
    }
    ItemPrepNonType RegistItem(const std::string& id, const std::string& category, const std::u16string& name, const ArgType argType,
        const std::any& cookie = {}, const std::u16string& description = u"")
    {
        Categories.try_emplace(category, category.cbegin(), category.cend());
        auto it = ControlItems.insert_or_assign(id, ControlItem{ id, category, name, description, cookie, {}, {}, std::variant_npos, argType, true }).first;
        return it->second;
    }
    template<typename T>
    decltype(auto) RegistItem(const std::string& id, const std::string& category, const std::u16string& name, const ArgType argType,
        const std::any& cookie = {}, const std::u16string& description = u"")
    {
        return RegistItem(id, category, name, argType, cookie, description).AsType<T>();
    }
    virtual std::u16string_view GetControlType() const = 0;
public:
    virtual ~Controllable() {}
    
};

struct ControlHelper
{
    static std::u16string_view GetControlType(const Controllable& control) { return control.GetControlType(); }
    static const auto& GetControlItems(const Controllable& control) { return control.ControlItems; }
    static const auto& GetCategories(const Controllable& control) { return control.Categories; }
    static const Controllable::ControlItem* GetControlItem(const Controllable& control, const std::string& id)
    {
        return common::container::FindInMap(control.ControlItems, id);
    }

    static Controllable::ControlArg ControllableGet(const Controllable& control, const std::string& id)
    {
        const auto item = GetControlItem(control, id);
        if (!item)
            COMMON_THROW(BaseException, u"No such target found for Controller");
        return item->Getter(control, id);
    }
    template<typename T>
    static T ControllableGet(const Controllable& control, const std::string& id)
    {
        return std::get<T>(ControllableGet(control, id));
    }

    static void ControllableSet(Controllable& control, const std::string& id, const Controllable::ControlArg& arg)
    {
        const auto item = GetControlItem(control, id);
        if (!item)
            COMMON_THROW(BaseException, u"No such target found for Controller");
        return item->Setter(control, id, arg);
    }
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
