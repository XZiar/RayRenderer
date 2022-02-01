#include "WindowManager.h"
#include "WindowHost.h"
#include "SystemCommon/StringConvert.h"

#include "SystemCommon/Exceptions.h"
#include "common/ContainerEx.hpp"
#include "common/StaticLookup.hpp"
#include "common/StringEx.hpp"
#include "common/StringPool.hpp"
#include "common/StringLinq.hpp"

#include <boost/container/small_vector.hpp>

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
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
constexpr uint32_t MessageUpdTitle  = 3;
constexpr uint32_t MessageClose     = 4;
constexpr uint32_t MessageStop      = 5;


namespace xziar::gui::detail
{
using namespace std::string_view_literals;
using xziar::gui::event::CommonKeys;


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

class WindowManagerXCB final : public XCBBackend, public WindowManager
{
public:
    static constexpr std::string_view BackendName = "XCB"sv;
private:
    class WdHost final : public XCBBackend::XCBWdHost
    {
    public:
        xcb_window_t Handle = 0;
        DragInfos DragInfo;
        bool NeedBackground = true;
        WdHost(WindowManagerXCB& manager, const XCBCreateInfo& info) noexcept :
            XCBWdHost(manager, info) { }
        ~WdHost() final {}
        uint32_t GetWindow() const noexcept final { return Handle; }
    };

    xcb_connection_t* Connection = nullptr;
    Display* TheDisplay = nullptr;
    int DefScreen = 0;
    xkb_context* XKBContext = nullptr;
    xkb_keymap* XKBKeymap = nullptr;
    xkb_state* XKBState = nullptr;
    xcb_screen_t* Screen = nullptr;
    xcb_window_t ControlWindow = 0;
    xcb_atom_t MsgAtom = 0;
    xcb_atom_t WMProtocolAtom = 0;
    xcb_atom_t CloseAtom = 0;
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

    std::string_view Name() const noexcept { return BackendName; }
    bool CheckFeature(std::string_view feat) const noexcept
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

    bool GeneralHandleError(xcb_generic_error_t* err) noexcept
    {
        if (err)
        {
            Logger.error(u"Error: [{}] [{},{}]\n", err->error_code, err->major_code, err->minor_code);
            free(err);
            return false;
        }
        return true;
    }
    forceinline bool GeneralHandleError(xcb_void_cookie_t cookie) noexcept
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
            if (readLen)
            {
                data.resize(offset + readLen);
                memcpy_s(&data[offset], readLen, ptr, readLen);
                offset += readLen;
            }
            want = reply->bytes_after;
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
public:
    WindowManagerXCB() : XCBBackend(true) { }
    ~WindowManagerXCB() override { }

    void OnInitialize(const void* info) final
    {
        using common::BaseException;
        // XInitThreads();
        TheDisplay = XOpenDisplay(nullptr);
        /* open display */
        if (!TheDisplay)
            COMMON_THROW(BaseException, u"Failed to open display");

        // Acquire event queue ownership
        XSetEventQueueOwner(TheDisplay, XCBOwnsEventQueue);

        const auto defScreen = DefaultScreen(TheDisplay);
        // Get the XCB connection from the display
        Connection = XGetXCBConnection(TheDisplay);
        if (!Connection)
        {
            XCloseDisplay(TheDisplay);
            COMMON_THROW(BaseException, u"Can't get xcb connection from display");
        }

        // Setup xkb
        uint16_t xkbVersion[2] = {0};
        const auto xkbRet = xkb_x11_setup_xkb_extension(Connection, XCB_XKB_MAJOR_VERSION, XCB_XKB_MINOR_VERSION, XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, &xkbVersion[0], &xkbVersion[1], &XKBEventID, &XKBErrorID);
        if (xkbRet == 0)
            COMMON_THROW(BaseException, u"Failed to setup xkb extension");
        Logger.debug(u"xkb initialized as [{}.{}] with event id[{}][{}].\n"sv, xkbVersion[0], xkbVersion[1], XKBEventID, XKBErrorID);
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
        auto screenIter = xcb_setup_roots_iterator(xcb_get_setup(Connection));
        for (auto screenCnt = defScreen; screenIter.rem && screenCnt--;)
            xcb_screen_next(&screenIter);
        Screen = screenIter.data;

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
            const std::pair<xcb_atom_t*, std::string_view> atomList[] = 
            {
                { &MsgAtom,             "1XZIAR_GUI_MSG"sv    },
                { &WMProtocolAtom,      "0WM_PROTOCOLS"sv     },
                { &CloseAtom,           "0WM_DELETE_WINDOW"sv },
                { &XdndProxyAtom,       "0XdndProxy"sv        },
                { &XdndAwareAtom,       "0XdndAware"sv        },
                { &XdndTypeListAtom,    "0XdndTypeList"sv     },
                { &XdndEnterAtom,       "0XdndEnter"sv        },
                { &XdndLeaveAtom,       "0XdndLeave"sv        },
                { &XdndPositionAtom,    "0XdndPosition"sv     },
                { &XdndDropAtom,        "0XdndDrop"sv         },
                { &XdndStatusAtom,      "0XdndStatus"sv       },
                { &XdndFinishedAtom,    "0XdndFinished"sv     },
                { &XdndSelectionAtom,   "0XdndSelection"sv    },
                { &XdndActionCopyAtom,  "0XdndActionCopy"sv   },
                { &XdndActionMoveAtom,  "0XdndActionMove"sv   },
                { &XdndActionLinkAtom,  "0XdndActionLink"sv   },
                { &UrlListAtom,         "0text/uri-list"sv    },
                { &PrimaryAtom,         "0PRIMARY"sv          },
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
                xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(Connection, cookie, nullptr);
                *atomList[atomIdx++].first = reply->atom;
                free(reply);
            }
        }
        xcb_flush(Connection);
        XCBBackend::OnInitialize(info);
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
        switch (data[0])
        {
        case MessageCreate:
        {
            const uint64_t ptr = ((uint64_t)(data[2]) << 32) + data[1];
            auto& payload = *reinterpret_cast<CreatePayload*>(static_cast<uintptr_t>(ptr));
            CreateNewWindow_(payload);
        } break;
        case MessageTask:
        {
            HandleTask();
        } break;
        case MessageUpdTitle:
        {
            const uintptr_t ptr = static_cast<uintptr_t>(((uint64_t)(data[2]) << 32) + data[1]);
            const auto host = static_cast<WdHost*>(reinterpret_cast<WindowHost_*>(ptr));
            TitleLock lock(host);
            SetTitle(host->Handle, host->Title);
        } break;
        case MessageClose:
        {
            const uintptr_t ptr = static_cast<uintptr_t>(((uint64_t)(data[2]) << 32) + data[1]);
            const auto host = static_cast<WdHost*>(reinterpret_cast<WindowHost_*>(ptr));
            const auto cookie = xcb_destroy_window(Connection, host->Handle);
            GeneralHandleError(cookie);
            host->Stop();
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
            Logger.info(u"DragEnter from [{}] with {} types:\n", uint32_t(dragInfo.SourceWindow), dragInfo.Types.size());
            for (const auto& type : dragInfo.Types)
            {
                if (type == UrlListAtom)
                    dragInfo.TargetType = type;
                const auto name = QueryAtomName(type);
                Logger.verbose(u"--[{:6}]: [{}]\n", type, name);
            }
        }
        else if (atom == XdndLeaveAtom)
        {
            auto& dragInfo = host->DragInfo;
            dragInfo.Clear();
            const xcb_window_t source = data.data32[0];
            Logger.info(u"DragLeave from [{}]\n", uint32_t(source));
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
                    sendData[4] = XdndActionLinkAtom;
                }
                SendClientMessage(dragInfo.SourceWindow, XdndStatusAtom, sendData);
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
                    Logger.info(u"DragDrop from [{}]\n", uint32_t(dragInfo.SourceWindow));
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
            Logger.verbose(u"Recieve xkb message [{}]\n", (uint32_t)xkbType);
        }
    }

    event::CombinedKey ProcessKey(xcb_keycode_t keycode) noexcept;

    void InitializeWindow(xcb_window_t window)
    {
        if (const auto host_ = GetWindow(window); host_)
        {
            const auto host = static_cast<WdHost*>(host_);
            host->Handle = window;
            host->Initialize();
        }
    }
    void OnMessageLoop() final
    {
        xcb_generic_event_t* evt;
        bool shouldContinue = true;
        while (shouldContinue && (evt = xcb_wait_for_event(Connection)))
        {
            // Logger.verbose(u"Recieve message [{}]\n", (uint32_t)event->response_type);
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
                    // Logger.verbose(u"key: [{}] => [{}]\n", (uint32_t)msg.detail, common::enum_cast(key.Key));
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
            case XCB_SELECTION_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_selection_notify_event_t*>(evt);
                Logger.info(u"Recieve selection[{}]({}) target[{}]({}) property[{}]({})\n",
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
                Logger.error(u"Error: [{}] [{},{}]\n", err.error_code, err.major_code, err.minor_code);
            } break;
            default:
            {
                if (evtType == XKBEventID) // handle xkb event
                    HandleXKBEvent(evt);
                else if (evtType == XKBErrorID)
                    Logger.warning(u"Recieve xkb error [{}]\n", evt->pad0);
                else
                    Logger.verbose(u"Recieve message [{}]\n", evtType);
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
        bool needBackground = true;
        if (payload.ExtraData)
        {
            const auto vi = (*payload.ExtraData)("visual");
            if (vi)
                visualId = *reinterpret_cast<const int*>(vi);
            const auto bg = (*payload.ExtraData)("background");
            if (bg)
                needBackground = *reinterpret_cast<const bool*>(bg);
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
        if (needBackground)
        {
            valuemask |= XCB_CW_BACK_PIXEL;
            valuelist[valueIdx++] = Screen->white_pixel;
        }
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
    void UpdateTitle(WindowHost_* host) const final
    {
        const uint64_t ptr = reinterpret_cast<uintptr_t>(host);
        SendControlRequest(MessageUpdTitle,
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
        UnregisterHost(host);
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