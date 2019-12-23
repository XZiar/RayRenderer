#include "WindowManager.h"
#include "WindowHost.h"

#include "common/Exceptions.hpp"
#include "common/ContainerEx.hpp"
#include "common/PromiseTaskSTD.hpp"

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#undef Always
#undef None

constexpr uint32_t MessageCreate = 1;


namespace xziar::gui::detail
{

static thread_local WindowManager* TheManager;


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
    xcb_screen_t* Screen = nullptr;
    xcb_window_t ControlWindow = 0;
    xcb_atom_t MsgAtom;
    xcb_atom_t ProtocolAtom;
    xcb_atom_t CloseAtom;

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
    void Terminate() noexcept override
    {
        TheManager = nullptr;
        xcb_destroy_window(Connection, ControlWindow);
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
        }
    }

    void HandleClientMessage(WindowHost_* host, const xcb_atom_t atom, const xcb_client_message_data_t& data)
    {
        if (atom == ProtocolAtom)
        {
            if (data.data32[0] == CloseAtom)
            {
                host->OnClose();
                return;
            }
        }

    }

    void MessageLoop() override
    {
        xcb_generic_event_t* event;
        while ((event = xcb_wait_for_event(Connection)))
        {
            // Logger.verbose(u"Recieve message [{}]\n", (uint32_t)event->response_type);
            switch (event->response_type & 0x7f)
            {
            case XCB_CREATE_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_create_notify_event_t*>(event);
                if (const auto host = GetWindow(msg.window); host)
                {
                    host->Handle = msg.window;
                    host->DCHandle = TheDisplay;
                    host->Initialize();
                    break;
                }
            } break;
            case XCB_REPARENT_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_reparent_notify_event_t*>(event);
                if (const auto host = GetWindow(msg.window); host)
                {
                    host->Handle = msg.window;
                    host->DCHandle = TheDisplay;
                    host->Initialize();
                    break;
                }
            } break;
            case XCB_MOTION_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_motion_notify_event_t*>(event);
                if (const auto host = GetWindow(msg.event); host)
                {
                    event::Position pos(msg.event_x, msg.event_y);
                    host->OnMouseMove(pos);
                }
            } break;
            case XCB_ENTER_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_leave_notify_event_t*>(event);
                if (const auto host = GetWindow(msg.event); host)
                {
                    event::Position pos(msg.event_x, msg.event_y);
                    host->OnMouseEnter(pos);
                }
            } break;
            case XCB_LEAVE_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_leave_notify_event_t*>(event);
                if (const auto host = GetWindow(msg.event); host)
                {
                    host->OnMouseLeave();
                }
            } break;
            case XCB_BUTTON_PRESS:
            case XCB_BUTTON_RELEASE:
            {
                const auto& msg = *reinterpret_cast<xcb_button_press_event_t*>(event);
                if (const auto host = GetWindow(msg.event); host)
                {
                    const bool isPress = (event->response_type & 0x7f) == XCB_BUTTON_PRESS;
                    const auto btn = TranslateButtonState(msg.detail);
                    host->OnMouseButton(btn, isPress);
                }

            } break;
            case XCB_CONFIGURE_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_configure_notify_event_t*>(event);
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
                const auto& msg = *reinterpret_cast<xcb_client_message_event_t*>(event);
                if (msg.window == ControlWindow)
                {
                    HandleControlMessage(msg.data.data32);
                }
                else  if (const auto host = GetWindow(msg.window); host)
                {
                    HandleClientMessage(host, msg.type, msg.data);
                }
            } break;
            default:
                Logger.verbose(u"Recieve message [{}]\n", (uint32_t)(event->response_type & 0x7f));
                break;
            }
            free(event);
        }
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

        WindowList.emplace_back(static_cast<uintptr_t>(window), host);

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
        const auto title = common::strchset::to_u8string(host->Title, common::str::Charset::UTF16LE);
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
        xcb_client_message_event_t event;
        event.response_type = XCB_CLIENT_MESSAGE;
        event.format = 32;
        event.type = MsgAtom;
        event.window = ControlWindow;
        const uint64_t ptr = reinterpret_cast<uintptr_t>(host);
        event.data.data32[0] = MessageCreate;
        event.data.data32[1] = static_cast<uint32_t>(ptr & 0xffffffff);
        event.data.data32[2] = static_cast<uint32_t>((ptr >> 32) & 0xffffffff);
        xcb_send_event(
            Connection,
            False,
            ControlWindow,
            XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
            reinterpret_cast<const char*>(&event)
        );
        xcb_flush(Connection);
    }
    void CloseWindow(WindowHost_* host) override
    {
        const auto cookie = xcb_destroy_window(Connection, static_cast<xcb_window_t>(host->Handle));
        GeneralHandleError(cookie);
    }
    void ReleaseWindow(WindowHost_* host) override
    {
    }
};



std::shared_ptr<WindowManager> CreateManagerImpl()
{
    return std::make_shared<WindowManagerXCB>();
}


}