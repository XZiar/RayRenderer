#include "WindowManager.h"
#include "WindowHost.h"

#include "common/Exceptions.hpp"
#include "common/ContainerEx.hpp"
#include "common/StaticLookup.hpp"

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

constexpr uint32_t MessageCreate = 1;
constexpr uint32_t MessageTask = 2;


namespace xziar::gui::detail
{
using namespace std::string_view_literals;
using event::CommonKeys;

struct XCBData
{
    xcb_window_t Handle = 0;
    Display* TheDisplay = nullptr;
};


static event::MouseButton TranslateButtonState(xcb_button_t xcbbtn)
{
    switch (xcbbtn)
    {
    case XCB_BUTTON_INDEX_1: return event::MouseButton::Left;
    case XCB_BUTTON_INDEX_2: return event::MouseButton::Middle;
    case XCB_BUTTON_INDEX_3: return event::MouseButton::Right;
    default:                 return event::MouseButton::None;
    }
}

class WindowManagerXCB : public WindowManager
{
private:
    xcb_connection_t* Connection = nullptr;
    Display* TheDisplay = nullptr;
    xkb_context* XKBContext = nullptr;
    xkb_keymap* XKBKeymap = nullptr;
    xkb_state* XKBState = nullptr;
    xcb_screen_t* Screen = nullptr;
    xcb_window_t ControlWindow = 0;
    xcb_atom_t MsgAtom = 0;
    xcb_atom_t ProtocolAtom = 0;
    xcb_atom_t CloseAtom = 0;
    xkb_mod_index_t CapsLockIndex = 0;
    uint8_t XKBEventID = 0;
    uint8_t XKBErrorID = 0;
    bool IsCapsLock = false;

    bool SupportNewThread() const noexcept override { return true; }

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
    forceinline WindowHost_* GetWindow(xcb_window_t window) const noexcept
    {
        for (const auto& pair : WindowList)
        {
            if (pair.first == window)
            {
                return pair.second;
            }
        }
        return nullptr;
    }
    xcb_atom_t QueryAtom(const std::string_view atomName, bool shouldCreate) const noexcept
    {
        xcb_intern_atom_cookie_t cookie = xcb_intern_atom_unchecked(Connection,
            shouldCreate ? 0 : 1, gsl::narrow_cast<uint16_t>(atomName.size()), atomName.data());
        xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(Connection, cookie, nullptr);
        xcb_atom_t atom = reply->atom;
        free(reply);
        return atom;
    }
    void UpdateXKBState()
    {
        IsCapsLock = xkb_state_mod_index_is_active(XKBState, CapsLockIndex, XKB_STATE_MODS_EFFECTIVE) > 0;
    }
    void SendControlRequest(const uint32_t request, const uint32_t data0 = 0, const uint32_t data1 = 0, const uint32_t data2 = 0, const uint32_t data3 = 0) noexcept
    {
        xcb_client_message_event_t event;
        event.response_type = XCB_CLIENT_MESSAGE;
        event.format = 32;
        event.type = MsgAtom;
        event.window = ControlWindow;
        event.data.data32[0] = request;
        event.data.data32[1] = data0;
        event.data.data32[2] = data1;
        event.data.data32[3] = data2;
        event.data.data32[4] = data3;
        xcb_send_event(
            Connection,
            False,
            ControlWindow,
            XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
            reinterpret_cast<const char*>(&event)
        );
        xcb_flush(Connection);
    }
public:
    WindowManagerXCB() { }
    ~WindowManagerXCB() override { }

    void Initialize() override
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
        MsgAtom = QueryAtom("XZIAR_GUI_MSG", true);
        ProtocolAtom = QueryAtom("WM_PROTOCOLS", false);
        CloseAtom = QueryAtom("WM_DELETE_WINDOW", false);

        xcb_flush(Connection);
    }
    void DeInitialize() noexcept override
    {
        xcb_destroy_window(Connection, ControlWindow);

        xkb_state_unref(XKBState);
        xkb_keymap_unref(XKBKeymap);
        xkb_context_unref(XKBContext);

        xcb_disconnect(Connection);
    }

    void HandleControlMessage(const uint32_t(&data)[5])
    {
        switch (data[0])
        {
        case MessageCreate:
        {
            const uint64_t ptr = ((uint64_t)(data[2]) << 32) + data[1];
            const auto host = reinterpret_cast<WindowHost_*>(static_cast<uintptr_t>(ptr));
            CreateNewWindow_(host);
        } break;
        case MessageTask:
        {
            HandleTask();
        } break;
        }
    }
    void HandleClientMessage(WindowHost_* host, const xcb_atom_t atom, const xcb_client_message_data_t& data)
    {
        if (atom == ProtocolAtom)
        {
            if (data.data32[0] == CloseAtom)
            {
                if (host->OnClose())
                    CloseWindow(host);
                return;
            }
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
        if (const auto host = GetWindow(window); host)
        {
            auto& data = host->template GetOSData<XCBData>();
            data.Handle = window;
            data.TheDisplay = TheDisplay;
            host->Initialize();
        }
    }
    void MessageLoop() override
    {
        xcb_generic_event_t* evt;
        while ((evt = xcb_wait_for_event(Connection)))
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
                    const bool isPress = evtType == XCB_BUTTON_PRESS;
                    const auto btn = TranslateButtonState(msg.detail);
                    host->OnMouseButton(btn, isPress);
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
            case XCB_CLIENT_MESSAGE:
            {
                const auto& msg = *reinterpret_cast<xcb_client_message_event_t*>(evt);
                if (msg.window == ControlWindow)
                {
                    HandleControlMessage(msg.data.data32);
                }
                else if (const auto host = GetWindow(msg.window); host)
                {
                    HandleClientMessage(host, msg.type, msg.data);
                }
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
    void NotifyTask() noexcept override
    {
        SendControlRequest(MessageTask);
    }

    bool CheckCapsLock() const noexcept override
    {
        return IsCapsLock;
    }

    void CreateNewWindow_(WindowHost_* host)
    {
        // Create XID's for window 
        xcb_window_t window = xcb_generate_id(Connection);

        /* Create window */
        const uint32_t valuemask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        const uint32_t eventmask = XCB_EVENT_MASK_EXPOSURE |
            XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION |
            XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
            XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
        const uint32_t valuelist[] = { Screen->white_pixel, eventmask };

        const auto cookie0 = xcb_create_window(
            Connection,
            XCB_COPY_FROM_PARENT,
            window,
            Screen->root,
            0, 0,
            host->Width, host->Height,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            Screen->root_visual,
            valuemask,
            valuelist
        );
        GeneralHandleError(cookie0);

        RegisterHost(window, host);

        // set close
        xcb_change_property(
            Connection,
            XCB_PROP_MODE_APPEND,
            window,
            ProtocolAtom, XCB_ATOM_ATOM, 32,
            1,
            &CloseAtom
        );
        // set title
        const auto title = common::str::to_u8string(host->Title, common::str::Charset::UTF16LE);
        xcb_change_property(
            Connection,
            XCB_PROP_MODE_REPLACE,
            window,
            XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
            gsl::narrow_cast<uint32_t>(host->Title.size()),
            title.c_str()
        );

        xcb_map_window(Connection, window);
        xcb_flush(Connection);
    }
    void CreateNewWindow(WindowHost_* host) override
    {
        const uint64_t ptr = reinterpret_cast<uintptr_t>(host);
        SendControlRequest(MessageCreate,
            static_cast<uint32_t>(ptr & 0xffffffff),
            static_cast<uint32_t>((ptr >> 32) & 0xffffffff));
    }
    void CloseWindow(WindowHost_* host) override
    {
        const auto cookie = xcb_destroy_window(Connection, host->GetOSData<XCBData>().Handle);
        GeneralHandleError(cookie);
        host->Stop();
    }
    void ReleaseWindow(WindowHost_* host) override
    {
        UnregisterHost(host);
    }
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


std::unique_ptr<WindowManager> CreateManagerImpl()
{
    return std::make_unique<WindowManagerXCB>();
}


}