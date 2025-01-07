#include "WindowManager.h"
#include "WindowHost.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/ThreadEx.h"
#include "SystemCommon/PromiseTaskSTD.h"
#include "common/StringEx.hpp"
#include "common/StringLinq.hpp"
#include "common/Linq2.hpp"


namespace xziar::gui
{
using common::BaseException;
using namespace std::string_view_literals;
}


#if COMMON_OS_UNIX
#  include "common/StaticLookup.hpp"
#  include <xkbcommon/xkbcommon.h>
namespace xziar::gui::detail
{
using xziar::gui::event::CombinedKey;
using xziar::gui::event::CommonKeys;
static constexpr auto KeyCodeLookup = BuildStaticLookup(xkb_keysym_t, CombinedKey,
    { XKB_KEY_F1,           CommonKeys::F1 },
    { XKB_KEY_F2,           CommonKeys::F2 },
    { XKB_KEY_F3,           CommonKeys::F3 },
    { XKB_KEY_F4,           CommonKeys::F4 },
    { XKB_KEY_F5,           CommonKeys::F5 },
    { XKB_KEY_F6,           CommonKeys::F6 },
    { XKB_KEY_F7,           CommonKeys::F7 },
    { XKB_KEY_F8,           CommonKeys::F8 },
    { XKB_KEY_F9,           CommonKeys::F9 },
    { XKB_KEY_F10,          CommonKeys::F10 },
    { XKB_KEY_F11,          CommonKeys::F11 },
    { XKB_KEY_F12,          CommonKeys::F12 },
    { XKB_KEY_KP_Left,      CommonKeys::Left },
    { XKB_KEY_Left,         CommonKeys::Left },
    { XKB_KEY_KP_Up,        CommonKeys::Up },
    { XKB_KEY_Up,           CommonKeys::Up },
    { XKB_KEY_KP_Right,     CommonKeys::Right },
    { XKB_KEY_Right,        CommonKeys::Right },
    { XKB_KEY_KP_Down,      CommonKeys::Down },
    { XKB_KEY_Down,         CommonKeys::Down },
    { XKB_KEY_Home,         CommonKeys::Home },
    { XKB_KEY_End,          CommonKeys::End },
    { XKB_KEY_Page_Up,      CommonKeys::PageUp },
    { XKB_KEY_Page_Down,    CommonKeys::PageDown },
    { XKB_KEY_Insert,       CommonKeys::Insert },
    { XKB_KEY_Control_L,    CommonKeys::Ctrl },
    { XKB_KEY_Control_R,    CommonKeys::Ctrl },
    { XKB_KEY_Shift_L,      CommonKeys::Shift },
    { XKB_KEY_Shift_R,      CommonKeys::Shift },
    { XKB_KEY_Alt_L,        CommonKeys::Alt },
    { XKB_KEY_Alt_R,        CommonKeys::Alt },
    { XKB_KEY_Escape,       CommonKeys::Esc },
    { XKB_KEY_BackSpace,    CommonKeys::Backspace },
    { XKB_KEY_Delete,       CommonKeys::Delete },
    { XKB_KEY_space,        CommonKeys::Space },
    { XKB_KEY_Tab,          CommonKeys::Tab },
    { XKB_KEY_KP_Tab,       CommonKeys::Tab },
    { XKB_KEY_Return,       CommonKeys::Enter },
    { XKB_KEY_KP_Enter,     CommonKeys::Enter },
    { XKB_KEY_Caps_Lock,    CommonKeys::CapsLock },
    { XKB_KEY_KP_Add,       '+' },
    { XKB_KEY_KP_Subtract,  '-' },
    { XKB_KEY_KP_Multiply,  '*' },
    { XKB_KEY_KP_Divide,    '/' },
    { XKB_KEY_comma,        ',' },
    { XKB_KEY_KP_Separator, ',' },
    { XKB_KEY_period,       '.' },
    { XKB_KEY_bracketleft,  '[' },
    { XKB_KEY_bracketright, ']' },
    { XKB_KEY_backslash,    '\\' },
    { XKB_KEY_slash,        '/' },
    { XKB_KEY_grave,        '`' },
    { XKB_KEY_semicolon,    ';' },
    { XKB_KEY_apostrophe,   '\'' }
);
CombinedKey ProcessXKBKey(void* state, uint8_t keycode) noexcept
{
    const auto keysym = xkb_state_key_get_one_sym(reinterpret_cast<xkb_state*>(state), keycode);
    if (keysym >= XKB_KEY_A && keysym <= XKB_KEY_Z)
        return static_cast<uint8_t>(keysym - XKB_KEY_A + 'A');
    if (keysym >= XKB_KEY_a && keysym <= XKB_KEY_z)
        return static_cast<uint8_t>(keysym - XKB_KEY_a + 'A');
    if (keysym >= XKB_KEY_0 && keysym <= XKB_KEY_9)
        return static_cast<uint8_t>(keysym - XKB_KEY_0 + '0');
    if (keysym >= XKB_KEY_KP_0 && keysym <= XKB_KEY_KP_9)
        return static_cast<uint8_t>(keysym - XKB_KEY_KP_0 + '0');
    //printf("check key %d.\n", keysym);
    return KeyCodeLookup(keysym).value_or(CommonKeys::UNDEFINE);
}

FileList UriStringToFiles(std::string_view str) noexcept
{
    FileList files;
    for (auto line : common::str::SplitStream(str, [](auto ch){ return ch == '\r' || ch == '\n'; }, false))
    {
        if (common::str::IsBeginWith(line, "file://")) // only accept local file
        {
            line.remove_prefix(7);
            files.AppendFile(common::str::to_u16string(line, common::str::Encoding::URI)); 
        }
    }
    return files;
}
}
#endif


#if defined(__has_include) && __has_include(<dbus/dbus.h>)
#   include <dbus/dbus.h>
#   define WDHOST_DBUS 1
#   pragma message("Compiling WindowHost with dbus[" DBUS_VERSION_STRING "]" )

namespace xziar::gui
{

struct DBErr
{
    DBusError Err = {};
    DBErr() noexcept
    {
        dbus_error_init(&Err);
    }
    ~DBErr()
    {
        dbus_error_free(&Err);
    }
    explicit operator bool() const noexcept { return dbus_error_is_set(&Err); }
    constexpr  DBusError* operator&() noexcept { return &Err; }
    std::u16string Msg() const noexcept { return common::str::to_u16string(Err.message, common::str::Encoding::UTF8); }
    BaseException Generate(std::u16string msg) const noexcept 
    {
        return BaseException(msg).Attach("detail", Msg());
    }
};

class MsgReader
{
    DBusMessageIter Iter;
    std::string_view ErrorMsg;
    uint32_t ErrorInfo = DBUS_TYPE_INVALID;
    bool ErrorFromSub = false;
    MsgReader(DBusMessageIter& parent) noexcept 
    {
        dbus_message_iter_recurse(&parent, &Iter);
    }
    forceinline void Expect_(std::string_view txt, int32_t typeval) noexcept
    {
        if (ErrorMsg.empty())
        {
            const auto type = dbus_message_iter_get_arg_type(&Iter);
            if (type == typeval)
                return;
            ErrorMsg = txt;
            ErrorInfo = type;
        }
    }
    template<typename T>
    forceinline void Expect_() noexcept
    {
        if constexpr (std::is_same_v<T, std::string_view> || std::is_same_v<T, std::string>)
            Expect_("string", DBUS_TYPE_STRING);
        else if constexpr (std::is_same_v<T, uint64_t>)
            Expect_("uint64", DBUS_TYPE_UINT64);
        else if constexpr (std::is_same_v<T, int64_t>)
            Expect_("int64", DBUS_TYPE_INT64);
        else if constexpr (std::is_same_v<T, uint32_t>)
            Expect_("uint32", DBUS_TYPE_UINT32);
        else if constexpr (std::is_same_v<T, int32_t>)
            Expect_("int32", DBUS_TYPE_INT32);
        else if constexpr (std::is_same_v<T, uint16_t>)
            Expect_("uint16", DBUS_TYPE_UINT16);
        else if constexpr (std::is_same_v<T, int16_t>)
            Expect_("int16", DBUS_TYPE_INT16);
        else if constexpr (std::is_same_v<T, double>)
            Expect_("fp64", DBUS_TYPE_DOUBLE);
        else if constexpr (std::is_same_v<T, bool>)
            Expect_("bool", DBUS_TYPE_BOOLEAN);
        else
            static_assert(!common::AlwaysTrue<T>, "unsupported type");
    }
    template<typename T>
    forceinline void ForceRead_(T& output) noexcept
    {
        if constexpr (std::is_same_v<T, std::string_view> || std::is_same_v<T, std::string>)
        {
            const char* str = nullptr;
            dbus_message_iter_get_basic(&Iter, &str);
            if (str)
                output = str;
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            int val;
            dbus_message_iter_get_basic(&Iter, &val);
            output = val;
        }
        else
            dbus_message_iter_get_basic(&Iter, &output);
    }
    template<typename T>
    forceinline T ForceRead_() noexcept
    {
        T output = {};
        ForceRead_(output);
        return output;
    }
    template<typename T>
    forceinline void Read_(T& output) noexcept
    {
        if (ErrorMsg.empty())
            ForceRead_(output);
    }
    forceinline bool CheckSubEnd(MsgReader& sub) noexcept
    {
        if (!sub.ErrorMsg.empty())
        {
            if (sub.ErrorInfo != DBUS_TYPE_INVALID) // not simply reach end
                ErrorMsg = sub.ErrorMsg, ErrorInfo = sub.ErrorInfo, ErrorFromSub = true;
            return true;
        }
        return false;
    }
    forceinline bool CheckSubValid(MsgReader& sub) noexcept
    {
        if (!sub.ErrorMsg.empty())
        {
            ErrorMsg = sub.ErrorMsg, ErrorInfo = sub.ErrorInfo, ErrorFromSub = true;
            return false;
        }
        return true;
    }
public:
    MsgReader(DBusMessage* msg) noexcept
    {
        if (!dbus_message_iter_init(msg, &Iter))
            ErrorMsg = "empty", ErrorInfo = DBUS_TYPE_INVALID;
    }
    explicit constexpr operator bool() const noexcept { return ErrorMsg.empty(); }

    template<typename T>
    forceinline MsgReader& Expect() noexcept
    {
        Expect_<T>();
        if (ErrorMsg.empty())
            dbus_message_iter_next(&Iter);
        return *this;
    }
    forceinline MsgReader& Expect(std::string_view txt, int32_t typeval) noexcept
    {
        Expect_(txt, typeval);
        if (ErrorMsg.empty())
            dbus_message_iter_next(&Iter);
        return *this;
    }
    template<typename T>
    forceinline MsgReader& Read(T& output) noexcept
    {
        Expect_<T>();
        Read_(output);
        if (ErrorMsg.empty())
            dbus_message_iter_next(&Iter);
        return *this;
    }
    template<typename T>
    forceinline MsgReader& ReadVariant(T& output) noexcept
    {
        Expect_("variant", DBUS_TYPE_VARIANT);
        if (ErrorMsg.empty())
        {
            MsgReader sub(Iter);
            sub.Read(output);
            if (CheckSubValid(sub))
                dbus_message_iter_next(&Iter);
        }
        return *this;
    }
    template<typename T>
    forceinline MsgReader& Read(T& output, std::string_view txt, int32_t typeval) noexcept
    {
        Expect_(txt, typeval);
        Read_(output);
        if (ErrorMsg.empty())
            dbus_message_iter_next(&Iter);
        return *this;
    }
    template<typename T>
    forceinline MsgReader& ReadArray(std::vector<T>& arr) noexcept
    {
        Expect_("array", DBUS_TYPE_ARRAY);
        if (ErrorMsg.empty())
        {
            MsgReader sub(Iter);
            while (true)
            {
                arr.push_back({});
                sub.Read(arr.back());
                if (CheckSubEnd(sub))
                {
                    arr.pop_back();
                    break;
                }
                // read next
            }
            dbus_message_iter_next(&Iter);
        }
        return *this;
    }
    template<typename F>
    forceinline MsgReader& ReadVarDict(F&& func) noexcept
    {
        Expect_("vardict", DBUS_TYPE_ARRAY);
        if (ErrorMsg.empty())
        {
            MsgReader sub(Iter);
            while (true)
            {
                sub.Expect_("dictentry", DBUS_TYPE_DICT_ENTRY);
                if (CheckSubEnd(sub))
                    break;
                MsgReader entry(sub.Iter);
                std::string_view key;
                entry.Read(key).Expect_("variant", DBUS_TYPE_VARIANT);
                if (!CheckSubValid(entry))
                    break;
                MsgReader val(entry.Iter);
                func(key, val);
                if (!CheckSubValid(val))
                    break;
                dbus_message_iter_next(&sub.Iter);
                // read next
            }
            if (ErrorMsg.empty())
                dbus_message_iter_next(&Iter);
        }
        return *this;
    }

    bool CheckStatus(common::mlog::MiniLogger<false>& logger) const noexcept
    {
        if (ErrorMsg.empty()) return true;
        logger.Warning(u"expects {} but get [{}({})].\n", ErrorMsg, ErrorInfo, ErrorInfo ? static_cast<char>(ErrorInfo) : ' ');
        return false;
    }
    bool CheckStatus(common::mlog::MiniLogger<false>& logger) noexcept
    {
        if (ErrorMsg.empty()) return true;
        std::string detail;
        switch (ErrorInfo)
        {
        case DBUS_TYPE_STRING:
        case DBUS_TYPE_OBJECT_PATH:
            ForceRead_(detail);
            break;
        case DBUS_TYPE_BOOLEAN: detail = ForceRead_<bool>() ? "true" : "false";  break;
        case DBUS_TYPE_INT16:   detail = std::to_string(ForceRead_<int16_t>());  break;
        case DBUS_TYPE_UINT16:  detail = std::to_string(ForceRead_<uint16_t>()); break;
        case DBUS_TYPE_INT32:   detail = std::to_string(ForceRead_<int32_t>());  break;
        case DBUS_TYPE_UINT32:  detail = std::to_string(ForceRead_<uint32_t>()); break;
        case DBUS_TYPE_INT64:   detail = std::to_string(ForceRead_<int64_t>());  break;
        case DBUS_TYPE_UINT64:  detail = std::to_string(ForceRead_<uint64_t>()); break;
        case DBUS_TYPE_DOUBLE:  detail = std::to_string(ForceRead_<double>());   break;
        default: break;
        }
        logger.Warning(u"expects {} but get [{}({})] : [{}].\n", ErrorMsg, ErrorInfo, ErrorInfo ? static_cast<char>(ErrorInfo) : ' ', detail);
        return false;
    }
};

template<typename... Ts>
struct DBMsgStruct
{
    static_assert(sizeof...(Ts) > 0);
    using Types = std::tuple<Ts...>;
};
template<typename T>
struct DBMsgArray
{
    using Type = T;
};
struct DBMsgVariant;
struct DBMsgTypes
{
    template<typename T>
    static constexpr inline bool IsComplex = common::is_specialization<T, DBMsgArray>::value || common::is_specialization<T, DBMsgStruct>::value;
    template<typename T>
    static constexpr std::pair<int, const char*> TypeSig() noexcept
    {
#define CASETYPE(t1, t2) if constexpr (std::is_same_v<T, t1>) return { DBUS_TYPE_##t2, DBUS_TYPE_##t2##_AS_STRING }
        if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> || std::is_same_v<T, const char*>)
            return { DBUS_TYPE_STRING, DBUS_TYPE_STRING_AS_STRING };
        else CASETYPE(uint64_t, UINT64);
        else CASETYPE( int64_t,  INT64);
        else CASETYPE(uint32_t, UINT32);
        else CASETYPE( int32_t,  INT32);
        else CASETYPE(uint16_t, UINT16);
        else CASETYPE( int16_t,  INT16);
        else CASETYPE(    bool, BOOLEAN);
        else
            static_assert(!common::AlwaysTrue<T>, "unsupported type");
#undef CASETYPE
    }
    template<typename T, uint8_t... Idxes>
    static constexpr void FillTypesStr(char*& str, size_t& size, std::integer_sequence<uint8_t, Idxes...>) noexcept
    {
        (..., FillTypeStr<std::tuple_element_t<Idxes, T>>(str, size));
    }
    template<typename T>
    static constexpr void FillTypeStr(char*& str, size_t& size) noexcept
    {
        if constexpr (common::is_specialization<T, DBMsgArray>::value)
        {
            if (size < 2) { size = 0; return; }
            *str++ = DBUS_TYPE_ARRAY_AS_STRING[0];
            size--;
            FillTypeStr<typename T::Type>(str, size);
        }
        else if constexpr (common::is_specialization<T, DBMsgStruct>::value)
        {
            using U = typename T::Types;
            constexpr auto Count = std::tuple_size_v<U>;
            if (size < Count + 2) { size = 0; return; }
            *str++ = DBUS_STRUCT_BEGIN_CHAR_AS_STRING[0];
            size--;
            FillTypesStr<U>(str, size, std::make_integer_sequence<uint8_t, Count>{});
            if (size < 1) { size = 0; return; }
            *str++ = DBUS_STRUCT_END_CHAR_AS_STRING[0];
            size--;
        }
        else if constexpr (std::is_same_v<T, DBMsgVariant>)
        {
            if (size < 1) { size = 0; return; }
            *str++ = DBUS_TYPE_VARIANT_AS_STRING[0];
            size--;
        }
        else
        {
            if (size < 1) { size = 0; return; }
            constexpr auto p = TypeSig<T>();
            *str++ = p.second[0];
            size--;
        }
    }
    template<typename T, size_t N = 512>
    static constexpr std::pair<std::array<char, N>, size_t> TypeToStr() noexcept
    {
        std::array<char, N> txt = {0};
        auto ptr = txt.data();
        auto size = N - 1;
        FillTypeStr<T>(ptr, size);
        return { txt, size };
    }
    template<size_t N>
    struct TypeStr
    {
        std::array<char, N> Txt = {0};
        template<size_t M>
        constexpr TypeStr(const std::array<char, M>& src) noexcept
        {
            static_assert(M >= N);
            for (size_t i = 0; i < N; ++i)
                Txt[i] = src[i];
        }
        constexpr operator const char*() const noexcept
        {
            return Txt.data();
        }
        constexpr operator std::string_view() const noexcept
        {
            return {Txt.data(), N - 1};
        }
    };
};
template<>
struct DBMsgTypes::TypeStr<0>
{
    template<size_t M>
    constexpr TypeStr(const std::array<char, M>&) noexcept { }
    constexpr operator const char*() const noexcept
    {
        return nullptr;
    }
    constexpr operator std::string_view() const noexcept
    {
        return {};
    }
};

#define DBusMsgTStr(...) []() noexcept                          \
{                                                               \
    constexpr auto Data = DBMsgTypes::TypeToStr<__VA_ARGS__>(); \
    constexpr auto Size = Data.second ?                         \
        Data.first.size() - Data.second : 0u;                   \
    constexpr DBMsgTypes::TypeStr<Size> Dummy(Data.first);      \
    return Dummy;                                               \
}()
[[maybe_unused]] constexpr auto OFFilter = DBusMsgTStr(DBMsgArray<DBMsgStruct<std::string, DBMsgArray<DBMsgStruct<uint32_t, std::string>>>>);
static_assert(OFFilter == "a(sa(us))"sv);

class MsgWriter;
class DictWriter;
template<typename T>
class ComplexWriter
{
    friend MsgWriter;
    template<typename> friend class ComplexWriter;
    DBusMessageIter& Host;
    bool IsValid;
    ComplexWriter(DBusMessageIter& host, bool isValid) noexcept : Host(host), IsValid(isValid) {}
public:
    bool Put(const T& val) noexcept
    {
        if (IsValid)
        {
            const auto [typeval, typestr] = DBMsgTypes::TypeSig<T>();
            if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
            {
                const auto ptr = val.data();
                IsValid = dbus_message_iter_append_basic(&Host, typeval, &ptr);
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                const auto val_ = val ? 1 : 0;
                IsValid = dbus_message_iter_append_basic(&Host, typeval, &val_);
            }
            else
                IsValid = dbus_message_iter_append_basic(&Host, typeval, &val);
        }
        return IsValid;
    }
    bool UpdateValid() noexcept { return IsValid; }
};
template<typename... Ts>
class ComplexWriter<DBMsgStruct<Ts...>>
{
    friend MsgWriter;
    template<typename> friend class ComplexWriter;
    static constexpr auto Indexes = std::make_integer_sequence<uint8_t, sizeof...(Ts)>{};
    DBusMessageIter& Host;
    DBusMessageIter Iter;
    uint32_t Count = 0;
    bool IsValid = true;
    ComplexWriter(DBusMessageIter& host, bool isValid) noexcept : Host(host), IsValid(isValid) 
    {
        dbus_message_iter_open_container(&host, DBUS_TYPE_STRUCT, nullptr, &Iter);
    }
    template<typename T, typename U>
    void Put_(const U& arg) noexcept
    {
        if (IsValid)
        {
            ComplexWriter<T> cur(Iter, IsValid);
            if constexpr (DBMsgTypes::IsComplex<T>)
            {
                arg(cur);
            }
            else
            {
                cur.Put(arg);
            }
            IsValid = cur.UpdateValid();
            Count++;
        }
    }
public:
    ~ComplexWriter()
    {
        if (IsValid)
            dbus_message_iter_close_container(&Host, &Iter);
        else
        {
            dbus_message_iter_abandon_container(&Host, &Iter);
        }
    }
    template<typename... Us>
    void Put(const Us&... args) noexcept
    {
        static_assert(sizeof...(Us) == sizeof...(Ts));
        (..., Put_<Ts>(args));
    }
    bool UpdateValid() noexcept { IsValid = IsValid && Count == sizeof...(Ts); return IsValid; }
};
template<typename T>
class ComplexWriter<DBMsgArray<T>>
{
    friend MsgWriter;
    template<typename> friend class ComplexWriter;
    DBusMessageIter& Host;
    DBusMessageIter Iter;
    bool IsValid = true;
    ComplexWriter(DBusMessageIter& host, bool isValid) noexcept : Host(host), IsValid(isValid) 
    {
        constexpr auto str = DBusMsgTStr(T);
        dbus_message_iter_open_container(&host, DBUS_TYPE_ARRAY, str, &Iter);
    }
public:
    ~ComplexWriter()
    {
        if (IsValid)
            dbus_message_iter_close_container(&Host, &Iter);
        else
        {
            dbus_message_iter_abandon_container(&Host, &Iter);
        }
    }
    template<typename U>
    ComplexWriter& Put(const U& arg) noexcept
    {
        if (IsValid)
        {
            ComplexWriter<T> cur(Iter, IsValid);
            if constexpr (common::is_specialization<T, DBMsgArray>::value || common::is_specialization<T, DBMsgStruct>::value)
            {
                arg(cur);
            }
            else
            {
                cur.Put(arg);
            }
            IsValid = cur.UpdateValid();
        }
        return *this;
    }
    bool UpdateValid() noexcept { return IsValid; }
};
template<>
class ComplexWriter<DBMsgVariant>
{
    friend MsgWriter;
    friend DictWriter;
    template<typename> friend class ComplexWriter;
protected:
    DBusMessageIter& Host;
    bool IsValid;
    bool HasPut = false;
    ComplexWriter(DBusMessageIter& host, bool isValid) noexcept : Host(host), IsValid(isValid) {}
public:
    template<typename T>
    bool Put(const T& val) noexcept
    {
        if (IsValid && !HasPut)
        {
            const auto [typeval, typestr] = DBMsgTypes::TypeSig<T>();
            DBusMessageIter var;
            dbus_message_iter_open_container(&Host, DBUS_TYPE_VARIANT, typestr, &var);
            {
                ComplexWriter<T> sub(var, IsValid);
                sub.Put(val);
                IsValid = sub.UpdateValid();
            }
            if (IsValid)
            {
                dbus_message_iter_close_container(&Host, &var);
            }
            else
            {
                dbus_message_iter_abandon_container(&Host, &var);
            }
            HasPut = true;
        }
        return IsValid;
    }
    template<typename T, typename F>
    bool PutComplex(F&& func) noexcept
    {
        if (IsValid && !HasPut)
        {
            constexpr auto typestr = DBusMsgTStr(T);
            DBusMessageIter var;
            dbus_message_iter_open_container(&Host, DBUS_TYPE_VARIANT, typestr, &var);
            {
                ComplexWriter<T> sub(var, IsValid);
                func(sub);
                IsValid = sub.UpdateValid();
            }
            if (IsValid)
            {
                dbus_message_iter_close_container(&Host, &var);
            }
            else
            {
                dbus_message_iter_abandon_container(&Host, &var);
            }
            HasPut = true;
        }
        return IsValid;
    }
    bool UpdateValid() noexcept { IsValid = IsValid && HasPut; return IsValid; }
};
class DictWriter
{
    friend MsgWriter;
protected:
    DBusMessageIter& Host;
    DBusMessageIter Iter;
    bool IsValid;
    DictWriter(DBusMessageIter& host, bool isValid) noexcept : Host(host), IsValid(isValid)
    {
        dbus_message_iter_open_container(&host, DBUS_TYPE_ARRAY, "{sv}", &Iter);
    }
public:
    ~DictWriter()
    {
        if (IsValid)
            dbus_message_iter_close_container(&Host, &Iter);
        else
        {
            dbus_message_iter_abandon_container(&Host, &Iter);
        }
    }
    template<typename T>
    DictWriter& Put(std::string_view key, const T& val) noexcept
    {
        if (IsValid)
        {
            DBusMessageIter entry;
            dbus_message_iter_open_container(&Iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
            const auto key_ = key.data();
            dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key_);
            {
                ComplexWriter<DBMsgVariant> var(entry, IsValid);
                var.Put(val);
                IsValid = var.UpdateValid();
            }
            if (IsValid)
            {
                dbus_message_iter_close_container(&Iter, &entry);
            }
            else
            {
                dbus_message_iter_abandon_container(&Iter, &entry);
            }
        }
        return *this;
    }
    template<typename T, typename F>
    DictWriter& PutComplex(std::string_view key, F&& func) noexcept
    {
        if (IsValid)
        {
            DBusMessageIter entry;
            dbus_message_iter_open_container(&Iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
            const auto key_ = key.data();
            dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key_);
            ComplexWriter<DBMsgVariant> var(entry, IsValid);
            var.PutComplex<T>(func);
            IsValid = var.UpdateValid();
            if (IsValid)
            {
                dbus_message_iter_close_container(&Iter, &entry);
            }
            else
            {
                dbus_message_iter_abandon_container(&Iter, &entry);
            }
        }
        return *this;
    }
};

class MsgWriter
{
protected:
    DBusMessageIter Iter;
    bool IsValid = true;
public:
    MsgWriter(DBusMessage* msg) noexcept
    { 
        dbus_message_iter_init_append(msg, &Iter);
    }
    explicit constexpr operator bool() const noexcept { return IsValid; }

    template<typename T>
    MsgWriter& Append(const T& val) noexcept
    {
        if (IsValid)
        {
            const auto [typeval, typestr] = DBMsgTypes::TypeSig<T>();
            if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
            {
                const auto ptr = val.data();
                IsValid = dbus_message_iter_append_basic(&Iter, typeval, &ptr);
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                const auto val_ = val ? 1 : 0;
                IsValid = dbus_message_iter_append_basic(&Iter, typeval, &val_);
            }
            else
                IsValid = dbus_message_iter_append_basic(&Iter, typeval, &val);
        }
        return *this;
    }
    template<size_t N>
    MsgWriter& Append(const char(&val)[N]) noexcept
    {
        return Append(std::string_view{val});
    }
    template<typename F>
    MsgWriter& AppendVarDict(F&& func) noexcept
    {
        if (IsValid)
        {
            DictWriter sub(Iter, IsValid);
            func(sub);
            IsValid = sub.IsValid;
        }
        return *this;
    }
    template<typename T, typename F>
    MsgWriter& AppendComplex(F&& func) noexcept
    {
        if (IsValid)
        {
            ComplexWriter<T> sub(Iter, IsValid);
            func(sub);
            IsValid = sub.UpdateValid();
        }
        return *this;
    }
};


class XDGInteraction final : public IFilePicker, public common::loop::LoopBase
{
    struct MessageNode : public common::NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<MessageNode>
    {
        XDGInteraction* Host;
        std::string ObjPath;
        std::promise<FileList> Pms;
        DBusMessage* Message;
        DBusPendingCall* Pending;
        common::SimpleTimer Timer;
        MessageNode(XDGInteraction* host, DBusMessage* msg) noexcept : Host(host), Message(msg), Pending(nullptr) { }
        ~MessageNode() { }
        // Pms set when return false
        bool ReadObjPath() noexcept
        {
            const auto reply = dbus_pending_call_steal_reply(Pending);
            dbus_pending_call_unref(Pending);
            Pending = nullptr;
            Host->Logger.Verbose(u"dbus get a reply.\n");
            if (DBErr error; dbus_set_error_from_message(&error, reply))
            {
                Pms.set_exception(std::make_exception_ptr(error.Generate(u"dbus reply is error")));
                dbus_message_unref(reply);
                return false;
            }
            else
            {
                MsgReader reader(reply);
                reader.Read(ObjPath, "objpath", DBUS_TYPE_OBJECT_PATH);
                reader.CheckStatus(Host->Logger);
                dbus_message_unref(reply);
            }
            if (ObjPath.empty())
            {
                Pms.set_exception(std::make_exception_ptr(BaseException(u"dbus reply is not objpath")));
                return false;
            }
            Host->Logger.Info(u"OpenFile recived objpath: [{}].\n", ObjPath);
            return true;
        }
        // Pms set when return true
        bool Handle(DBusMessage* msg) noexcept
        {
            auto& logger = Host->Logger;
            MsgReader reader(msg);
            
            uint32_t resp = 0;
            if (!reader.Read(resp))
                return reader.CheckStatus(logger);
            switch (resp)
            {
            case 0: // success
                break;
            case 1: // user cancel
                logger.Verbose(u"cancelled.\n");
                Pms.set_value({});
                return true;
            default:
            case 2: // error
                logger.Warning(u"errored.\n");
                return false;
            }

            FileList ret;
            reader.ReadVarDict([&](std::string_view key, MsgReader& val)
            {
                if (key != "uris"sv)
                {
                    logger.Verbose(u"skip key: [{}]\n", key);
                    return;
                }
                std::vector<std::string_view> urls;
                val.ReadArray(urls);
                for (auto url : urls)
                {
                    if (common::str::IsBeginWith(url, "file://")) // only accept local file
                    {
                        url.remove_prefix(7);
                        ret.AppendFile(common::str::to_u16string(url, common::str::Encoding::URI));
                    }
                }
            });
            if (!reader)
                return reader.CheckStatus(logger);
            
            logger.Verbose(u"finish reading.\n");
            Pms.set_value(std::move(ret));
            return true;
        }
    };
    DBusConnection* Bus = nullptr;
    common::container::IntrusiveDoubleLinkList<MessageNode, common::spinlock::WRSpinLock> MessageList;
    common::mlog::MiniLogger<false> Logger{u"XDGHost", { common::mlog::GetConsoleBackend() }};
    common::SimpleTimer Timer;

    //static DBusHandlerResult OnObjPathMessage(DBusConnection* connection, DBusMessage* msg, void* user_data)
    //{
    //    auto& node = *reinterpret_cast<MessageNode*>(user_data);
    //    return DBUS_HANDLER_RESULT_HANDLED;
    //}
    //static void OnObjPathUnregister(DBusConnection* connection, void* user_data)
    //{
    //    auto& node = *reinterpret_cast<MessageNode*>(user_data);
    //}
    //static inline const DBusObjectPathVTable ObjPathVtable = 
    //{
    //    .unregister_function = &OnObjPathUnregister,
    //    .message_function = &OnObjPathMessage,
    //};

    static void FreeUser(void *) {}
    static void PendingNotify(DBusPendingCall* pending, void* user_data)
    {
        auto& node = *reinterpret_cast<MessageNode*>(user_data);
        Ensures(node.Pending == pending);
        if (dbus_pending_call_get_completed(pending))
        {
            if (node.ReadObjPath())
            {
                node.Timer.Stop();
                node.Host->Logger.Verbose(u"notify received in {}ms.\n", node.Timer.ElapseMs());
                return;
            }
        }
        else
            node.Host->Logger.Verbose(u"pending call expire.\n");
        dbus_pending_call_unref(pending);
        node.Pending = nullptr;
        node.Host->MessageList.PopNode(&node);
        delete(&node);
    }
    static DBusHandlerResult HandleMsg(DBusConnection* connection, DBusMessage* msg, void* user_data)
    {
        auto& self = *reinterpret_cast<XDGInteraction*>(user_data);
        if (!dbus_message_is_signal(msg, "org.freedesktop.portal.Request", "Response"))
        {
            self.Logger.Warning(u"dbus recived a unknown message.\n");
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        const std::string_view path = dbus_message_get_path(msg);
        self.Logger.Verbose(u"dbus recived a response at [{}].\n", path);

        auto target = self.MessageList.Begin(); 
        while (target != nullptr)
        {
            if (target->ObjPath == path)
                break;
            target = self.MessageList.ToNext(target);
        }
        if (target)
        {
            if (!target->Handle(msg))
                target->Pms.set_exception(std::make_exception_ptr(BaseException(u"bad reply")));
            self.MessageList.PopNode(target);
            delete(target);
        }
        else
            self.Logger.Warning(u"dbus not found corresponding request on [{}].\n", path);
        
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    MessageNode* RemoveTask(MessageNode& node, std::u16string_view info)
    {
        node.Pms.set_exception(std::make_exception_ptr(BaseException(info)));
        const auto next = MessageList.PopNode(&node);
        delete(&node);
        return next;
    };
public:
    uint32_t Version = 0;
    XDGInteraction() : LoopBase(LoopBase::GetThreadedExecutor)
    {
        dbus_threads_init_default();
        DBErr error;
        Bus = dbus_bus_get_private(DBUS_BUS_SESSION, &error);
        if (error)
            COMMON_THROW(BaseException, u"Failed to connect dbus").Attach("detail", error.Msg());
        dbus_connection_set_exit_on_disconnect(Bus, false);
        {
            const auto msg = dbus_message_new_method_call(
                "org.freedesktop.portal.Desktop",
                "/org/freedesktop/portal/desktop",
                "org.freedesktop.DBus.Properties",
                "Get");
            MsgWriter writer(msg);
            writer.Append("org.freedesktop.portal.FileChooser").Append("version");
            common::SimpleTimer timer;
            timer.Start();
            const auto reply = dbus_connection_send_with_reply_and_block(Bus, msg, 100, &error);
            dbus_message_unref(msg);
            if (error)
                COMMON_THROW(BaseException, u"Failed to Get FileChooser version").Attach("detail", error.Msg());
            timer.Stop();
            MsgReader reader{reply};
            reader.ReadVariant(Version);
            if (!reader.CheckStatus(Logger))
                COMMON_THROW(BaseException, u"Failed to Get FileChooser version");
            Logger.Info(u"FileChooser version [{}], returned in {}us\n", Version, timer.ElapseUs());
        }
		dbus_bus_add_match(Bus, "type='signal',interface='org.freedesktop.portal.Request',member='Response'", &error);
        dbus_connection_add_filter(Bus, &HandleMsg, this, &FreeUser);
        Start();
    }
    ~XDGInteraction() final
    {
        if (Bus) 
        {
            dbus_connection_close(Bus);
            dbus_connection_unref(Bus);
        }
    }
    bool OnStart(const common::ThreadObject& thr, std::any&) noexcept final 
    {
        thr.SetName(u"DBus-wdhost");
        thr.SetQoS(common::ThreadQoS::Background);
        Timer.Start();
        return true;
    }
    LoopBase::LoopAction OnLoop() final
    {
        Timer.Stop();
        Logger.Verbose(u"loop once, slept {}ms.\n", Timer.ElapseMs());
        bool hasUpdated = false;
        do
        {
            hasUpdated = false;
            while (true)
            {
                dbus_connection_read_write_dispatch(Bus, 10);
                if (dbus_connection_get_dispatch_status(Bus) == DBUS_DISPATCH_DATA_REMAINS || dbus_connection_has_messages_to_send(Bus))
                    hasUpdated = true;
                else
                    break; // finished rwd
            }

            for (auto target = MessageList.Begin(); target != nullptr;)
            {
                if (target->Message) // msg not send
                {
                    hasUpdated = true;
                    Logger.Verbose("send a msg.\n");
                    target->Timer.Start();
                    dbus_connection_send_with_reply(Bus, target->Message, &target->Pending, -1);
                    dbus_message_unref(target->Message);
                    target->Message = nullptr;
                    if (!target->Pending)
                    {
                        target = RemoveTask(*target, u"dbus connection closed");
                        continue;
                    }
                    dbus_pending_call_set_notify(target->Pending, &PendingNotify, target, &FreeUser);
                }
                else if (target->Pending)
                {
                    target->Timer.Stop();
                    if (target->Timer.ElapseMs() > 5000) // not received notify in 5s
                    {
                        hasUpdated = true;
                        //dbus_pending_call_set_notify(target->Pending, nullptr, nullptr, &FreeUser);
                        dbus_pending_call_cancel(target->Pending);
                        dbus_pending_call_unref(target->Pending);
                        target->Pending = nullptr;
                        target = RemoveTask(*target, u"timeout in 5s");
                        continue;
                    }
                }
                target = MessageList.ToNext(target);
            }
        } while (hasUpdated);
        Timer.Start();
        return LoopAction::SleepFor(MessageList.Begin() ? 200 : 5000); // when empty, increase sleep time
    }

    common::PromiseResult<FileList> OpenFilePicker(const FilePickerInfo& info) noexcept final
    {
        const auto msg = dbus_message_new_method_call(
            "org.freedesktop.portal.Desktop",
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.FileChooser",
            info.IsOpen ? "OpenFile" : "SaveFile");
        
        const auto window_handle = "parent_window";
        const auto title = common::str::to_string(info.Title, common::str::Encoding::UTF8);
        // Prepare method arguments
        MsgWriter writer(msg);
        writer.Append(window_handle)
            .Append(title)
            .AppendVarDict([&](auto& dict)
            {
                if (info.IsFolder)
                    dict.Put("directory", true);
                if (info.IsOpen && info.AllowMultiSelect)
                    dict.Put("multiple", true);
                if (!info.ExtensionFilters.empty())
                {
                    using TV = DBMsgArray<DBMsgStruct<std::string, DBMsgArray<DBMsgStruct<uint32_t, std::string>>>>;
                    dict.template PutComplex<TV>("filters", [&](auto& arr)
                    {
                        for (const auto& filter : info.ExtensionFilters)
                        {
                            arr.Put([&](auto& item)
                            {
                                item.Put(common::str::to_string(filter.first, common::str::Encoding::UTF8), [&](auto& exts)
                                    {
                                        for (const auto& ext : filter.second)
                                        {
                                            exts.Put([&](auto& stru)
                                            {
                                                std::string txt = "*.";
                                                // TODO: UTF pair awareness
                                                for (const auto ch : ext)
                                                {
                                                    txt.push_back('[');
                                                    txt.push_back(std::tolower(static_cast<char>(ch)));
                                                    txt.push_back(std::toupper(static_cast<char>(ch)));
                                                    txt.push_back(']');
                                                }
                                                //txt += common::str::to_string(ext, common::str::Encoding::UTF8);
                                                stru.Put(0u, txt);
                                            });
                                        }
                                    });
                            });
                        }
                    });
                }
            });

        const auto node = new MessageNode(this, msg);
        const auto ret = common::PromiseResultSTD<FileList>::Get(node->Pms.get_future());
        MessageList.AppendNode(node);
        Wakeup();
        return ret;
    }

    static inline const auto Dummy = []()
    {
        try
        {
            detail::RegisterInteraction(std::make_unique<XDGInteraction>());
            return true;
        }
        catch (BaseException& be)
        {
            detail::wdLog().Warning(u"{}: {}\n", be.Message(), be.GetDetailMessage());
        }
        catch(...) {}
        return false;
    }();
};


}

#endif

