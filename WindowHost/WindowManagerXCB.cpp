#include "WindowManager.h"
#include "WindowHost.h"
#include "SystemCommon/StringConvert.h"

#include "SystemCommon/Exceptions.h"
#include "common/ContainerEx.hpp"
#include "common/FrozenDenseSet.hpp"
#include "common/StaticLookup.hpp"
#include "common/StringEx.hpp"
#include "common/StringPool.hpp"
#include "common/StringLinq.hpp"
#include "common/MemoryStream.hpp"
#include "common/Linq2.hpp"

#include <boost/container/small_vector.hpp>

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/bigreq.h>
#include <xcb/dri2.h>
#include <xcb/dri3.h>
//#include <xcb/xcb_image.h>
#define explicit explicit_
#include <xcb/xkb.h>
#undef explicit
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#undef Always
#undef None

constexpr uint32_t MessageCreate    = 1;
constexpr uint32_t MessageTask      = 2;
constexpr uint32_t MessageClose     = 3;
constexpr uint32_t MessageStop      = 4;
constexpr uint32_t MessageDpi       = 5;
//constexpr uint32_t MessageBuildBG   = 6;
constexpr uint32_t MessageUpdTitle  = 10;
constexpr uint32_t MessageUpdIcon   = 11;


namespace xziar::gui::detail
{
using namespace std::string_view_literals;
using xziar::gui::event::CommonKeys;
using xziar::img::Image;
using xziar::img::ImageView;
using xziar::img::ImgDType;


struct DragInfos
{
    boost::container::small_vector<uint32_t, 8> Types;
    uint32_t Version = 0;
    xcb_window_t SourceWindow = 0;
    xcb_atom_t TargetType = 0;
    int16_t PosX = 0, PosY = 0;
    void Clear() noexcept
    {
        Types.clear();
        Version = 0;
        SourceWindow = 0;
        TargetType = 0;
        PosX = PosY = 0;
    }
};

struct IconHolder : public OpaqueResource
{
    explicit IconHolder(common::span<const uint32_t> pixels, uint32_t w, uint32_t h) noexcept : 
        OpaqueResource(&TheDisposer, pixels.size() + 2, reinterpret_cast<uintptr_t>(new uint32_t[pixels.size() + 2])) 
    {
        const auto data = Data();
        data[0] = w, data[1] = h;
        memcpy_s(&data[2], data.size_bytes() - 4, pixels.data(), pixels.size() * sizeof(uint32_t));
    }
    common::span<uint32_t> Data() noexcept { return { reinterpret_cast<uint32_t*>(Cookie[1]), static_cast<size_t>(Cookie[0]) }; }
    common::span<const uint32_t> Data() const noexcept { return { reinterpret_cast<const uint32_t*>(Cookie[1]), static_cast<size_t>(Cookie[0]) }; }
    static void TheDisposer(const OpaqueResource& res) noexcept
    {
        const auto& holder = static_cast<const IconHolder&>(res);
        const auto data = holder.Data();
        ::delete[] data.data();
    }
};
struct RenderImgHolder : public OpaqueResource
{
    explicit RenderImgHolder(const ImageView& img) noexcept : OpaqueResource(&TheDisposer, reinterpret_cast<uintptr_t>(new ImageView(img))) {}
    const ImageView& Image() const noexcept { return *reinterpret_cast<const ImageView*>(Cookie[0]); }
    static void TheDisposer(const OpaqueResource& res) noexcept
    {
        const auto& holder = static_cast<const RenderImgHolder&>(res);
        delete &holder.Image();
    }
};

class WindowManagerXCB final : public XCBBackend, public WindowManager
{
public:
    static constexpr std::string_view BackendName = "XCB"sv;
private:
    class WdHost final : public XCBBackend::XCBWdHost
    {
    public:
        WindowManagerXCB& GetManager() noexcept { return static_cast<WindowManagerXCB&>(WindowHost_::GetManager()); }
        xcb_window_t Handle = 0;
        xcb_gcontext_t GContext = 0;
        xcb_pixmap_t BackImage = 0;
        CacheRect<uint16_t> BackBuf;
        DragInfos DragInfo;
        bool NeedBackground = true;
        WdHost(WindowManagerXCB& manager, const XCBCreateInfo& info) noexcept :
            XCBWdHost(manager, info) { }
        ~WdHost() final {}
        uint32_t GetWindow() const noexcept final { return Handle; }
        void OnDisplay() noexcept final
        {
            auto& manager = GetManager();
            const auto [ sizeChanged, needInit ] = BackBuf.Update(*this);
            if (sizeChanged)
            {
                if (!needInit) manager.Free(BackImage);
                manager.GeneralHandleError(xcb_create_pixmap(manager.Connection, manager.Screen->root_depth, BackImage, Handle, BackBuf.Width, BackBuf.Height));
            }
            bool needDraw = false;
            {
                BgLock lock(this);
                const bool bgChanged = lock;
                const auto& holder = static_cast<RenderImgHolder&>(*GetWindowResource(this, WdAttrIndex::Background));
                if (bgChanged || sizeChanged)
                {
                    common::SimpleTimer timer;
                    timer.Start();
                    xcb_rectangle_t rect{ .x = 0, .y = 0, .width = BackBuf.Width, .height = BackBuf.Height };
                    manager.GeneralHandleError(xcb_poly_fill_rectangle(manager.Connection, BackImage, GContext, 1, &rect));
                    if (holder)
                    {
                        const auto& img = holder.Image();
                        const auto [tw, th] = BackBuf.ResizeWithin(img.GetWidth(), img.GetHeight());
                        const auto newimg = img.ResizeTo(tw, th, true, true);
                        manager.PutImageInPixmap(*this, BackImage, newimg, BackBuf.Width, BackBuf.Height);
                        timer.Stop();
                        Manager.Logger.Verbose(u"Window [{:x}] rebuild backbuffer in [{} ms].\n", Handle, timer.ElapseMs());
                    }
                }
                needDraw = NeedBackground || bgChanged || (sizeChanged && holder);
            }
            if (needDraw)
            {
                xcb_copy_area(manager.Connection, BackImage, Handle, GContext, 0, 0, 0, 0, BackBuf.Width, BackBuf.Height);
                xcb_flush(manager.Connection);
            }
            WindowHost_::OnDisplay();
        }
    };

    xcb_connection_t* Connection = nullptr;
    Display* TheDisplay = nullptr;
    int32_t DefScreen = 0;
    ImgDType PixmapDType;
    size_t ReqMaxSize = 0;
    xkb_context* XKBContext = nullptr;
    xkb_keymap* XKBKeymap = nullptr;
    xkb_state* XKBState = nullptr;
    xcb_screen_t* Screen = nullptr;
    common::container::FrozenDenseSet<std::string_view> Extensions;
    std::map<std::string, std::variant<uint32_t, std::string, std::array<uint16_t, 4>>, std::less<>> XSettings;
    uint32_t BGColor = 0;
    xcb_window_t ControlWindow = 0;
    xcb_atom_t MsgAtom = 0;
    xcb_atom_t WMProtocolAtom = 0;
    xcb_atom_t WMStateAtom = 0;
    xcb_atom_t WMStateHiddenAtom = 0;
    xcb_atom_t WMIconAtom = 0;
    xcb_atom_t CloseAtom = 0;
    xcb_atom_t XSServerAtom = 0;
    xcb_atom_t XSSettingAtom = 0;
    xcb_atom_t XdndProxyAtom = 0;
    xcb_atom_t XdndAwareAtom = 0;
    xcb_atom_t XdndTypeListAtom = 0;
    xcb_atom_t XdndEnterAtom = 0;
    xcb_atom_t XdndLeaveAtom = 0;
    xcb_atom_t XdndPositionAtom = 0;
    xcb_atom_t XdndDropAtom = 0;
    xcb_atom_t XdndStatusAtom = 0;
    xcb_atom_t XdndFinishedAtom = 0;
    xcb_atom_t XdndSelectionAtom = 0;
    xcb_atom_t XdndActionCopyAtom = 0;
    xcb_atom_t XdndActionMoveAtom = 0;
    xcb_atom_t XdndActionLinkAtom = 0;
    xcb_atom_t UrlListAtom = 0;
    xcb_atom_t PrimaryAtom = 0;
    xkb_mod_index_t CapsLockIndex = 0;
    uint8_t XKBEventID = 0;
    uint8_t XKBErrorID = 0;
    bool IsCapsLock = false;

    std::string_view Name() const noexcept final { return BackendName; }
    bool CheckFeature(std::string_view feat) const noexcept final
    {
        constexpr std::string_view Features[] =
        {
            "OpenGL"sv, "OpenGLES"sv, "Vulkan"sv, "NewThread"sv,
        };
        return std::find(std::begin(Features), std::end(Features), feat) != std::end(Features);
    }
    void* GetDisplay() const noexcept final { return TheDisplay; }
    void* GetConnection() const noexcept final { return Connection; }
    int32_t GetDefaultScreen() const noexcept final { return DefScreen; }

    bool GeneralHandleError(xcb_generic_error_t* err) const noexcept
    {
        if (err)
        {
            Logger.Error(u"Error: [{}] [{},{}]\n", err->error_code, err->major_code, err->minor_code);
            free(err);
            return false;
        }
        return true;
    }
    forceinline bool GeneralHandleError(xcb_void_cookie_t cookie) const noexcept
    {
        return GeneralHandleError(xcb_request_check(Connection, cookie));
    }
    std::string QueryAtomName(xcb_atom_t atom) noexcept
    {
        xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(Connection, atom);
        xcb_generic_error_t *err = nullptr;
        xcb_get_atom_name_reply_t* reply = xcb_get_atom_name_reply(Connection, cookie, &err);
        GeneralHandleError(err);
        if (reply)
        {
            std::string ret(xcb_get_atom_name_name(reply), xcb_get_atom_name_name_length(reply));
            free(reply);
            return ret;
        }
        return {};
    }
    std::optional<boost::container::small_vector<std::byte, 48>> QueryProperty(xcb_window_t window, xcb_atom_t prop, xcb_atom_t type)
    {
        uint32_t offset = 0, want = 1024;
        boost::container::small_vector<std::byte, 48> data;
        do
        {
            Ensures(offset % 4 == 0);
            xcb_get_property_cookie_t cookie = xcb_get_property(Connection, 0, window, prop, type, offset / 4, (want + 3) / 4);
            xcb_generic_error_t *err = nullptr;
            xcb_get_property_reply_t* reply = xcb_get_property_reply(Connection, cookie, &err);
            GeneralHandleError(err);
            if (!reply)
                return {};
            if (reply->type == XCB_NONE)
            {
                free(reply);
                return {};
            }
            const auto ptr = xcb_get_property_value(reply);
            const auto readLen = xcb_get_property_value_length(reply);
            want = reply->bytes_after;
            if (readLen)
            {
                const auto copyLen = (want > 0 && readLen % 4 != 0) ? readLen / 4 * 4 : readLen; // u32 unit
                data.resize(offset + copyLen);
                memcpy_s(&data[offset], copyLen, ptr, copyLen);
                offset += copyLen;
            }
            free(reply);
        } while (want);
        data.resize(offset);
        return data;
    }
    void UpdateXKBState()
    {
        IsCapsLock = xkb_state_mod_index_is_active(XKBState, CapsLockIndex, XKB_STATE_MODS_EFFECTIVE) > 0;
    }
    template<typename T, size_t M, size_t N, size_t... I>
    forceinline static void CopyData(T(&dst)[M], const T(&src)[N], std::index_sequence<I...>) noexcept
    {
        (..., void(dst[I] = src[I]));
    }
    template<typename T, size_t N>
    forceinline void SendClientMessage(xcb_window_t window, xcb_atom_t type, const T(&data)[N]) const noexcept
    {
        static_assert(std::is_same_v<T, uint32_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint8_t>, "need u32/u16/u8 data");
        xcb_client_message_event_t evt = {};
        evt.response_type = XCB_CLIENT_MESSAGE;
        evt.type = type;
        evt.window = window;
        if constexpr (std::is_same_v<T, uint32_t>)
        {
            static_assert(N <= 5, "at most 5 u32");
            evt.format = 32;
            CopyData(evt.data.data32, data, std::make_index_sequence<N>{});
        }
        else if constexpr (std::is_same_v<T, uint16_t>)
        {
            static_assert(N <= 10, "at most 10 u16");
            evt.format = 16;
            CopyData(evt.data.data16, data, std::make_index_sequence<N>{});
        }
        else
        {
            static_assert(N <= 10, "at most 20 u8");
            evt.format = 8;
            CopyData(evt.data.data8, data, std::make_index_sequence<N>{});
        }
        xcb_send_event(Connection, False, window, XCB_EVENT_MASK_NO_EVENT, reinterpret_cast<const char*>(&evt));
    }
    void SendControlRequest(const uint32_t request, const uint32_t data0 = 0, const uint32_t data1 = 0, const uint32_t data2 = 0, const uint32_t data3 = 0) const noexcept
    {
        uint32_t data[5] = { request, data0, data1, data2, data3 };
        SendClientMessage(ControlWindow, MsgAtom, data);
        xcb_flush(Connection);
    }
    void SetTitle(xcb_window_t window, std::u16string_view name)
    {
        const auto title = common::str::to_u8string(name, common::str::Encoding::UTF16LE);
        xcb_change_property(Connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
            gsl::narrow_cast<uint32_t>(title.size()), title.c_str());
    }
    void SetIcon(xcb_window_t window, common::span<const uint32_t> data)
    {
        xcb_change_property(Connection, XCB_PROP_MODE_REPLACE, window, WMIconAtom, XCB_ATOM_CARDINAL, 32,
            gsl::narrow_cast<uint32_t>(data.size()), data.data());
    }
    void LoadXSettings()
    {
        if (!XSServerAtom) return;
        const auto ownerReply = xcb_get_selection_owner_reply(Connection, xcb_get_selection_owner(Connection, XSServerAtom), nullptr);
        if (!ownerReply) return;
        const auto xsWindow = ownerReply->owner;
        free(ownerReply);
        if (!xsWindow) return;
        const auto settings = QueryProperty(xsWindow, XSSettingAtom, XCB_ATOM_ANY);
        if (!settings.has_value()) return;
        common::io::MemoryInputStream stream(common::span<const std::byte>{settings->data(), settings->size()});
        struct Header
        {
            uint8_t ByteOrder = XCB_IMAGE_ORDER_LSB_FIRST;
            uint8_t Padding[3] = { 0 };
            uint8_t Serial[4] = { 0 };
            int32_t Count = 0;
        };
        Header header;
        if (!stream.Read(header) || header.ByteOrder != XCB_IMAGE_ORDER_LSB_FIRST) return;
        bool isSuccess = true;
        while (header.Count-- && isSuccess)
        {
            static constexpr uint8_t TypeInteger = 0;
            static constexpr uint8_t TypeString  = 1;
            static constexpr uint8_t TypeColor   = 2;
            struct SettingHeader
            {
                uint8_t Type = 0;
                uint8_t Padding = 0;
                uint16_t NameSize = 0;
            };
            SettingHeader sHead;
            isSuccess = stream.Read(sHead);
            std::string name;
            const uint32_t nameUnit = (sHead.NameSize + 3) / 4 * 4; // u32 align
            isSuccess = isSuccess && nameUnit > 0 && sHead.Type <= TypeColor // valid setting
                && stream.ReadTo(name, nameUnit) == nameUnit // name with padding
                && stream.Skip(4); // serial
            if (isSuccess)
            {
                name.resize(sHead.NameSize);
                switch (sHead.Type)
                {
                case TypeInteger:
                    if (uint32_t val = 0; (isSuccess = stream.Read(val)))
                        XSettings.insert_or_assign(std::move(name), val);
                    break;
                case TypeString:
                {
                    uint32_t strSize = 0;
                    isSuccess = stream.Read(strSize);
                    std::string str;
                    const auto strUnit = (strSize + 3) / 4 * 4; // u32 align
                    isSuccess = isSuccess && strUnit > 0 && stream.ReadTo(str, strUnit) == strUnit;
                    if (isSuccess)
                    {
                        str.resize(strSize);
                        XSettings.insert_or_assign(std::move(name), std::move(str));
                    }
                } break;
                case TypeColor:
                    if (std::array<uint16_t, 4> clr = {}; (isSuccess = stream.Read(clr)))
                        XSettings.insert_or_assign(std::move(name), clr);
                    break;
                default: CM_UNREACHABLE();
                }
            }
        }
    }
public:
    WindowManagerXCB() : XCBBackend(true) { }
    ~WindowManagerXCB() override { }

    void OnInitialize(const void* info_) final
    {
        using common::BaseException;

        const auto info = reinterpret_cast<const XCBInitInfo*>(info_);
        const auto dispName = (info && !info->DisplayName.empty()) ? info->DisplayName.c_str() : nullptr;

        if (info && info->UsePureXCB)
        {
            Connection = xcb_connect(dispName, &DefScreen);
            if (!Connection)
            {
                COMMON_THROW(BaseException, u"Can't connect xcb");
            }
        }
        else
        {
            // XInitThreads();
            TheDisplay = XOpenDisplay(dispName);
            /* open display */
            if (!TheDisplay)
                COMMON_THROW(BaseException, u"Failed to open display");

            // Acquire event queue ownership
            XSetEventQueueOwner(TheDisplay, XCBOwnsEventQueue);

            DefScreen = DefaultScreen(TheDisplay);
            // Get the XCB connection from the display
            Connection = XGetXCBConnection(TheDisplay);
            if (!Connection)
            {
                XCloseDisplay(TheDisplay);
                COMMON_THROW(BaseException, u"Can't get xcb connection from display");
            }
        }
        // list extensions
        {
            xcb_generic_error_t *err = nullptr;
            const auto ck = xcb_list_extensions(Connection);
            const auto reply = xcb_list_extensions_reply(Connection, ck, &err);
            GeneralHandleError(err);
            if (reply)
            {
                std::vector<std::string_view> exts;
                for (auto extIter = xcb_list_extensions_names_iterator(reply); extIter.rem; xcb_str_next(&extIter))
                {
                    const auto str = xcb_str_name(extIter.data);
                    const auto len = xcb_str_name_length(extIter.data);
                    exts.emplace_back(str, len);
                }
                Extensions = exts;
                std::string exttxts("xcb Extensions:\n");
                for (const auto& ext : Extensions)
                    exttxts.append(ext).push_back('\n');
                Logger.Verbose(u"{}", exttxts);
                free(reply);
            }
        }
        // no need to explicit enable big request
#if 0
        if (Extensions.Has("BIG-REQUESTS"))
        {
            xcb_generic_error_t *err = nullptr;
            const auto ck = xcb_big_requests_enable(Connection);
            const auto reply = xcb_big_requests_enable_reply(Connection, ck, &err);
            GeneralHandleError(err);
            if (reply)
            {
                ReqMaxSize = reply->maximum_request_length * sizeof(uint32_t);
                free(reply);
            }
        }
#endif
        ReqMaxSize = xcb_get_maximum_request_length(Connection) * sizeof(uint32_t);

        auto setup = xcb_get_setup(Connection);
        Ensures(setup->image_byte_order == XCB_IMAGE_ORDER_LSB_FIRST);
        std::string_view vendor;
        if (const auto vlen = xcb_setup_vendor_length(setup); vlen > 0)
            vendor = { xcb_setup_vendor(setup), static_cast<size_t>(vlen) };
        Logger.Debug(u"xcb initialized as [{}.{}], vendor[{}], max-req-size[{}]\n"sv, setup->protocol_major_version, setup->protocol_minor_version, vendor, ReqMaxSize);

        // Setup xkb
        uint16_t xkbVersion[2] = {0};
        const auto xkbRet = xkb_x11_setup_xkb_extension(Connection, XCB_XKB_MAJOR_VERSION, XCB_XKB_MINOR_VERSION, XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, &xkbVersion[0], &xkbVersion[1], &XKBEventID, &XKBErrorID);
        if (xkbRet == 0)
            COMMON_THROW(BaseException, u"Failed to setup xkb extension");
        Logger.Debug(u"xkb initialized as [{}.{}] with event id[{}][{}].\n"sv, xkbVersion[0], xkbVersion[1], XKBEventID, XKBErrorID);
        const auto kbId = xkb_x11_get_core_keyboard_device_id(Connection);
        constexpr auto evtMask = static_cast<uint16_t>(XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY | XCB_XKB_EVENT_TYPE_MAP_NOTIFY | XCB_XKB_EVENT_TYPE_STATE_NOTIFY);
        xcb_xkb_select_events(Connection, kbId,
            evtMask, 0, evtMask,
            UINT16_MAX, UINT16_MAX,
            nullptr);

        XKBContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        XKBKeymap = xkb_x11_keymap_new_from_device(XKBContext, Connection, kbId, XKB_KEYMAP_COMPILE_NO_FLAGS);
        XKBState = xkb_x11_state_new_from_device(XKBKeymap, Connection, kbId);
        CapsLockIndex = xkb_keymap_mod_get_index(XKBKeymap, XKB_MOD_NAME_CAPS);
        UpdateXKBState();

        // Find XCB screen
        {
            auto screenIter = xcb_setup_roots_iterator(setup);
            for (auto screenCnt = DefScreen; screenIter.rem && screenCnt--;)
                xcb_screen_next(&screenIter);
            Screen = screenIter.data;
        }
        const xcb_format_t* pixmapFormat = nullptr;
        {
            const auto ptrFmt = xcb_setup_pixmap_formats(setup);
            const auto count = xcb_setup_pixmap_formats_length(setup);
            for (int i = 0; i < count; ++i)
            {
                if (ptrFmt[i].depth == Screen->root_depth)
                {
                    Logger.Verbose(u"match pixmap format: [{}]bpp, pad[{}]\n"sv, ptrFmt[i].bits_per_pixel, ptrFmt[i].scanline_pad);
                    if (!pixmapFormat) pixmapFormat = &ptrFmt[i];
                }
            }
        }
        {
            struct VisaulDType
            {
                ImgDType Type;
                uint8_t TotalBits;
                uint8_t BitPerComp;
                uint32_t RedMask;
                uint32_t GreenMask;
                uint32_t BlueMask;
                constexpr bool operator==(const xcb_visualtype_t& v) const noexcept
                {
                    return v.bits_per_rgb_value == BitPerComp && v.red_mask == RedMask && v.green_mask == GreenMask && v.blue_mask == BlueMask;
                }
            };
            static constexpr VisaulDType VDMap[] = 
            {
                { xziar::img::ImageDataType::RGBA,   32,  8, 0x000000ffu, 0x0000ff00u, 0x00ff0000u },
                { xziar::img::ImageDataType::BGRA,   32,  8, 0x00ff0000u, 0x0000ff00u, 0x000000ffu },
                { xziar::img::ImageDataType::RGB,    24,  8, 0x000000ffu, 0x0000ff00u, 0x00ff0000u },
                { xziar::img::ImageDataType::BGR,    24,  8, 0x00ff0000u, 0x0000ff00u, 0x000000ffu },
                { xziar::img::ImageDataType::GA,     16,  8, 0x000000ffu, 0x00000000u, 0x00000000u },
                { xziar::img::ImageDataType::GRAY,    8,  8, 0x000000ffu, 0x00000000u, 0x00000000u },
                { xziar::img::ImageDataType::RGBA16, 64, 16, 0x000000ffu, 0x0000ff00u, 0x00ff0000u },
                { xziar::img::ImageDataType::BGRA16, 64, 16, 0x00ff0000u, 0x0000ff00u, 0x000000ffu },
                { xziar::img::ImageDataType::RGB16,  48, 16, 0x000000ffu, 0x0000ff00u, 0x00ff0000u },
                { xziar::img::ImageDataType::BGR16,  48, 16, 0x00ff0000u, 0x0000ff00u, 0x000000ffu },
                { xziar::img::ImageDataType::GA16,   32, 16, 0x000000ffu, 0x00000000u, 0x00000000u },
                { xziar::img::ImageDataType::GRAY16, 16, 16, 0x000000ffu, 0x00000000u, 0x00000000u },
            };
            const auto targetBPP = pixmapFormat ? pixmapFormat->bits_per_pixel : Screen->root_depth;
            const xcb_visualtype_t* matchVisual = nullptr;
            for (auto depthIter = xcb_screen_allowed_depths_iterator(Screen); depthIter.rem; xcb_depth_next(&depthIter))
            {
                const auto visuals = xcb_depth_visuals(depthIter.data);
                const auto count = xcb_depth_visuals_length(depthIter.data);
                for (int i = 0; i < count; ++i)
                {
                    if (const auto& visual = visuals[i]; visual.visual_id == Screen->root_visual)
                    {
                        const auto dtype = common::linq::FromIterable(VDMap)
                            .Where([&](const VisaulDType& vd){ return vd.TotalBits == targetBPP && vd == visual; })
                            .TryGetFirst();
                        Logger.Verbose(u"match visual: [{}]bit each, R[{:08X}] G[{:08X}] B[{:08X}]\n"sv, visual.bits_per_rgb_value, visual.red_mask, visual.green_mask, visual.blue_mask);
                        if (!PixmapDType && dtype)
                        {
                            matchVisual = &visual;
                            PixmapDType = dtype->Type;
                        }
                        else if (!matchVisual)
                            matchVisual = &visual;
                    }
                }
            }
        }
        Logger.Debug(u"screen: [{}x{}] depth[{}] visual[{}]({})\n"sv, Screen->width_in_pixels, Screen->height_in_pixels, Screen->root_depth, Screen->root_visual, PixmapDType.ToString());

        // control window 
        ControlWindow = xcb_generate_id(Connection);
        const uint32_t valuemask = XCB_CW_EVENT_MASK;
        const uint32_t eventmask = XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
        const uint32_t valuelist[] = { eventmask };

        const auto cookie0 = xcb_create_window(
            Connection,
            XCB_COPY_FROM_PARENT,
            ControlWindow,
            Screen->root,
            0, 0,
            1, 1,
            0,
            XCB_WINDOW_CLASS_INPUT_ONLY,
            Screen->root_visual,
            valuemask,
            valuelist
        );
        GeneralHandleError(cookie0);

        // prepare atoms
        {
            const auto xsName = std::string("0_XSETTINGS_S") + std::to_string(DefScreen);
            const std::pair<xcb_atom_t*, std::string_view> atomList[] = 
            {
                { &MsgAtom,             "1XZIAR_GUI_MSG"sv       },
                { &WMProtocolAtom,      "0WM_PROTOCOLS"sv        },
                { &WMStateAtom,         "0_NET_WM_STATE"sv       },
                { &WMStateHiddenAtom,   "0_NET_WM_STATE_HIDDEN"sv},
                { &WMIconAtom,          "0_NET_WM_ICON"sv        },
                { &CloseAtom,           "0WM_DELETE_WINDOW"sv    },
                { &XSServerAtom,        xsName                   },
                { &XSSettingAtom,       "0_XSETTINGS_SETTINGS"sv },
                { &XdndProxyAtom,       "0XdndProxy"sv           },
                { &XdndAwareAtom,       "0XdndAware"sv           },
                { &XdndTypeListAtom,    "0XdndTypeList"sv        },
                { &XdndEnterAtom,       "0XdndEnter"sv           },
                { &XdndLeaveAtom,       "0XdndLeave"sv           },
                { &XdndPositionAtom,    "0XdndPosition"sv        },
                { &XdndDropAtom,        "0XdndDrop"sv            },
                { &XdndStatusAtom,      "0XdndStatus"sv          },
                { &XdndFinishedAtom,    "0XdndFinished"sv        },
                { &XdndSelectionAtom,   "0XdndSelection"sv       },
                { &XdndActionCopyAtom,  "0XdndActionCopy"sv      },
                { &XdndActionMoveAtom,  "0XdndActionMove"sv      },
                { &XdndActionLinkAtom,  "0XdndActionLink"sv      },
                { &UrlListAtom,         "0text/uri-list"sv       },
                { &PrimaryAtom,         "0PRIMARY"sv             },
            };
            std::vector<xcb_intern_atom_cookie_t> atomCookies;
            atomCookies.reserve(sizeof(atomList) / sizeof(atomList[0]));
            for (const auto& item : atomList)
            {
                const uint8_t onlyIfExists = item.second[0] == '1' ? 0 : 1;
                const auto nameSize = gsl::narrow_cast<uint16_t>(item.second.size() - 1);
                const auto namePtr  = item.second.data() + 1;
                atomCookies.emplace_back(xcb_intern_atom_unchecked(Connection, onlyIfExists, nameSize, namePtr));
            }
            size_t atomIdx = 0;
            for (const auto& cookie : atomCookies)
            {
                const auto reply = xcb_intern_atom_reply(Connection, cookie, nullptr);
                if (reply)
                {
                    *atomList[atomIdx++].first = reply->atom;
                    free(reply);
                }
            }
        }
        // read xsettings
        LoadXSettings();
        {
            std::string str;
            common::str::Formatter<char> fmter;
            for (const auto& [k, v] : XSettings)
            {
                switch (v.index())
                {
                case 0: fmter.FormatToStatic(str, FmtString("--[{}] = [{}]\n"), k, std::get<0>(v)); break;
                case 1: fmter.FormatToStatic(str, FmtString("--[{}] = [{}]\n"), k, std::get<1>(v)); break;
                case 2: fmter.FormatToStatic(str, FmtString("--[{}] = [{}]\n"), k, common::span<const uint16_t>(std::get<2>(v))); break;
                }
            }
            if (!str.empty())
                Logger.Verbose(u"XSettings:\n{}", str);
            if (const auto it = XSettings.find("Net/ThemeName"); it != XSettings.end() && it->second.index() == 1)
                BGColor = common::str::ToLowerEng(std::get<1>(it->second)).ends_with("dark") ? Screen->black_pixel : Screen->white_pixel;
            else
                BGColor = Screen->black_pixel;
        }
#if 0
        {
            const auto rmdb = xcb_xrm_database_from_default(Connection);

            if (rmdb)
            {
                const char* names[] = 
                {
                    "background",
                    "foreground",
                    "color0",  // Black
                    "color1",  // Red
                    "color2",  // Green
                    "kde.palette.activeBackground",
                    "kde.palette.activeForeground"
                };
                for (const auto name : names)
                {
                    char* val = nullptr;
                    if (0 == xcb_xrm_resource_get_string(rmdb, name, nullptr, &val) && val)
                    {
                        Logger.Verbose(u"xrm: [{}]: [{}]\n", name, val);
                    }
                }
                xcb_xrm_database_free(rmdb);
            }
        }
#endif

        xcb_flush(Connection);
        XCBBackend::OnInitialize(info_);
    }
    void OnDeInitialize() noexcept final
    {
        xcb_destroy_window(Connection, ControlWindow);

        xkb_state_unref(XKBState);
        xkb_keymap_unref(XKBKeymap);
        xkb_context_unref(XKBContext);

        xcb_disconnect(Connection);
        XCBBackend::OnDeInitialize();
    }
    bool RequestStop() noexcept final
    {
        SendControlRequest(MessageStop, 0, 0);
        return true;
    }

    bool HandleControlMessage(const uint32_t(&data)[5])
    {
        const uintptr_t ptr = static_cast<uintptr_t>(((uint64_t)(data[2]) << 32) + data[1]);
        switch (data[0])
        {
        case MessageCreate:
        {
            auto& payload = *reinterpret_cast<CreatePayload*>(ptr);
            CreateNewWindow_(payload);
        } break;
        case MessageTask:
        {
            HandleTask();
        } break;
        case MessageClose:
        {
            const auto host = static_cast<WdHost*>(reinterpret_cast<WindowHost_*>(ptr));
            const auto cookie = xcb_destroy_window(Connection, host->Handle);
            GeneralHandleError(cookie);
            host->Stop();
        } break;
        case MessageDpi:
        {
            const auto host = static_cast<WdHost*>(reinterpret_cast<WindowHost_*>(ptr));
            //const auto screen = screen_of_display(Connection, DefScreen);
            const auto wmm = Screen->width_in_millimeters, hmm = Screen->height_in_millimeters;
            const auto wpix = Screen->width_in_pixels, hpix = Screen->height_in_pixels;
            const auto dpix = 25.4f * wpix / wmm, dpiy = 25.4f * hpix / hmm;
            host->OnDPIChange(dpix, dpiy);
        } break;
        case MessageUpdTitle:
        {
            const auto host = static_cast<WdHost*>(reinterpret_cast<WindowHost_*>(ptr));
            TitleLock lock(host);
            SetTitle(host->Handle, host->Title);
        } break;
        case MessageUpdIcon:
        {
            const auto host = static_cast<WdHost*>(reinterpret_cast<WindowHost_*>(ptr));
            const auto ptrIcon = GetWindowResource(host, WdAttrIndex::Icon);
            if (ptrIcon)
            {
                IconLock lock(host);
                const auto& holder = static_cast<IconHolder&>(*ptrIcon);
                SetIcon(host->Handle, holder.Data());
            }
        } break;
        case MessageStop:
            return false;
        }
        return true;
    }
    void HandleClientMessage(WindowHost_* host_, xcb_window_t window, const xcb_atom_t atom, const xcb_client_message_data_t& data)
    {
        const auto host = static_cast<WdHost*>(host_);
        if (atom == WMProtocolAtom)
        {
            if (data.data32[0] == CloseAtom)
            {
                if (host->OnClose())
                    CloseWindow(host);
                return;
            }
        }
        else if (atom == WMStateAtom)
        {
            if (data.data32[0] == WMStateHiddenAtom)
            {
                host->OnResize(0, 0);
                return;
            }
        }
        else if (atom == XdndEnterAtom)
        {
            auto& dragInfo = host->DragInfo;
            dragInfo.Clear();
            dragInfo.SourceWindow = data.data32[0];
            dragInfo.Version      = data.data32[1] >> 24;
            if (data.data32[1] & 1) // > 3 types
            {
                const auto types = QueryProperty(dragInfo.SourceWindow, XdndTypeListAtom, XCB_ATOM_ATOM);
                if (types.has_value())
                {
                    const auto count = types->size() / 4;
                    const auto ptr = reinterpret_cast<const uint32_t*>(types->data());
                    dragInfo.Types.assign(ptr, ptr + count);
                }
            }
            else
                dragInfo.Types.assign(&data.data32[2], &data.data32[5]);
            Logger.Info(u"DragEnter from [{}] with {} types:\n", uint32_t(dragInfo.SourceWindow), dragInfo.Types.size());
            for (const auto& type : dragInfo.Types)
            {
                if (type == UrlListAtom)
                    dragInfo.TargetType = type;
                const auto name = QueryAtomName(type);
                Logger.Verbose(u"--[{:6}]: [{}]\n", type, name);
            }
        }
        else if (atom == XdndLeaveAtom)
        {
            auto& dragInfo = host->DragInfo;
            dragInfo.Clear();
            const xcb_window_t source = data.data32[0];
            Logger.Info(u"DragLeave from [{}]\n", uint32_t(source));
        }
        else if (atom == XdndPositionAtom)
        {
            auto& dragInfo = host->DragInfo;
            if (dragInfo.SourceWindow == data.data32[0])
            {
                dragInfo.PosX = static_cast<int16_t>(data.data32[2] >> 16);
                dragInfo.PosY = static_cast<int16_t>(data.data32[2] & 0xffff);
                uint32_t sendData[5] = { window, 0, 0, 0, 0 };
                if (dragInfo.TargetType)
                {
                    sendData[1] = 1;
                    sendData[4] = XdndActionCopyAtom;
                }
                SendClientMessage(dragInfo.SourceWindow, XdndStatusAtom, sendData);
                xcb_flush(Connection);
            }
            else if (dragInfo.SourceWindow)
                dragInfo.Clear();
        }
        else if (atom == XdndDropAtom)
        {
            auto& dragInfo = host->DragInfo;
            if (dragInfo.SourceWindow == data.data32[0])
            {
                if (dragInfo.TargetType)
                {
                    Logger.Info(u"DragDrop from [{}]\n", uint32_t(dragInfo.SourceWindow));
                    xcb_convert_selection(Connection, window, XdndSelectionAtom, dragInfo.TargetType, PrimaryAtom, dragInfo.Version >= 1 ? data.data32[2] : XCB_CURRENT_TIME);
                    xcb_flush(Connection);
                    return;
                }
            }
            uint32_t sendData[3] = { window, 0, 0 }; // send reject
            SendClientMessage(dragInfo.SourceWindow, XdndFinishedAtom, sendData);
            dragInfo.Clear();
        }
    }
    void HandleXKBEvent(const xcb_generic_event_t* evt)
    {
        switch (const auto xkbType = evt->pad0; xkbType)
        {
        case XCB_XKB_STATE_NOTIFY:
        {
            const auto& msg = *reinterpret_cast<const xcb_xkb_state_notify_event_t*>(evt);
            xkb_state_update_mask(XKBState,
                msg.baseMods,
                msg.latchedMods,
                msg.lockedMods,
                msg.baseGroup,
                msg.latchedGroup,
                msg.lockedGroup);
            UpdateXKBState();
        } break;
        default:
            Logger.Verbose(u"Recieve xkb message [{}]\n", (uint32_t)xkbType);
        }
    }

    event::CombinedKey ProcessKey(xcb_keycode_t keycode) noexcept;

    void InitializeWindow(xcb_window_t window)
    {
        if (const auto host_ = GetWindow(window); host_)
        {
            const auto host = static_cast<WdHost*>(host_);
            host->Handle = window;
            host->GContext = xcb_generate_id(Connection);
            host->BackImage = xcb_generate_id(Connection);
            uint32_t values[] = { BGColor, 0 };
            xcb_create_gc(Connection, host->GContext, host->Handle, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, values);
            host->Initialize();
        }
    }
    void OnMessageLoop() final
    {
        xcb_generic_event_t* evt;
        bool shouldContinue = true;
        while (shouldContinue && (evt = xcb_wait_for_event(Connection)))
        {
            // Logger.Verbose(u"Recieve message [{}]\n", (uint32_t)evt->response_type);
            switch (const auto evtType = evt->response_type & 0x7f; evtType)
            {
            case XCB_CREATE_NOTIFY:
                InitializeWindow(reinterpret_cast<xcb_create_notify_event_t*>(evt)->window);
                break;
            case XCB_REPARENT_NOTIFY:
                InitializeWindow(reinterpret_cast<xcb_reparent_notify_event_t*>(evt)->window);
                break;
            case XCB_MAP_NOTIFY:
                InitializeWindow(reinterpret_cast<xcb_map_notify_event_t*>(evt)->window);
                break;
            case XCB_MOTION_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_motion_notify_event_t*>(evt);
                if (const auto host = GetWindow(msg.event); host)
                {
                    event::Position pos(msg.event_x, msg.event_y);
                    host->OnMouseMove(pos);
                }
            } break;
            case XCB_ENTER_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_leave_notify_event_t*>(evt);
                if (const auto host = GetWindow(msg.event); host)
                {
                    event::Position pos(msg.event_x, msg.event_y);
                    host->OnMouseEnter(pos);
                }
            } break;
            case XCB_LEAVE_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_leave_notify_event_t*>(evt);
                if (const auto host = GetWindow(msg.event); host)
                {
                    host->OnMouseLeave();
                }
            } break;
            case XCB_BUTTON_PRESS:
            case XCB_BUTTON_RELEASE:
            {
                const auto& msg = *reinterpret_cast<xcb_button_press_event_t*>(evt);
                if (const auto host = GetWindow(msg.event); host)
                {
                    if (msg.detail >= 4 && msg.detail <= 7) // scroll
                    {
                        float dh = 0, dv = 0;
                        (msg.detail & 0b10 ? dh : dv) = msg.detail & 0b01 ? -0.5f : 0.5f;
                        event::Position pos(msg.event_x, msg.event_y);
                        host->OnMouseScroll(pos, dh, dv);
                    }
                    else
                    {
                        event::MouseButton btn = event::MouseButton::None;
                        switch (msg.detail)
                        {
                        case XCB_BUTTON_INDEX_1: btn = event::MouseButton::Left;   break;
                        case XCB_BUTTON_INDEX_2: btn = event::MouseButton::Middle; break;
                        case XCB_BUTTON_INDEX_3: btn = event::MouseButton::Right;  break;
                        default: break;
                        }
                        const bool isPress = evtType == XCB_BUTTON_PRESS;
                        host->OnMouseButton(btn, isPress);
                    }
                }
            } break;
            case XCB_KEY_PRESS:
            case XCB_KEY_RELEASE:
            {
                const auto& msg = *reinterpret_cast<xcb_key_press_event_t*>(evt);
                if (const auto host = GetWindow(msg.event); host)
                {
                    const auto key = ProcessKey(msg.detail);
                    // Logger.Verbose(u"key: [{}] => [{}]\n", (uint32_t)msg.detail, common::enum_cast(key.Key));
                    if (evtType == XCB_KEY_PRESS) 
                    {
                        // dirty fix for caps state since it's updated in next message
                        if (key.Key == event::CommonKeys::CapsLock)
                            IsCapsLock = !IsCapsLock;
                        host->OnKeyDown(key);
                    }
                    else
                        host->OnKeyUp(key);
                }
            } break;
            case XCB_CONFIGURE_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_configure_notify_event_t*>(evt);
                if (const auto host = GetWindow(msg.window); host)
                {
                    uint16_t width = msg.width;
                    uint16_t height = msg.height;
                    if ((width > 0 && width != host->Width) || (height > 0 && height != host->Height))
                    {
                        host->OnResize(width, height);
                    }
                }
            } break;
            case XCB_EXPOSE:
            {
                const auto& msg = *reinterpret_cast<xcb_expose_event_t*>(evt);
                if (const auto host = GetWindow(msg.window); host)
                {
                    host->Invalidate();
                }
            } break;
            case XCB_NO_EXPOSURE: // simply ignore
            {
            } break;
            case XCB_SELECTION_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_selection_notify_event_t*>(evt);
                Logger.Info(u"Recieve selection[{}]({}) target[{}]({}) property[{}]({})\n",
                    QueryAtomName(msg.selection), uint32_t(msg.selection),
                    QueryAtomName(msg.target),    uint32_t(msg.target),
                    QueryAtomName(msg.property),  uint32_t(msg.property));
                if (const auto host_ = GetWindow(msg.requestor); host_)
                {
                    const auto host = static_cast<WdHost*>(host_);
                    auto& dragInfo = host->DragInfo;
                    if (msg.selection == XdndSelectionAtom && msg.target == dragInfo.TargetType)
                    {
                        const auto ret = QueryProperty(msg.requestor, PrimaryAtom, XCB_ATOM_ANY);

                        Expects(dragInfo.SourceWindow);
                        uint32_t sendData[3] = { host->Handle, 1, XdndActionCopyAtom };
                        SendClientMessage(dragInfo.SourceWindow, XdndFinishedAtom, sendData);
                        xcb_flush(Connection);
                        if (ret)
                        {
                            const auto cookie = xcb_translate_coordinates(Connection, Screen->root, host->Handle, dragInfo.PosX, dragInfo.PosY);
                            
                            common::StringPool<char16_t> fileNamePool;
                            std::vector<common::StringPiece<char16_t>> fileNamePieces;
                            const auto ptr = reinterpret_cast<const char*>(ret->data());
                            for (auto line : common::str::SplitStream(std::string_view(ptr, ret->size()), 
                                [](auto ch){ return ch == '\r' || ch == '\n'; }, false))
                            {
                                if (common::str::IsBeginWith(line, "file://")) // only accept local file
                                {
                                    line.remove_prefix(7);
                                    fileNamePieces.emplace_back(fileNamePool.AllocateString(
                                        common::str::to_u16string(line, common::str::Encoding::URI))); 
                                }
                            }
                            const auto reply = xcb_translate_coordinates_reply(Connection, cookie, nullptr);
                            event::Position pos(reply->dst_x, reply->dst_y);
                            free(reply);
                            host->OnDropFile(pos, std::move(fileNamePool), std::move(fileNamePieces));
                        }
                        dragInfo.Clear();
                    }
                }
            } break;
            case XCB_CLIENT_MESSAGE:
            {
                const auto& msg = *reinterpret_cast<xcb_client_message_event_t*>(evt);
                if (msg.window == ControlWindow)
                {
                    shouldContinue = HandleControlMessage(msg.data.data32);
                }
                else if (const auto host = GetWindow(msg.window); host)
                {
                    HandleClientMessage(host, msg.window, msg.type, msg.data);
                }
            } break;
            case 0: // error
            {
                const auto& err = *reinterpret_cast<xcb_generic_error_t*>(evt);
                Logger.Error(u"Error: [{}] [{},{}]\n", err.error_code, err.major_code, err.minor_code);
            } break;
            default:
            {
                if (evtType == XKBEventID) // handle xkb event
                    HandleXKBEvent(evt);
                else if (evtType == XKBErrorID)
                    Logger.Warning(u"Recieve xkb error [{}]\n", evt->pad0);
                else
                    Logger.Verbose(u"Recieve message [{}]\n", evtType);
            } break;
            }
            free(evt);
        }
    }
    void NotifyTask() noexcept final
    {
        SendControlRequest(MessageTask);
    }

    bool CheckCapsLock() const noexcept final
    {
        return IsCapsLock;
    }

    void CreateNewWindow_(CreatePayload& payload)
    {
        const auto host = static_cast<WdHost*>(payload.Host);

        int visualId = Screen->root_visual;
        if (payload.ExtraData)
        {
            const auto vi_ = (*payload.ExtraData)("visual");
            if (const auto vi = TryGetFinally<int>(vi_); vi)
                visualId = *vi;
            const auto bg_ = (*payload.ExtraData)("background");
            if (const auto bg = TryGetFinally<bool>(bg_); bg)
                host->NeedBackground = *bg;
        }

        // Create XID's for window 
        xcb_window_t window = xcb_generate_id(Connection);
        /* Create window */
        const uint32_t eventmask = XCB_EVENT_MASK_EXPOSURE |
            XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION |
            XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
            XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
        uint32_t valuemask = XCB_CW_EVENT_MASK;
        std::array<uint32_t, 2> valuelist = {0};
        size_t valueIdx = 0;
        //if (host->NeedBackground)
        //{
        //    valuemask |= XCB_CW_BACK_PIXEL;
        //    valuelist[valueIdx++] = BGColor;
        //}
        valuelist[valueIdx++] = eventmask;

        const auto cookie0 = xcb_create_window(
            Connection,
            XCB_COPY_FROM_PARENT,
            window,
            Screen->root,
            0, 0,
            host->Width, host->Height,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            visualId,
            valuemask, valuelist.data()
        );
        GeneralHandleError(cookie0);
        
        payload.Promise.set_value(); // can release now
        RegisterHost(window, host);

        // //Property does not exist, so set it to redirect to me
        // XChangeProperty(disp, root, XdndProxy, XA_WINDOW, 32, PropModeReplace, (unsigned char*)&w, 1);
        // //Set the proxy on me to point to me (as per the spec)
        // XChangeProperty(disp, w, XdndProxy, XA_WINDOW, 32, PropModeReplace, (unsigned char*)&w, 1);

        const auto dummy = QueryProperty(Screen->root, XdndProxyAtom, XCB_ATOM_ANY);
        // set Xdnd
        xcb_change_property(Connection, XCB_PROP_MODE_REPLACE, window, XdndProxyAtom, XCB_ATOM_WINDOW, 32, 1, &window);
        constexpr xcb_atom_t xdndVersion = 5;
        xcb_change_property(Connection, XCB_PROP_MODE_REPLACE, window, XdndAwareAtom, XCB_ATOM_ATOM, 32, 1, &xdndVersion);

        // set close
        xcb_change_property(Connection, XCB_PROP_MODE_APPEND, window, WMProtocolAtom, XCB_ATOM_ATOM, 32, 1, &CloseAtom);
        // set title
        SetTitle(window, host->Title);

        xcb_map_window(Connection, window);
        xcb_flush(Connection);
    }
    void CreateNewWindow(CreatePayload& payload) final
    {
        const uint64_t ptr = reinterpret_cast<uintptr_t>(&payload);
        SendControlRequest(MessageCreate,
            static_cast<uint32_t>(ptr & 0xffffffff),
            static_cast<uint32_t>((ptr >> 32) & 0xffffffff));
        payload.Promise.get_future().get();
    }
    void AfterWindowOpen(WindowHost_* host) const final
    {
        const uint64_t ptr = reinterpret_cast<uintptr_t>(host);
        SendControlRequest(MessageDpi,
            static_cast<uint32_t>(ptr & 0xffffffff),
            static_cast<uint32_t>((ptr >> 32) & 0xffffffff));
    }
    void UpdateTitle(WindowHost_* host) const final
    {
        const uint64_t ptr = reinterpret_cast<uintptr_t>(host);
        SendControlRequest(MessageUpdTitle,
            static_cast<uint32_t>(ptr & 0xffffffff),
            static_cast<uint32_t>((ptr >> 32) & 0xffffffff));
    }
    void UpdateIcon(WindowHost_* host) const final
    {
        if (!WMIconAtom)
        {
            Logger.Warning(u"Not updating icon due to atom not found.\n");
            return;
        }
        const uint64_t ptr = reinterpret_cast<uintptr_t>(host);
        SendControlRequest(MessageUpdIcon,
            static_cast<uint32_t>(ptr & 0xffffffff),
            static_cast<uint32_t>((ptr >> 32) & 0xffffffff));
    }
    void CloseWindow(WindowHost_* host) const final
    {
        const uint64_t ptr = reinterpret_cast<uintptr_t>(host);
        SendControlRequest(MessageClose,
            static_cast<uint32_t>(ptr & 0xffffffff),
            static_cast<uint32_t>((ptr >> 32) & 0xffffffff));
    }
    void ReleaseWindow(WindowHost_* host) final
    {
        auto& wd = *static_cast<WdHost*>(host);
        //xcb_free_pixmap(Connection, wd.BackImage);
        xcb_free_gc(Connection, wd.GContext);
        UnregisterHost(host);
    }

    OpaqueResource PrepareIcon(WindowHost_& host, ImageView img) const noexcept final
    {
        if (!img.GetDataType().Is(ImgDType::DataTypes::Uint8))
            return {};
        const auto ow = img.GetWidth(), oh = img.GetHeight();
        if (!ow || !oh)
            return {};

        const auto osize = std::min(ow, oh);
        uint16_t tsize = 0;
        if (osize > 256u)
            tsize = 256; // resize to 256x256
        else if (osize < 32u)
            tsize = 32;
        else if (osize % 16 != 0)
            tsize = static_cast<uint16_t>(osize / 16 * 16);
        if (tsize)
            img = img.ResizeTo(tsize, tsize, true, true);
        else
            tsize = static_cast<uint16_t>(osize);
        Ensures(tsize <= 256u && tsize >= 32u && tsize % 16 == 0u);

        if (img.GetDataType() != xziar::img::ImageDataType::BGRA)
            img = img.ConvertTo(xziar::img::ImageDataType::BGRA);

        return static_cast<OpaqueResource>(IconHolder(img.AsSpan<uint32_t>(), tsize, tsize));
    }

    bool PutImageInPixmap(WindowHost_& host_, xcb_pixmap_t pixmap, const ImageView& img, uint16_t pw, uint16_t ph) const noexcept
    {
        const auto w = img.GetWidth(), h = img.GetHeight(); 
        Expects(w <= pw && h <= ph);
        Expects(img.GetDataType() == PixmapDType);
        auto& host = static_cast<WdHost&>(host_);
        const auto rowStep = img.RowSize();
        const auto maxHeight = ReqMaxSize / rowStep;
        const auto trunks = (h + maxHeight - 1) / maxHeight;
        if (trunks > 1)
        {
            Logger.Verbose(u"Image is big [{}x{}], need to transfer with {} trunk({} row each).\n", w, h, trunks, maxHeight);
            std::vector<xcb_void_cookie_t> cks;
            cks.reserve(trunks);
            for (uint16_t i = 0; i < h;)
            {
                const auto row = std::min<size_t>(maxHeight, h - i);
                const auto sendSize = gsl::narrow_cast<uint32_t>(row * rowStep);
                cks.emplace_back(xcb_put_image(Connection, XCB_IMAGE_FORMAT_Z_PIXMAP, pixmap, host.GContext, 
                    w, row, 0, i, 0, Screen->root_depth, sendSize, img.GetRawPtr<uint8_t>() + i * rowStep));
                i += row;
            }
            for (const auto& ck : cks)
                if (!GeneralHandleError(ck)) return false;
        }
        else
        {
            const auto ck = xcb_put_image(Connection, XCB_IMAGE_FORMAT_Z_PIXMAP, pixmap, host.GContext, 
                w, h, 0, 0, 0, Screen->root_depth, gsl::narrow_cast<uint32_t>(img.GetSize()), img.GetRawPtr<uint8_t>());
            if (!GeneralHandleError(ck)) return false;
        }
        xcb_flush(Connection);
        Logger.Verbose(u"Image transfer complete.\n");
        return true;
    }
    OpaqueResource CacheRenderImage(WindowHost_& host_, ImageView img) const noexcept final
    {
        if (!PixmapDType) return {};
        if (const auto w = img.GetWidth(), h = img.GetHeight(); w >= UINT16_MAX || h >= UINT16_MAX)
        {
            uint32_t neww = 0, newh = 0;
            (w > h ? neww : newh) = UINT16_MAX;
            img = img.ResizeTo(neww, newh, true, true);
        }
        if (img.GetDataType() != PixmapDType)
        {
            img = img.ConvertTo(PixmapDType);
        }
        return static_cast<OpaqueResource>(RenderImgHolder(img));
    }

    WindowHost Create(const CreateInfo& info_) final
    {
        XCBCreateInfo info;
        static_cast<CreateInfo&>(info) = info_;
        return Create(info);
    }
    std::shared_ptr<XCBWdHost> Create(const XCBCreateInfo& info) final
    {
        return std::make_shared<WdHost>(*this, info);
    }

    void Free(xcb_pixmap_t pixmap) const noexcept
    {
        GeneralHandleError(xcb_free_pixmap(Connection, pixmap));
    }

    static inline const auto Dummy = RegisterBackend<WindowManagerXCB>();
};


static constexpr auto KeyCodeLookup = BuildStaticLookup(xkb_keysym_t, event::CombinedKey,
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
event::CombinedKey WindowManagerXCB::ProcessKey(xcb_keycode_t keycode) noexcept
{
    const auto keysym = xkb_state_key_get_one_sym(XKBState, keycode);
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



}
