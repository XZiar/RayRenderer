#include "RenderCoreWrapRely.h"
#include "ControllableWrap.h"

namespace Common
{
using namespace System::Linq;

using std::string;
using std::u16string;
using common::ControlHelper;
using System::Globalization::CultureInfo;

static constexpr size_t ValIndexBool    = common::get_variant_index_v<bool,                     common::Controllable::ControlArg>();
static constexpr size_t ValIndexUInt16  = common::get_variant_index_v<uint16_t,                 common::Controllable::ControlArg>();
static constexpr size_t ValIndexInt32   = common::get_variant_index_v<int32_t,                  common::Controllable::ControlArg>();
static constexpr size_t ValIndexUInt32  = common::get_variant_index_v<uint32_t,                 common::Controllable::ControlArg>();
static constexpr size_t ValIndexUInt64  = common::get_variant_index_v<uint64_t,                 common::Controllable::ControlArg>();
static constexpr size_t ValIndexFloat   = common::get_variant_index_v<float,                    common::Controllable::ControlArg>();
static constexpr size_t ValIndexFPair   = common::get_variant_index_v<std::pair<float, float>,  common::Controllable::ControlArg>();
static constexpr size_t ValIndexVec3    = common::get_variant_index_v<mbase::Vec3,              common::Controllable::ControlArg>();
static constexpr size_t ValIndexVec4    = common::get_variant_index_v<mbase::Vec4,              common::Controllable::ControlArg>();
static constexpr size_t ValIndexStr     = common::get_variant_index_v<string,                   common::Controllable::ControlArg>();
static constexpr size_t ValIndexU16Str  = common::get_variant_index_v<u16string,                common::Controllable::ControlArg>();


#pragma managed(push, off)
static bool GetBool(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control)
{
    return std::get<bool>(item->Getter(*control, item->Id));
}
static uint16_t GetUInt16(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control)
{
    return std::get<uint16_t>(item->Getter(*control, item->Id));
}
static int32_t GetInt32(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control)
{
    return std::get<int32_t>(item->Getter(*control, item->Id));
}
static uint32_t GetUInt32(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control)
{
    return std::get<uint32_t>(item->Getter(*control, item->Id));
}
static uint64_t GetUInt64(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control)
{
    return std::get<uint64_t>(item->Getter(*control, item->Id));
}
static float GetFloat(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control)
{
    return std::get<float>(item->Getter(*control, item->Id));
}
static std::pair<float, float> GetFloatPair(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control)
{
    return std::get<std::pair<float, float>>(item->Getter(*control, item->Id));
}
static string GetStr(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control)
{
    return std::get<string>(item->Getter(*control, item->Id));
}
static u16string GetU16Str(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control)
{
    return std::get<u16string>(item->Getter(*control, item->Id));
}
static std::tuple<float, float, float> GetVec3(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control)
{
    const auto& val = std::get<mbase::Vec3>(item->Getter(*control, item->Id));
    return { val.X,val.Y,val.Z };
}
static std::tuple<float, float, float, float> GetVec4(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control)
{
    const auto& val = std::get<mbase::Vec4>(item->Getter(*control, item->Id));
    return { val.X,val.Y,val.Z,val.W };
}
template<typename T>
static std::u16string_view GetEnum(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control)
{
    const auto enums = std::any_cast<common::Controllable::EnumSet<T>>(&(item->Cookie));
    return enums ? enums->ConvertTo(std::get<T>(item->Getter(*control, item->Id))) : std::u16string_view{};
}

static void SetArg(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control, const bool arg)
{
    item->Setter(*control, item->Id, arg);
}
static void SetArg(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control, const uint16_t arg)
{
    item->Setter(*control, item->Id, arg);
}
static void SetArg(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control, const int32_t arg)
{
    item->Setter(*control, item->Id, arg);
}
static void SetArg(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control, const uint32_t arg)
{
    item->Setter(*control, item->Id, arg);
}
static void SetArg(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control, const uint64_t arg)
{
    item->Setter(*control, item->Id, arg);
}
static void SetArg(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control, const float arg)
{
    item->Setter(*control, item->Id, arg);
}
static void SetArg(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control, string&& arg)
{
    item->Setter(*control, item->Id, arg);
}
static void SetArg(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control, u16string&& arg)
{
    item->Setter(*control, item->Id, arg);
}
static void SetArg(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control, const float x, const float y)
{
    item->Setter(*control, item->Id, std::pair(x, y));
}
static void SetArg(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control, const float x, const float y, const float z)
{
    item->Setter(*control, item->Id, mbase::Vec3(x, y, z));
}
static void SetArg(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control, const float x, const float y, const float z, const float w)
{
    item->Setter(*control, item->Id, mbase::Vec4(x, y, z, w));
}
template<typename T>
static void SetEnum(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control, const u16string& arg)
{
    const auto enums = std::any_cast<common::Controllable::EnumSet<T>>(&(item->Cookie));
    if (enums)
        item->Setter(*control, item->Id, enums->ConvertFrom(arg));
}
static void NotifyChanged(const common::Controllable::ControlItem* item, const std::shared_ptr<common::Controllable>& control)
{
    if (item->Notifier)
        item->Notifier(*control, item->Id);
}
#pragma managed(pop)



template<typename T>
static array<String^>^ ToStrArray(const vector<T>& src)
{
    auto arr = gcnew array<String^>((int32_t)src.size());
    int32_t i = 0;
    for (const auto& str : src)
        arr[i++] = ToStr(str);
    return arr;
}
static Object^ ParseCookie(const std::any& cookie)
{
    if (const auto ck = std::any_cast<std::pair<float, float>>(&cookie); ck)
        return gcnew Vector2(ck->first, ck->second);
    else if (const auto ck = std::any_cast<vector<string>>(&cookie); ck)
        return ToStrArray(*ck);
    else if (const auto ck = std::any_cast<vector<u16string>>(&cookie); ck)
        return ToStrArray(*ck);
    else if (const auto ck = std::any_cast<common::Controllable::EnumSet<uint16_t>>(&cookie); ck)
        return ToStrArray(ck->GetEnumNames());
    else if (const auto ck = std::any_cast<common::Controllable::EnumSet< int32_t>>(&cookie); ck)
        return ToStrArray(ck->GetEnumNames());
    else if (const auto ck = std::any_cast<common::Controllable::EnumSet<uint32_t>>(&cookie); ck)
        return ToStrArray(ck->GetEnumNames());
    else if (const auto ck = std::any_cast<common::Controllable::EnumSet<uint64_t>>(&cookie); ck)
        return ToStrArray(ck->GetEnumNames());
    else if (const auto ck = std::any_cast<std::pair<uint16_t, uint16_t>>(&cookie); ck)
        return gcnew Tuple<uint16_t, uint16_t>(ck->first, ck->second);
    else if (const auto ck = std::any_cast<std::pair< int32_t,  int32_t>>(&cookie); ck)
        return gcnew Tuple< int32_t,  int32_t>(ck->first, ck->second);
    else if (const auto ck = std::any_cast<std::pair<uint32_t, uint32_t>>(&cookie); ck)
        return gcnew Tuple<uint32_t, uint32_t>(ck->first, ck->second);
    else if (const auto ck = std::any_cast<std::pair<uint64_t, uint64_t>>(&cookie); ck)
        return gcnew Tuple<uint64_t, uint64_t>(ck->first, ck->second);
    return nullptr;
}
ControlItem::ControlItem(Common::Controllable^ control, String^ id, const common::Controllable::ControlItem& item)
{
    Control = control;
    ChangedEvent = gcnew PropertyChangedEventArgs(id);
    ValueChanged += gcnew ControlItemChangedEventHandler(Control, &Controllable::WhenPropertyChanged);
    Id = id;
    PtrItem = &item;
    Name = ToStr(item.Name);
    Category = ToStr(item.Category);
    Description = ToStr(item.Description);
    Cookie = ParseCookie(item.Cookie);
    Access = (item.Getter ? PropAccess::Read : PropAccess::Empty) | (item.Setter ? PropAccess::Write : PropAccess::Empty);
    Type = (PropType)item.Type;
    if (Type == PropType::Color) 
        ValType = System::Windows::Media::Color::typeid;
    else if (Type == PropType::Color)
        ValType = String::typeid;
    else
    {
        switch (item.TypeIdx)
        {
        case ValIndexBool:          ValType = bool::typeid; break;
        case ValIndexUInt16:        ValType = uint16_t::typeid; break;
        case ValIndexInt32:         ValType = int32_t::typeid; break;
        case ValIndexUInt32:        ValType = uint32_t::typeid; break;
        case ValIndexUInt64:        ValType = uint64_t::typeid; break;
        case ValIndexFloat:         ValType = float::typeid; break;
        case ValIndexFPair:         ValType = Vector2::typeid; break;
        case ValIndexVec3:          ValType = Vector3::typeid; break;
        case ValIndexVec4:          ValType = Vector4::typeid; break;
        case ValIndexStr:           ValType = String::typeid; break;
        case ValIndexU16Str:        ValType = String::typeid; break;
        default:                    ValType = Object::typeid; break;
        }
    }
}

bool ControlItem::GetValue(const std::shared_ptr<common::Controllable>& control, [Out] Object^% arg)
{
    const auto item = ControlHelper::GetControlItem(*control, PtrItem->Id);
    if (!item) return false;
    switch (PtrItem->Type)
    {
    case common::Controllable::ArgType::RawValue:
        switch (PtrItem->TypeIdx)
        {
        case ValIndexBool:      arg = GetBool(PtrItem, control); break;
        case ValIndexUInt16:    arg = GetUInt16(PtrItem, control); break;
        case ValIndexInt32:     arg = GetInt32(PtrItem, control); break;
        case ValIndexUInt32:    arg = GetUInt32(PtrItem, control); break;
        case ValIndexUInt64:    arg = GetUInt64(PtrItem, control); break;
        case ValIndexFloat:     arg = GetFloat(PtrItem, control); break;
        case ValIndexFPair:     arg = ToVector2(GetFloatPair(PtrItem, control)); break;
        case ValIndexVec3:      arg = ToVector3(GetVec3(PtrItem, control)); break;
        case ValIndexVec4:      arg = ToVector4(GetVec4(PtrItem, control)); break;
        case ValIndexStr:       arg = ToStr(GetStr(PtrItem, control)); break;
        case ValIndexU16Str:    arg = ToStr(GetU16Str(PtrItem, control)); break;
        default:                return false;
        } break;
    case common::Controllable::ArgType::Color:
        switch (PtrItem->TypeIdx)
        {
        case ValIndexVec3:      arg = ToColor(GetVec3(PtrItem, control)); break;
        case ValIndexVec4:      arg = ToColor(GetVec4(PtrItem, control)); break;
        default:                return false;
        } break;
    case common::Controllable::ArgType::LongText:
        switch (PtrItem->TypeIdx)
        {
        case ValIndexStr:       arg = ToStr(GetStr(PtrItem, control)); break;
        case ValIndexU16Str:    arg = ToStr(GetU16Str(PtrItem, control)); break;
        default:                return false;
        } break;
    case common::Controllable::ArgType::Enum:
        switch (PtrItem->TypeIdx)
        {
        case ValIndexStr:       arg = ToStr(GetStr(PtrItem, control)); break;
        case ValIndexU16Str:    arg = ToStr(GetU16Str(PtrItem, control)); break;
        case ValIndexUInt16:    arg = ToStr(GetEnum<uint16_t>(PtrItem, control)); break;
        case ValIndexInt32:     arg = ToStr(GetEnum< int32_t>(PtrItem, control)); break;
        case ValIndexUInt32:    arg = ToStr(GetEnum<uint32_t>(PtrItem, control)); break;
        case ValIndexUInt64:    arg = ToStr(GetEnum<uint64_t>(PtrItem, control)); break;
        default:                return false;
        } break;
    default:                return false;
    }
    return true;
}


template<typename T> static T ForceCast(Object^ value)
{
    return safe_cast<T>(Convert::ChangeType(value, T::typeid, CultureInfo::InvariantCulture));
}

bool ControlItem::SetValue(const std::shared_ptr<common::Controllable>& control, Object^ arg)
{
    const auto item = ControlHelper::GetControlItem(*control, PtrItem->Id);
    if (!item) return false;
    switch (PtrItem->Type)
    {
    case common::Controllable::ArgType::RawValue:
        switch (item->TypeIdx)
        {
        case ValIndexBool:      SetArg(item, control, safe_cast<bool>(arg)); break;
        case ValIndexUInt16:    SetArg(item, control, Convert::ToUInt16(arg)); break;
        case ValIndexInt32:     SetArg(item, control, Convert::ToInt32(arg)); break;
        case ValIndexUInt32:    SetArg(item, control, Convert::ToUInt32(arg)); break;
        case ValIndexUInt64:    SetArg(item, control, Convert::ToUInt64(arg)); break;
        case ValIndexFloat:     SetArg(item, control, Convert::ToSingle(arg)); break;
        case ValIndexStr:       SetArg(item, control, ToCharStr(safe_cast<String^>(arg))); break;
        case ValIndexU16Str:    SetArg(item, control, ToU16Str(safe_cast<String^>(arg))); break;
        case ValIndexFPair:
        {
            const auto& tmp = safe_cast<Vector2>(arg);
            SetArg(item, control, tmp.X, tmp.Y);
        } break;
        case ValIndexVec3:
        {
            const auto& tmp = safe_cast<Vector3>(arg);
            SetArg(item, control, tmp.X, tmp.Y, tmp.Z);
        } break;
        case ValIndexVec4:
        {
            const auto& tmp = safe_cast<Vector4>(arg);
            SetArg(item, control, tmp.X, tmp.Y, tmp.Z, tmp.W);
        } break;
        default:                return false;
        } break;
    case common::Controllable::ArgType::Color:
    {
        auto color = safe_cast<System::Windows::Media::Color>(arg);
        switch (item->TypeIdx)
        {
        case ValIndexVec3:  SetArg(item, control, color.ScR, color.ScG, color.ScB); break;
        case ValIndexVec4:  SetArg(item, control, color.ScR, color.ScG, color.ScB, color.ScA); break;
        default:                return false;
        }
    } break;
    case common::Controllable::ArgType::LongText:
        switch (item->TypeIdx)
        {
        case ValIndexStr:       SetArg(item, control, ToCharStr(safe_cast<String^>(arg))); break;
        case ValIndexU16Str:    SetArg(item, control, ToU16Str(safe_cast<String^>(arg))); break;
        default:                return false;
        }
        break;
    case common::Controllable::ArgType::Enum:
        switch (item->TypeIdx)
        {
        case ValIndexStr:       SetArg(item, control, ToCharStr(safe_cast<String^>(arg))); break;
        case ValIndexU16Str:    SetArg(item, control, ToU16Str(safe_cast<String^>(arg))); break;
        case ValIndexUInt16:    SetEnum<uint16_t>(item, control, ToU16Str(safe_cast<String^>(arg))); break;
        case ValIndexInt32:     SetEnum< int32_t>(item, control, ToU16Str(safe_cast<String^>(arg))); break;
        case ValIndexUInt32:    SetEnum<uint32_t>(item, control, ToU16Str(safe_cast<String^>(arg))); break;
        case ValIndexUInt64:    SetEnum<uint64_t>(item, control, ToU16Str(safe_cast<String^>(arg))); break;
        default:                return false;
        } break;
    default:                return false;
    }
    NotifyChanged(item, control);
    ValueChanged(ChangedEvent);
    return true;
}



Controllable::Controllable(const std::shared_ptr<common::Controllable>& control)
{
    Control = new std::weak_ptr<common::Controllable>(control);
    controlType = ToStr(ControlHelper::GetControlType(*control));
    Categories = gcnew Dictionary<String^, String^>(0);
    ControlItems = gcnew Dictionary<String^, ControlItem^>(0);
    RefreshControl();
}
Controllable::!Controllable()
{
    if (const auto ptr = ExchangeNullptr(Control); ptr)
        delete ptr;
}

void Controllable::RefreshControl()
{
    const auto control = GetControl();
    if (!control) return;
    for (const auto&[cat, des] : ControlHelper::GetCategories(*control))
    {
        Categories->Add(ToStr(cat), ToStr(des));
    }
    for (const auto&[id_, item] : ControlHelper::GetControlItems(*control))
    {
        auto id = ToStr(id_);
        ControlItems->Add(id, gcnew ControlItem(this, id, item));
    }
}



String^ GetControlItemIds(KeyValuePair<String^, ControlItem^> pair)
{
    return pair.Value->Id;
}

IEnumerable<String^>^ Controllable::GetDynamicMemberNames()
{
    return Enumerable::Select(ControlItems, gcnew Func<KeyValuePair<String^, ControlItem^>, String^>(GetControlItemIds));
}

bool Controllable::DoGetMember(String^ id, [Out] Object^% arg)
{
    ControlItem^ item = nullptr;
    if (!ControlItems->TryGetValue(id, item))
        return false;
    const auto control = GetControl();
    if (!control) 
        return false;
    return item->GetValue(control, arg);
}

bool Controllable::DoSetMember(String^ id, Object^ arg)
{
    ControlItem^ item = nullptr;
    if (!ControlItems->TryGetValue(id, item))
        return false;
    const auto control = GetControl();
    if (!control)
        return false;
    return item->SetValue(control, arg);
}

}