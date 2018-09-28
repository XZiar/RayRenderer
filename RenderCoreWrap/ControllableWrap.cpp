#include "RenderCoreWrapRely.h"
#include "ControllableWrap.h"

namespace RayRender
{
using namespace System::Linq;

using std::string;
using std::u16string;
using common::ControlHelper;
using System::Globalization::CultureInfo;

static constexpr size_t ValIndexBool    = common::get_variant_index_v<bool,                     rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexInt32   = common::get_variant_index_v<int32_t,                  rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexUInt64  = common::get_variant_index_v<uint64_t,                 rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexFloat   = common::get_variant_index_v<float,                    rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexFPair   = common::get_variant_index_v<std::pair<float, float>,  rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexVec3    = common::get_variant_index_v<miniBLAS::Vec3,           rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexVec4    = common::get_variant_index_v<miniBLAS::Vec4,           rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexStr     = common::get_variant_index_v<string,                   rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexU16Str  = common::get_variant_index_v<u16string,                rayr::Controllable::ControlArg>();


template<typename Char>
static array<String^>^ ToStrArray(const vector<std::basic_string<Char>>& src)
{
    auto arr = gcnew array<String^>((int32_t)src.size());
    int32_t i = 0;
    for (const auto& str : src)
        arr[i++] = ToStr(str);
    return arr;
}
ControlItem::ControlItem(const common::Controllable::ControlItem& item)
{
    Id = ToStr(item.Id);
    Name = ToStr(item.Name);
    Category = ToStr(item.Category);
    Description = ToStr(item.Description);
    if (const auto ck = std::any_cast<std::pair<float, float>>(&item.Cookie); ck)
        Cookie = gcnew System::Numerics::Vector2(ck->first, ck->second);
    else if (const auto ck = std::any_cast<vector<string>>(&item.Cookie); ck)
        Cookie = ToStrArray(*ck);
    else if (const auto ck = std::any_cast<vector<u16string>>(&item.Cookie); ck)
        Cookie = ToStrArray(*ck);
    PropAccess access;
    if (item.Getter) access = access | PropAccess::Read;
    if (item.Setter) access = access | PropAccess::Write;
    Access = access;
    Type = (PropType)item.Type;
    if (Type == PropType::Color) 
        ValType = System::Windows::Media::Color::typeid;
    else
    {
        switch (item.TypeIdx)
        {
        case ValIndexBool:          ValType = bool::typeid; break;
        case ValIndexInt32:         ValType = int32_t::typeid; break;
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


Controllable::Controllable(const std::shared_ptr<rayr::Controllable>& control)
{
    Control = new std::weak_ptr<rayr::Controllable>(control);
    controlType = ToStr(ControlHelper::GetControlType(*control));
    name = ToStr(ControlHelper::GetControlName(*control));
    Categories = gcnew Dictionary<String^, String^>(0);
    Items = gcnew List<ControlItem^>(0);
    RefreshControl();
}
Controllable::Controllable(Controllable^ other) : Controllable(other->Control->lock())
{
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
    for (const auto&[id, item] : ControlHelper::GetControlItems(*control))
    {
        Items->Add(gcnew ControlItem(item));
    }
}


#pragma managed(push, off)
static bool GetBool(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id)
{
    return std::get<bool>(item->Getter(*control, id));
}
static int32_t GetInt32(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id)
{
    return std::get<int32_t>(item->Getter(*control, id));
}
static uint64_t GetUInt64(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id)
{
    return std::get<uint64_t>(item->Getter(*control, id));
}
static float GetFloat(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id)
{
    return std::get<float>(item->Getter(*control, id));
}
static std::pair<float, float> GetFloatPair(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id)
{
    return std::get<std::pair<float, float>>(item->Getter(*control, id));
}
static string GetStr(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id)
{
    return std::get<string>(item->Getter(*control, id));
}
static u16string GetU16Str(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id)
{
    return std::get<u16string>(item->Getter(*control, id));
}
static const miniBLAS::Vec3& GetVec3(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id)
{
    return std::get<miniBLAS::Vec3>(item->Getter(*control, id));
}
static const miniBLAS::Vec4& GetVec4(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id)
{
    return std::get<miniBLAS::Vec4>(item->Getter(*control, id));
}

static void SetArg(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id, bool arg)
{
    item->Setter(*control, id, arg);
}
static void SetArg(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id, int32_t arg)
{
    item->Setter(*control, id, arg);
}
static void SetArg(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id, uint64_t arg)
{
    item->Setter(*control, id, arg);
}
static void SetArg(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id, float arg)
{
    item->Setter(*control, id, arg);
}
static void SetArg(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id, string&& arg)
{
    item->Setter(*control, id, arg);
}
static void SetArg(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id, u16string&& arg)
{
    item->Setter(*control, id, arg);
}
static void SetArg(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id, const float x, const float y)
{
    item->Setter(*control, id, std::pair(x, y));
}
static void SetArg(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id, const float x, const float y, const float z)
{
    item->Setter(*control, id, miniBLAS::Vec3(x, y, z));
}
static void SetArg(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id, const float x, const float y, const float z, const float w)
{
    item->Setter(*control, id, miniBLAS::Vec4(x, y, z, w));
}
#pragma managed(pop)

String^ GetControlItemIds(ControlItem^ item)
{
    return item->Id;
}

IEnumerable<String^>^ Controllable::GetDynamicMemberNames()
{
    return Enumerable::Select(Items, gcnew Func<ControlItem^, String^>(GetControlItemIds));
}

bool Controllable::DoGetMember(String^ id_, [Out] Object^% arg)
{
    const auto id = ToCharStr(id_);
    const auto control = GetControl();
    if (!control) return false;
    const auto item = ControlHelper::GetControlItem(*control, id);
    if (!item) return false;
    switch (item->Type)
    {
    case rayr::Controllable::ArgType::RawValue:
        switch (item->TypeIdx)
        {
        case ValIndexBool:      arg = GetBool(item, control, id); break;
        case ValIndexInt32:     arg = GetInt32(item, control, id); break;
        case ValIndexUInt64:    arg = GetUInt64(item, control, id); break;
        case ValIndexFloat:     arg = GetFloat(item, control, id); break;
        case ValIndexFPair:     arg = ToVector2(GetFloatPair(item, control, id)); break;
        case ValIndexVec3:      arg = ToVector3(GetVec3(item, control, id)); break;
        case ValIndexVec4:      arg = ToVector4(GetVec4(item, control, id)); break;
        case ValIndexStr:       arg = ToStr(GetStr(item, control, id)); break;
        case ValIndexU16Str:    arg = ToStr(GetU16Str(item, control, id)); break;
        default:                return false;
        } break;
    case rayr::Controllable::ArgType::Color:
        switch (item->TypeIdx)
        {
        case ValIndexVec3:      arg = ToColor(GetVec3(item, control, id)); break;
        case ValIndexVec4:      arg = ToColor(GetVec4(item, control, id)); break;
        default:                return false;
        } break;
    case rayr::Controllable::ArgType::LongText:
        switch (item->TypeIdx)
        {
        case ValIndexStr:       arg = ToStr(GetStr(item, control, id)); break;
        case ValIndexU16Str:    arg = ToStr(GetU16Str(item, control, id)); break;
        default:                return false;
        } break;
    default:                return false;
    }
    return true;
}

template<typename T> T ForceCast(Object^ value)
{
    return safe_cast<T>(Convert::ChangeType(value, T::typeid, CultureInfo::InvariantCulture));
}

bool Controllable::DoSetMember(String^ id_, Object^ arg)
{
    const auto id = ToCharStr(id_);
    const auto control = GetControl();
    if (!control) return false;
    const auto item = ControlHelper::GetControlItem(*control, id);
    if (!item) return false;
    switch (item->Type)
    {
    case rayr::Controllable::ArgType::RawValue:
        switch (item->TypeIdx)
        {
        case ValIndexBool:      SetArg(item, control, id, safe_cast<bool>(arg)); break;
        case ValIndexInt32:     SetArg(item, control, id, Convert::ToInt32(arg)); break;
        case ValIndexUInt64:    SetArg(item, control, id, Convert::ToUInt64(arg)); break;
        case ValIndexFloat:     SetArg(item, control, id, Convert::ToSingle(arg)); break;
        case ValIndexStr:       SetArg(item, control, id, ToCharStr(safe_cast<String^>(arg))); break;
        case ValIndexU16Str:    SetArg(item, control, id, ToU16Str(safe_cast<String^>(arg))); break;
        case ValIndexFPair:     
        {
            const auto& tmp = safe_cast<Vector2>(arg);
            SetArg(item, control, id, tmp.X, tmp.Y);
        } break;
        case ValIndexVec3:
        {
            const auto& tmp = safe_cast<Vector3>(arg);
            SetArg(item, control, id, tmp.X, tmp.Y, tmp.Z);
        } break;
        case ValIndexVec4:
        {
            const auto& tmp = safe_cast<Vector4>(arg);
            SetArg(item, control, id, tmp.X, tmp.Y, tmp.Z, tmp.W);
        } break;
        default:                return false;
        } 
        break;
    case rayr::Controllable::ArgType::Color:
    {
        auto color = safe_cast<System::Windows::Media::Color>(arg);
        switch (item->TypeIdx)
        {
        case ValIndexVec3:  SetArg(item, control, id, color.ScR, color.ScG, color.ScB); break;
        case ValIndexVec4:  SetArg(item, control, id, color.ScR, color.ScG, color.ScB, color.ScA); break;
        default:                return false;
        }
    } break;
    case rayr::Controllable::ArgType::LongText:
        switch (item->TypeIdx)
        {
        case ValIndexStr:       SetArg(item, control, id, ToCharStr(safe_cast<String^>(arg))); break;
        case ValIndexU16Str:    SetArg(item, control, id, ToU16Str(safe_cast<String^>(arg))); break;
        default:                return false;
        }
        break;
    default:                return false;
    }
    ViewModel.OnPropertyChanged(id_);
    return true;
}

}