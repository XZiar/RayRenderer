#pragma once
#include "common/CommonRely.hpp"
#include "common/ContainerEx.hpp"
#include "common/Exceptions.hpp"
#include "3DBasic/miniBLAS.hpp"
#include <string>
#include <string_view>
#include <map>
#include <set>
#include <variant>
#include <any>
#include <functional>
#include <type_traits>

namespace rayr
{


class /*RAYCOREAPI*/ Controllable
{
public:
    using ControlArg = std::variant<bool, int32_t, uint64_t, float, std::pair<float, float>, miniBLAS::Vec3, miniBLAS::Vec4, std::string, std::u16string, std::any>;
    enum class ArgType : uint8_t { RawValue, Color };
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
    std::set<ControlItem, common::container::SetKeyLess<ControlItem, &ControlItem::Id>> ControlItems;
protected:
    Controllable(const std::u16string& name) : Name(name) {}
    template<typename Func, typename T>
    Controllable(const std::u16string& name, Func&& func, T* self) : Name(name) { func(self); }

    void AddCategory(const std::string category, const std::u16string name)
    {
        Categories[category] = name;
    }
    template<typename T, typename Get, typename Set>
    void RegistControlItem(const std::string& id, const std::string& category, const std::u16string& name, Get&& getter, Set&& setter,
        const ArgType argType, const std::any& cookie = {}, const std::u16string& description = u"")
    {
        constexpr auto index = common::get_variant_index_v<T, ControlArg>();
        Categories.try_emplace(category, category.cbegin(), category.cend());
        ControlItem item{ id, category, name, description, cookie, getter, setter, index, argType, true };
        ControlItems.insert(item);
    }
    template<typename T, typename T1>
    void RegistControlItemDirect2(const std::string& id, const std::string& category, const std::u16string& name, T1& object,
        const ArgType argType, const std::any& cookie = {}, const std::u16string& description = u"")
    {
        if constexpr (std::is_constructible_v<ControlArg, T>)
        {
            static_assert(std::is_convertible_v<T, T1> && std::is_convertible_v<T1, T>, "cannot convert");
            RegistControlItem<T>(id, category, name,
                [&object](const Controllable&, const std::string&) { return ControlArg((T)object); },
                [&object](Controllable&, const std::string&, const ControlArg& arg) { object = std::get<T>(arg); },
                argType, cookie, description);
        }
        else
        {
            RegistControlItem<std::any>(id, category, name,
                [&object](const Controllable&, const std::string&) { return ControlArg(std::any(object)); },
                [&object](Controllable&, const std::string&, const ControlArg& arg)
                { object = std::any_cast<T1>(std::get<std::any>(arg)); },
                argType, cookie, description);
        }
    }
    template<typename T>
    void RegistControlItemDirect(const std::string& id, const std::string& category, const std::u16string& name, T& object,
        const ArgType argType, const std::any& cookie = {}, const std::u16string& description = u"")
    {
        if constexpr (std::is_constructible_v<ControlArg, T>)
        {
            RegistControlItem<T>(id, category, name,
                [&object](const Controllable&, const std::string&) { return ControlArg(object); },
                [&object](Controllable&, const std::string&, const ControlArg& arg) { object = std::get<T>(arg); },
                argType, cookie, description);
        }
        else
        {
            RegistControlItem<std::any>(id, category, name,
                [&object](const Controllable&, const std::string&) { return ControlArg(std::any(object)); },
                [&object](Controllable&, const std::string&, const ControlArg& arg) 
                { object = std::any_cast<T>(std::get<std::any>(arg)); },
                argType, cookie, description);
        }
    }
    template<typename T, typename D, typename G, typename S>
    void RegistControlItemInDirect(const std::string& id, const std::string& category, const std::u16string& name,
        G(D::*getter)(void) const, void(D::*setter)(S),
        const ArgType argType, const std::any& cookie = {}, const std::u16string& description = u"")
    {
        using GetType = std::remove_cv_t<std::remove_reference_t<G>>;
        using SetType = std::remove_cv_t<std::remove_reference_t<S>>;
        static_assert(std::is_convertible_v<GetType, T>, "getter's return cannot be convert to T");
        static_assert(std::is_convertible_v<T, SetType>, "setter does not accept value of T");
        if constexpr (std::is_constructible_v<ControlArg, T>)
        {
            RegistControlItem<T>(id, category, name,
                [getter](const Controllable& obj, const std::string&) { return (dynamic_cast<const D&>(obj).*getter)(); },
                [setter](Controllable& obj, const std::string&, const ControlArg& arg) { (dynamic_cast<D&>(obj).*setter)(std::get<T>(arg)); },
                argType, cookie, description);
        }
        else
        {
            RegistControlItem<std::any>(id, category, name,
                [getter](const Controllable& obj, const std::string&) { return std::any((dynamic_cast<const D&>(obj).*getter)()); },
                [setter](Controllable& obj, const std::string&, const ControlArg& arg) 
                { (dynamic_cast<D&>(obj).*setter)(std::any_cast<T>(std::get<std::any>(arg))); },
                argType, cookie, description);
        }
    }
    template<typename T, typename D, typename M>
    void RegistControlItemInDirect(const std::string& id, const std::string& category, const std::u16string& name,
        M D::*member, const ArgType argType, const std::any& cookie = {}, const std::u16string& description = u"")
    {
        static_assert(std::is_convertible_v<M, T>, "member cannot be convert to T");
        static_assert(std::is_convertible_v<T, M>, "member cannot be convert from T");
        if constexpr (std::is_constructible_v<ControlArg, T>)
        {
            RegistControlItem<T>(id, category, name,
                [member](const Controllable& obj, const std::string&) { return dynamic_cast<const D&>(obj).*member; },
                [member](Controllable& obj, const std::string&, const ControlArg& arg) { dynamic_cast<D&>(obj).*member = std::get<T>(arg); },
                argType, cookie, description);
        }
        else
        {
            RegistControlItem<std::any>(id, category, name,
                [member](const Controllable& obj, const std::string&) { return std::any(dynamic_cast<const D&>(obj).*member); },
                [member](Controllable& obj, const std::string&, const ControlArg& arg)
                { dynamic_cast<D&>(obj).*member = std::any_cast<T>(std::get<std::any>(arg)); },
                argType, cookie, description);
        }
    }
    template<typename T, typename D, typename Func, typename = std::enable_if_t<std::is_invocable_v<Func, D&> && std::is_invocable_v<Func, const D&>>>
    void RegistControlItemInDirect(const std::string& id, const std::string& category, const std::u16string& name,
        Func&& func, const ArgType argType, const std::any& cookie = {}, const std::u16string& description = u"")
    {
        using GetType = std::invoke_result_t<Func, const D&>;
        using SetType = std::invoke_result_t<Func, D&>;
        static_assert(std::is_convertible_v<GetType, T>, "object cannot be convert to T");
        static_assert(std::is_lvalue_reference_v<SetType>, "object is not a lvalue reference when being set");
        static_assert(std::is_assignable_v<SetType, T>, "object cannot be assigned from value of T when being set");
        if constexpr (std::is_constructible_v<ControlArg, T>)
        {
            RegistControlItem<T>(id, category, name,
                [func](const Controllable& obj, const std::string&) { return func(dynamic_cast<const D&>(obj)); },
                [func](Controllable& obj, const std::string&, const ControlArg& arg) { func(dynamic_cast<D&>(obj)) = std::get<T>(arg); },
                argType, cookie, description);
        }
        else
        {
            RegistControlItem<std::any>(id, category, name,
                [func](const Controllable& obj, const std::string&) { return std::any(func(dynamic_cast<const D&>(obj))); },
                [func](Controllable& obj, const std::string&, const ControlArg& arg)
                { func(dynamic_cast<D&>(obj)) = std::any_cast<T>(std::get<std::any>(arg)); },
                argType, cookie, description);
        }
    }
public:
    const u16string Name;
    virtual ~Controllable() {}
    virtual u16string_view GetControlType() = 0;
    const auto& GetControlItems() const { return ControlItems; }
    const auto& GetCategories() const { return Categories; }
    const ControlItem* GetControlItem(const std::string& id) const
    { 
        return common::container::FindInSet(ControlItems, id);
    }

    ControlArg ControllableGet(const std::string& id) const
    {
        const auto item = GetControlItem(id);
        if (!item)
            COMMON_THROW(BaseException, u"No such target found for Controller");
        return item->Getter(*this, id);
    }
    template<typename T>
    T ControllableGet(const std::string& id) const
    {
        return std::get<T>(ControllableGet(id));
    }

    void ControllableSet(const std::string& id, const ControlArg& arg)
    {
        const auto item = GetControlItem(id);
        if (!item)
            COMMON_THROW(BaseException, u"No such target found for Controller");
        return item->Setter(*this, id, arg);
    }
};


}
