#include "RenderCoreWrapRely.h"
#include "ControllableWrap.h"
#include "b3d.hpp"


namespace RayRender
{

using std::string;
using std::u16string;

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
static void SetArg(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id, const float x, const float y, const float z)
{
    item->Setter(*control, id, miniBLAS::Vec3(x, y, z));
}
static void SetArg(const rayr::Controllable::ControlItem* item, const std::shared_ptr<rayr::Controllable>& control, const string& id, const float x, const float y, const float z, const float w)
{
    item->Setter(*control, id, miniBLAS::Vec4(x, y, z, w));
}
#pragma managed(pop)

static constexpr size_t ValIndexBool    = common::get_variant_index_v<bool, rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexInt32   = common::get_variant_index_v<int32_t, rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexUInt64  = common::get_variant_index_v<uint64_t, rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexFloat   = common::get_variant_index_v<float, rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexVec3    = common::get_variant_index_v<miniBLAS::Vec3, rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexVec4    = common::get_variant_index_v<miniBLAS::Vec4, rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexStr     = common::get_variant_index_v<string, rayr::Controllable::ControlArg>();
static constexpr size_t ValIndexU16Str  = common::get_variant_index_v<u16string, rayr::Controllable::ControlArg>();

System::Collections::Generic::IEnumerable<String^>^ Controllable::GetDynamicMemberNames()
{
    const auto control = GetControl();
    if (!control) return nullptr;
    const auto& items = control->GetControlItems();
    array<String^>^ names = gcnew array<String^>(static_cast<int32_t>(items.size()));
    int32_t i = 0;
    for (const auto& item : items)
    {
        names[i++] = ToStr(item.Id);
    }
    return names;
}

bool Controllable::TryGetMember(GetMemberBinder^ binder, [Out] Object^% arg)
{
    const auto id = ToCharStr(binder->Name);
    const auto control = GetControl();
    if (!control) return false;
    const auto item = control->GetControlItem(id);
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
        case ValIndexVec3:      arg = gcnew Basic3D::Vec3F(GetVec3(item, control, id)); break;
        case ValIndexVec4:      arg = gcnew Basic3D::Vec4F(GetVec4(item, control, id)); break;
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
    default:                return false;
    }
    return true;
}

bool Controllable::TrySetMember(SetMemberBinder^ binder, Object^ arg)
{
    const auto id = ToCharStr(binder->Name);
    const auto control = GetControl();
    if (!control) return false;
    const auto item = control->GetControlItem(id);
    if (!item) return false;
    switch (item->Type)
    {
    case rayr::Controllable::ArgType::RawValue:
        switch (item->TypeIdx)
        {
        case ValIndexBool:      SetArg(item, control, id, safe_cast<bool>(arg)); break;
        case ValIndexInt32:     SetArg(item, control, id, safe_cast<int32_t>(Convert::ChangeType(arg, int32_t::typeid))); break;
        case ValIndexUInt64:    SetArg(item, control, id, safe_cast<uint64_t>(Convert::ChangeType(arg, uint64_t::typeid))); break;
        case ValIndexFloat:     SetArg(item, control, id, safe_cast<float>(Convert::ChangeType(arg, float::typeid))); break;
        case ValIndexStr:       SetArg(item, control, id, ToCharStr(safe_cast<String^>(arg))); break;
        case ValIndexU16Str:    SetArg(item, control, id, ToU16Str(safe_cast<String^>(arg))); break;
        case ValIndexVec3:
        {
            const auto& tmp = safe_cast<Basic3D::Vec3F>(arg);
            SetArg(item, control, id, tmp.x, tmp.y, tmp.z);
        } break;
        case ValIndexVec4:
        {
            const auto& tmp = safe_cast<Basic3D::Vec4F>(arg);
            SetArg(item, control, id, tmp.x, tmp.y, tmp.z, tmp.w);
        } break;
        default:                return false;
        } break;
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
    default:                return false;
    }
    ViewModel.OnPropertyChanged(binder->Name);
    return true;
}

}