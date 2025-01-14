#define TGUI_TEST 1
#if defined(TGUI_TEST) || (defined(__has_include) && __has_include(<termuxgui/termuxgui.h>))

#include "WindowManager.h"
#include "WindowHost.h"
#include "ImageUtil/ImageUtil.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/StackTrace.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/PromiseTaskSTD.h"
#include "SystemCommon/DynamicLibrary.h"
#include "SystemCommon/ErrorCodeHelper.h"
#include "common/ContainerEx.hpp"
#include "common/StaticLookup.hpp"
#include "common/MemoryStream.hpp"
#include "common/TimeUtil.hpp"
#include "common/Linq2.hpp"

#include <boost/container/small_vector.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/lockfree/queue.hpp>

#if defined(__has_include) && __has_include(<termuxgui/termuxgui.h>)
#   include <termuxgui/termuxgui.h>
#   undef __INTRODUCED_IN // not directly use, ignore api check
#   define __INTRODUCED_IN(x)
#   include <android/hardware_buffer.h>
#else
#   include "../Tests/Data/termuxgui/termuxgui.h"
struct AHardwareBuffer_Desc
{
    uint32_t width;
    uint32_t height;
    uint32_t layers;
    uint32_t format;
    uint64_t usage;
    uint32_t stride;
    uint32_t rfu0;
    uint64_t rfu1;
};
void AHardwareBuffer_describe(const AHardwareBuffer* buffer, AHardwareBuffer_Desc* outDesc);
int AHardwareBuffer_lock(AHardwareBuffer* buffer, uint64_t usage, int32_t fence, const void* rect, void** outVirtualAddress);
int AHardwareBuffer_lockAndGetInfo(AHardwareBuffer* buffer, uint64_t usage, int32_t fence, const void* rect, void** outVirtualAddress, int32_t* outBytesPerPixel, int32_t* outBytesPerStride);
int AHardwareBuffer_unlock(AHardwareBuffer* buffer, int32_t* fence);
inline constexpr auto AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY = 2UL << 4;
inline constexpr auto AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN = 3UL << 4;
#endif

#include <mutex>
#include <deque>


#define THROW_TERR(eval, msg) do { const tgui_err err___ = eval;    \
IF_UNLIKELY(err___ != TGUI_ERR_OK)                                  \
{                                                                   \
    const auto errmsg = TGUIErrorNames(err___).value_or("UNKNOWN"); \
    Logger.Error(msg "[{}({})].\n", errmsg, (int)err___);           \
    COMMON_THROWEX(BaseException, msg)                              \
        .Attach("detail", errmsg).Attach("errorcode", err___);      \
} } while(0)

#define LOG_TERR(eval, log, msg) do { const tgui_err err___ = eval; \
IF_UNLIKELY(err___ != TGUI_ERR_OK)                                  \
{                                                                   \
    const auto errmsg = TGUIErrorNames(err___).value_or("UNKNOWN"); \
    log.Error(msg "[{}({})].\n", errmsg, (int)err___);              \
} } while(0)


#if COMMON_COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunused-const-variable"
#endif

static inline constexpr uint32_t MessageCreate    = 1;
static inline constexpr uint32_t MessageTask      = 2;
static inline constexpr uint32_t MessageClose     = 3;
static inline constexpr uint32_t MessageStop      = 4;
static inline constexpr uint32_t MessageDpi       = 5;
static inline constexpr uint32_t MessageUpdTitle  = 10;
static inline constexpr uint32_t MessageUpdIcon   = 11;
static inline constexpr uint32_t MessageClipboard = 12;
static inline constexpr uint32_t MessageFilePick  = 13;

#if COMMON_COMPILER_CLANG
#   pragma clang diagnostic pop
#endif


namespace xziar::gui::detail
{
using namespace std::string_view_literals;
using xziar::gui::event::CommonKeys;
using xziar::img::Image;
using xziar::img::ImageView;
using xziar::img::ImgDType;
using common::BaseException;
using common::SimpleTimer;
using common::enum_cast;


#define TERR(name) { TGUI_ERR_##name, #name ""sv }
static constexpr auto TGUIErrorNames = BuildStaticLookup(int, std::string_view,
    TERR(OK), TERR(SYSTEM), TERR(CONNECTION_LOST), TERR(ACTIVITY_DESTROYED), TERR(MESSAGE), TERR(NOMEM), TERR(EXCEPTION), TERR(VIEW_INVALID), TERR(API_LEVEL));
#undef TERR

namespace tgui
{

static constexpr auto KeyCodeLookup = BuildStaticLookup(uint32_t, event::CombinedKey,
    { 131,  CommonKeys::F1 },
    { 132,  CommonKeys::F2 },
    { 133,  CommonKeys::F3 },
    { 134,  CommonKeys::F4 },
    { 135,  CommonKeys::F5 },
    { 136,  CommonKeys::F6 },
    { 137,  CommonKeys::F7 },
    { 138,  CommonKeys::F8 },
    { 139,  CommonKeys::F9 },
    { 140,  CommonKeys::F10 },
    { 141,  CommonKeys::F11 },
    { 142,  CommonKeys::F12 },
    { 21,   CommonKeys::Left },
    { 19,   CommonKeys::Up },
    { 22,   CommonKeys::Right },
    { 20,   CommonKeys::Down },
    { 59,   CommonKeys::Shift },
    { 60,   CommonKeys::Shift },
    { 61,   CommonKeys::Tab },
    { 62,   CommonKeys::Space },
    { 66,   CommonKeys::Enter },
    { 160,  CommonKeys::Enter },
    { 67,   CommonKeys::Backspace },
    { 78,   CommonKeys::Alt },
    { 92,   CommonKeys::PageUp },
    { 93,   CommonKeys::PageDown },
    { 111,  CommonKeys::Esc },
    { 112,  CommonKeys::Delete },
    { 113,  CommonKeys::Ctrl },
    { 114,  CommonKeys::Ctrl },
    { 115,  CommonKeys::CapsLock },
    { 122,  CommonKeys::Home },
    { 123,  CommonKeys::End },
    { 124,  CommonKeys::Insert },
    { 17,   '*' },
    { 18,   '#' },
    { 55,   ',' },
    { 56,   '.' },
    { 68,   '`' },
    { 69,   '-' },
    { 70,   '=' },
    { 71,   '[' },
    { 72,   ']' },
    { 73,   '\\' },
    { 74,   ';' },
    { 75,   '\'' },
    { 76,   '/' },
    { 77,   '@' },
    { 81,   '+' },
    { 154,  '/' },
    { 155,  '*' },
    { 156,  '-' },
    { 157,  '+' },
    { 158,  '.' },
    { 159,  '.' },
    { 161,  '=' },
    { 162,  '(' },
    { 163,  ')' },
);
static event::CombinedKey ProcessAndroidKey(uint32_t keycode, uint32_t codePoint) noexcept
{
    if (codePoint >= '0' && codePoint <= '9')
        return static_cast<uint8_t>(codePoint);
    if (codePoint >= 'A' && codePoint <= 'Z')
        return static_cast<uint8_t>(codePoint);
    if (codePoint >= 'a' && codePoint <= 'z')
        return static_cast<uint8_t>(codePoint - 'a' + 'A');
    return KeyCodeLookup(keycode).value_or(CommonKeys::UNDEFINE);
}

struct IconHolder : public OpaqueResource
{
    explicit IconHolder(std::unique_ptr<std::vector<std::byte>> data) noexcept : OpaqueResource(&TheDisposer, data.release())
    { }
    std::vector<std::byte>& Data() const noexcept { return *reinterpret_cast<std::vector<std::byte>*>(Cookie[0]); }
    static void TheDisposer(const OpaqueResource& res) noexcept
    {
        const auto& holder = static_cast<const IconHolder&>(res);
        if (holder.Cookie[0])
            delete &holder.Data();
    }
};

struct FilePicker
{
    struct Item
    {
        common::fs::path Path;
        bool IsFolder;
        bool IsSelected = false;
        Item(common::fs::path&& path, bool isFolder) noexcept : Path(std::move(path)), IsFolder(isFolder) {}
    };
    struct Task
    {
        FilePickerInfo Info;
        std::promise<FileList> Pms;
        common::fs::path CurPath;
        std::vector<Item> CurItems;
        uint32_t FilterIdx;
        Task(const FilePickerInfo& info) : Info(info), CurPath(common::fs::current_path()), FilterIdx(Info.ExtensionFilters.empty() ? UINT32_MAX : 0u) {}
        void UpdateList(common::mlog::MiniLogger<false>&, const common::fs::path& newPath)
        {
            std::vector<common::fs::path> folders, files;
            for (const auto& entry : common::fs::directory_iterator(newPath))
            {
                if (entry.is_directory())
                    folders.push_back(entry.path());
                else if (!Info.IsFolder)
                {
                    if (FilterIdx != UINT32_MAX)
                    {
                        const auto& filters = Info.ExtensionFilters[FilterIdx].second;
                        auto ext = entry.path().extension().u16string();
                        if (!ext.empty()) ext = ext.substr(1); // remove dot
                        if (std::find(filters.begin(), filters.end(), ext) == filters.end())
                        {
                            continue;
                        }
                    }
                    files.push_back(entry.path());
                }
            }
            std::sort(folders.begin(), folders.end());
            std::sort(files.begin(), files.end());
            CurPath = newPath;
            CurItems.clear();
            for (auto& item : folders)
                CurItems.emplace_back(std::move(item), true);
            for (auto& item : files)
                CurItems.emplace_back(std::move(item), false);
        }
    };
    std::mutex MainMutex;
    std::deque<Task> TaskList;
    tgui_activity Activity = -1;
    tgui_view View = -1;
    tgui_view BtnOK = -1, BtnCancel = -1, TxtTitle = -1, TxtPath = -1, SpinFilter = -1, ViewBtns = -1, ViewListHolder = -1, ViewList = -1;
    boost::container::flat_map<tgui_view, uint32_t> CurItems;
};

}
using namespace tgui;

class WindowManagerTermuxGUI final : public TermuxGUIBackend, public WindowManager
{
public:
    static constexpr std::string_view BackendName = "TermuxGUI"sv;
private:
    inline static constexpr ImgDType BackBufType = img::ImageDataType::RGBA;
    struct BackBuffer
    {
        tgui_hardware_buffer HWBuf = { 0, nullptr };
        RectBase<int32_t> Shape;
        uint32_t RealWidth = 0;
        bool UpdateSize(WindowManagerTermuxGUI& manager, const RectBase<int32_t>& shape, tgui_hardware_buffer_cpu_frequency usage)
        {
            if (shape != Shape)
            {
                Shape = shape;
                Release(manager);
                LOG_TERR(manager.hardware_buffer_create(manager.Connection, &HWBuf, TGUI_HARDWARE_BUFFER_FORMAT_RGBA8888,
                    Shape.Width, Shape.Height, usage, usage), manager.Logger, u"Can't create HW Buffer");
                AHardwareBuffer_Desc desc{};
                manager.AHB_describe(HWBuf.buffer, &desc);
                RealWidth = desc.stride;
                Ensures(desc.stride >= static_cast<uint32_t>(Shape.Width));
                manager.Logger.Verbose(u"Resize HWBuffer to [{}({})x{}].\n", Shape.Width, RealWidth, Shape.Height);
                return true;
            }
            return false;
        }
        void Release(WindowManagerTermuxGUI& manager)
        {
            if (HWBuf.buffer)
            {
                LOG_TERR(manager.hardware_buffer_destroy(manager.Connection, &HWBuf), manager.Logger, u"Can't destroy HW Buffer");
                HWBuf = { 0, nullptr };
            }
        }
    };
    class WdHost;
    struct HWRenderer
    {
        WdHost& Window;
        CacheRect<int32_t> LastRect;
        BackBuffer BackBuf[2] = { };
        uint32_t BackBufIdx = 0;
        HWRenderer(WdHost* wd) : Window(*wd) {}
        [[nodiscard]] forceinline BackBuffer& Back() noexcept { return BackBuf[BackBufIdx]; }
        [[nodiscard]] forceinline const BackBuffer& Back() const noexcept { return BackBuf[BackBufIdx]; }
        forceinline std::pair<bool, bool> Update(tgui_hardware_buffer_cpu_frequency usage = TGUI_HARDWARE_BUFFER_CPU_NEVER)
        {
            auto& backBuf = Back();
            const auto [sizeChanged, needInit] = LastRect.Update(Window);
            backBuf.UpdateSize(Window.GetManager(), LastRect, usage);
            return { sizeChanged, needInit };
        }
        forceinline void Draw()
        {
            auto& backBuf = Back();
            auto& manager = Window.GetManager();
            manager.surface_view_set_buffer(manager.Connection, Window.Activity, Window.View, &backBuf.HWBuf);
            BackBufIdx = BackBufIdx ^ 0b1u;
        }
    };
    class CPURenderer final : public HWRenderer, public BasicRenderer
    {
        friend WindowManagerTermuxGUI;
        std::optional<ImageView> AttachedImage;
        LockField ResourceLock;
        [[nodiscard]] forceinline auto& Log() noexcept { return Window.Manager.Logger; }
        [[nodiscard]] forceinline auto UseImg() noexcept { return detail::LockField::ConsumerLock<0>{ResourceLock}; }
        bool ReplaceImage(std::optional<ImageView> img)
        {
            detail::LockField::ProducerLock<0> lock{ ResourceLock };
            AttachedImage = std::move(img);
            return lock;
        }
    public:
        using HWRenderer::HWRenderer;
        ~CPURenderer() final { }
        void Initialize() noexcept
        {
        }
        void Render(bool forceRedraw) noexcept;
        void SetImage(std::optional<ImageView> src) noexcept final
        {
            if (src)
            {
                std::optional<Image> img;
                if (const auto w = src->GetWidth(), h = src->GetHeight(); w >= UINT16_MAX || h >= UINT16_MAX)
                {
                    uint32_t neww = 0, newh = 0;
                    (w > h ? neww : newh) = UINT16_MAX;
                    img = src->ResizeTo(neww, newh, true, true);
                }
                if (src->GetDataType() != BackBufType)
                {
                    if (img) img = img->ConvertTo(BackBufType);
                    else img = src->ConvertTo(BackBufType);
                }
                // pre-filp since back buffer is flipped
                if (img)
                    img->FlipVertical();
                else
                    img = src->FlipToVertical();
                ReplaceImage(*img);
            }
            else
                ReplaceImage({});
        }
    };
    struct Msg
    {
        WdHost* Host;
        void* Ptr;
        uint32_t OpCode;
        uint32_t Data;
        constexpr Msg(WdHost* host, uint32_t op, uint32_t data = 0, void* ptr = nullptr) noexcept : Host(host), Ptr(ptr), OpCode(op), Data(data) {}
        constexpr Msg() noexcept : Msg(nullptr, 0) {}
    };
    class WdHost final : public TermuxGUIBackend::TermuxGUIWdHost
    {
    public:
        tgui_activity Activity = -1;
        tgui_view View = -1;
        std::pair<float, float> DPIs = { 0.f, 0.f };
        std::variant<std::monostate, HWRenderer, CPURenderer> Renderer;
        int32_t MainPointer = -1;
        [[nodiscard]] forceinline WindowManagerTermuxGUI& GetManager() noexcept { return static_cast<WindowManagerTermuxGUI&>(Manager); }
        WdHost(WindowManagerTermuxGUI& manager, const TermuxGUICreateInfo& info) noexcept : TermuxGUIWdHost(manager, info) 
        {
            if (info.UseDefaultRenderer)
                Renderer.emplace<CPURenderer>(this);
            else
                Renderer.emplace<HWRenderer>(this);
        }
        ~WdHost() final {}
        uint64_t FormatAs() const noexcept
        {
            return (static_cast<uint64_t>(Activity) << 32) + static_cast<uint32_t>(View);
        }

        [[nodiscard]] void* GetCurrentHWBuffer() const noexcept final
        {
            if (Renderer.index() == 1)
            {
                return std::get<1>(Renderer).Back().HWBuf.buffer;
            }
            return nullptr;
        }
        void OnDisplay(bool forceRedraw) noexcept final
        {
            switch (Renderer.index())
            {
            case 1: std::get<1>(Renderer).Update(); break;
            case 2: std::get<2>(Renderer).Render(forceRedraw); break;
            default: break;
            }
            WindowHost_::OnDisplay(forceRedraw);
            if (Renderer.index() == 1)
                std::get<1>(Renderer).Draw();
        }
        BasicRenderer* GetRenderer() noexcept final { return Renderer.index() == 2 ? &std::get<2>(Renderer) : nullptr; }
    };

    common::DynLib TGui;
    common::DynLib LibAndroid;
#define DECLARE_FUNC(func) decltype(&tgui_##func) func = nullptr
    DECLARE_FUNC(connection_create);
    DECLARE_FUNC(connection_destroy);
    DECLARE_FUNC(get_version);
    DECLARE_FUNC(get_log);
    DECLARE_FUNC(wait_event);
    DECLARE_FUNC(poll_event);
    DECLARE_FUNC(event_destroy); 
    DECLARE_FUNC(focus);
    DECLARE_FUNC(toast);
    DECLARE_FUNC(activity_create);
    DECLARE_FUNC(activity_get_configuration);
    DECLARE_FUNC(activity_set_task_description);
    DECLARE_FUNC(activity_set_input_mode);
    DECLARE_FUNC(activity_intercept_back_button);
    DECLARE_FUNC(activity_finish);
    DECLARE_FUNC(create_linear_layout);
    DECLARE_FUNC(create_button);
    DECLARE_FUNC(create_checkbox);
    DECLARE_FUNC(create_spinner);
    DECLARE_FUNC(create_nested_scroll_view);
    DECLARE_FUNC(create_text_view);
    DECLARE_FUNC(create_image_view); 
    DECLARE_FUNC(create_surface_view);
    DECLARE_FUNC(create_web_view);
    DECLARE_FUNC(delete_view);
    DECLARE_FUNC(delete_children);
    DECLARE_FUNC(get_dimensions);
    DECLARE_FUNC(send_touch_event);
    DECLARE_FUNC(send_click_event);
    DECLARE_FUNC(set_clickable);
    DECLARE_FUNC(visibility);
    DECLARE_FUNC(set_margin);
    DECLARE_FUNC(set_width);
    DECLARE_FUNC(set_height);
    DECLARE_FUNC(linear_params);
    DECLARE_FUNC(set_list);
    DECLARE_FUNC(select_item);
    DECLARE_FUNC(set_text);
    DECLARE_FUNC(text_size);
    DECLARE_FUNC(set_image); 
    DECLARE_FUNC(add_buffer);
    DECLARE_FUNC(delete_buffer);
    DECLARE_FUNC(blit_buffer);
    DECLARE_FUNC(set_buffer);
    DECLARE_FUNC(refresh_image_view);
    DECLARE_FUNC(webview_allow_javascript);
    DECLARE_FUNC(webview_set_data);
    DECLARE_FUNC(webview_eval_js);
    DECLARE_FUNC(hardware_buffer_create); 
    DECLARE_FUNC(hardware_buffer_destroy);
    DECLARE_FUNC(surface_view_set_buffer);
    DECLARE_FUNC(surface_view_config);
#undef DECLARE_FUNC
#define DECLARE_FUNC(func) decltype(&AHardwareBuffer_##func) AHB_##func = nullptr
    DECLARE_FUNC(describe);
    DECLARE_FUNC(lockAndGetInfo);
    DECLARE_FUNC(lock);
    DECLARE_FUNC(unlock);
#undef DECLARE_FUNC
    tgui_connection Connection = nullptr;
    mutable boost::lockfree::queue<Msg, boost::lockfree::capacity<1024>> MsgQueue;
    FilePicker Picker;
    bool IsCapsLock = false;

    std::string_view Name() const noexcept final { return BackendName; }
    bool CheckFeature(std::string_view feat) const noexcept final
    {
        constexpr std::string_view Features[] =
        {
            "OpenGLES"sv, "Vulkan"sv, "NewThread"sv,
        };
        if (std::find(std::begin(Features), std::end(Features), feat) != std::end(Features))
            return true;
        return false;
    }

    void SendControlMessage(WdHost* host, uint32_t op, uint32_t data = 0, void* ptr = nullptr) const noexcept
    {
        //Logger.Verbose(u"Push Msg[{}] on window[{:x}].\n", op, (uintptr_t)host->Surface.Proxy);
        MsgQueue.push({ host, op, data, ptr });
    }
public:
    WindowManagerTermuxGUI() : TermuxGUIBackend(true), TGui("libtermuxgui.so"), LibAndroid("libandroid.so")
    {
#define LOAD_FUNC(func) func = TGui.GetFunction<decltype(&tgui_##func)>("tgui_" #func ""sv)
        LOAD_FUNC(connection_create);
        LOAD_FUNC(connection_destroy);
        LOAD_FUNC(get_version);
        LOAD_FUNC(get_log);
        LOAD_FUNC(wait_event);
        LOAD_FUNC(poll_event);
        LOAD_FUNC(event_destroy);
        LOAD_FUNC(focus);
        LOAD_FUNC(toast);
        LOAD_FUNC(activity_create);
        LOAD_FUNC(activity_get_configuration);
        LOAD_FUNC(activity_set_task_description);
        LOAD_FUNC(activity_set_input_mode);
        LOAD_FUNC(activity_intercept_back_button);
        LOAD_FUNC(activity_finish);
        LOAD_FUNC(create_linear_layout);
        LOAD_FUNC(create_button);
        LOAD_FUNC(create_checkbox);
        LOAD_FUNC(create_spinner);
        LOAD_FUNC(create_nested_scroll_view);
        LOAD_FUNC(create_text_view);
        LOAD_FUNC(create_image_view);
        LOAD_FUNC(create_surface_view);
        LOAD_FUNC(create_web_view);
        LOAD_FUNC(delete_view);
        LOAD_FUNC(delete_children);
        LOAD_FUNC(get_dimensions);
        LOAD_FUNC(send_touch_event);
        LOAD_FUNC(send_click_event);
        LOAD_FUNC(set_clickable);
        LOAD_FUNC(visibility);
        LOAD_FUNC(set_margin);
        LOAD_FUNC(set_width);
        LOAD_FUNC(set_height);
        LOAD_FUNC(linear_params);
        LOAD_FUNC(set_list);
        LOAD_FUNC(select_item);
        LOAD_FUNC(set_text);
        LOAD_FUNC(text_size);
        LOAD_FUNC(set_image);
        LOAD_FUNC(add_buffer);
        LOAD_FUNC(delete_buffer);
        LOAD_FUNC(blit_buffer);
        LOAD_FUNC(set_buffer);
        LOAD_FUNC(refresh_image_view);
        LOAD_FUNC(webview_allow_javascript);
        LOAD_FUNC(webview_set_data);
        LOAD_FUNC(webview_eval_js);
        LOAD_FUNC(hardware_buffer_create);
        LOAD_FUNC(hardware_buffer_destroy);
        LOAD_FUNC(surface_view_set_buffer);
        LOAD_FUNC(surface_view_config);
#undef LOAD_FUNC
#define LOAD_FUNC(func) AHB_##func = LibAndroid.GetFunction<decltype(&AHardwareBuffer_##func)>("AHardwareBuffer_" #func ""sv)
        LOAD_FUNC(describe);
        LOAD_FUNC(lockAndGetInfo);
        LOAD_FUNC(lock);
        LOAD_FUNC(unlock);
#undef LOAD_FUNC
    }
    ~WindowManagerTermuxGUI() final
    { }

    void OnInitialize(const void* info_) final
    {
        THROW_TERR(connection_create(&Connection), u"Can't connect termux-gui");
        int ver = 0;
        THROW_TERR(get_version(Connection, &ver), u"Can't get termux-gui version");
        Logger.Info(u"Termux-GUI version [{}].\n", ver);
        TermuxGUIBackend::OnInitialize(info_);
    }
    void OnDeInitialize() noexcept final
    {
        connection_destroy(Connection);
    }

    void InitializeWindow(WdHost& host)
    {
        if (host.View == -1) 
        {
            THROW_TERR(activity_set_input_mode(Connection, host.Activity, TGUI_ACTIVITY_INPUT_MODE_RESIZE), u"Can't set input mode");
            //THROW_TERR(activity_intercept_back_button(Connection, host.Activity, true), u"Can't set intercept back");
            THROW_TERR(create_surface_view(Connection, host.Activity, &host.View, nullptr, TGUI_VIS_VISIBLE, true), u"Can't create surface view");
            Logger.Verbose(u"create view [{}] for [{}]\n", host.View, host.Activity);
            THROW_TERR(surface_view_config(Connection, host.Activity, host.View, 0xffffffff, TGUI_MISMATCH_STICK_TOPLEFT, TGUI_MISMATCH_STICK_TOPLEFT, 60), u"Can't config surface view");
            THROW_TERR(send_touch_event(Connection, host.Activity, host.View, true), u"Can't enable touch for surface view");
        }
        host.Initialize();
    }
    void OnMessageLoop() final
    {
        tgui_event evt;
        SimpleTimer timer;
        timer.Start();
        bool shouldContinue = true;
        while (shouldContinue)
        {
            bool hasProcess = false;
            for (bool getEvt = true; getEvt;)
            {
                THROW_TERR(poll_event(Connection, &evt, &getEvt), u"Can't poll event");
                if (getEvt)
                {
                    hasProcess = true;
                    const auto host = GetWindow(evt.activity);
                    switch (evt.type)
                    {
                    case TGUI_EVENT_CONFIG:
                        Logger.Verbose(u"=>config country[{}] lang[{}] screen[{}][{}x{}]x{} fscale[{}]\n", evt.configuration.country, evt.configuration.language, 
                            enum_cast(evt.configuration.orientation), evt.configuration.screen_width, evt.configuration.screen_height, evt.configuration.density, evt.configuration.font_scale);
                        break;
                    case TGUI_EVENT_CREATE:
                        Logger.Verbose(u"=>create [{}]\n", evt.activity);
                        if (host)
                            InitializeWindow(*static_cast<WdHost*>(host));
                        else if (evt.activity == Picker.Activity)
                            FilePickInit();
                        break;
                    case TGUI_EVENT_START:
                        Logger.Verbose(u"=>start [{}]\n", evt.activity);
                        break;
                    case TGUI_EVENT_RESUME:
                        Logger.Verbose(u"=>resume [{}]\n", evt.activity);
                        if (!host) break;
                        if (auto& wd = *static_cast<WdHost*>(host); evt.focus.id == wd.View) // only check first 
                            LOG_TERR(focus(Connection, wd.Activity, wd.View, true), Logger, u"Can't focus");
                        break;
                    case TGUI_EVENT_PAUSE:
                        Logger.Verbose(u"=>pause [{}]\n", evt.activity);
                        if (host)
                            host->OnResize(0, 0);
                        break;
                    case TGUI_EVENT_STOP:
                        Logger.Verbose(u"=>stop [{}]\n", evt.activity);
                        break;
                    case TGUI_EVENT_DESTROY:
                        Logger.Verbose(u"=>destroy [{}]\n", evt.activity);
                        if (host)
                            host->Stop();
                        break;
                    case TGUI_EVENT_KEY:
                        Logger.Verbose(u"=>key [{}]\n", evt.activity);
                        if (host)
                        {
                            const auto key = ProcessAndroidKey(evt.key.code, evt.key.codePoint);
                            Logger.Verbose(u"key: [{}][{}] [{:08X}](mod[{:08b}]) => [{}]\n", evt.key.code, static_cast<char32_t>(evt.key.codePoint), 
                                evt.key.flags, enum_cast(evt.key.mod), enum_cast(key.Key));
                            IsCapsLock = evt.key.mod & TGUI_MOD_CAPS_LOCK;
                            if (evt.key.down)
                                host->OnKeyDown(key);
                            else
                                host->OnKeyUp(key);
                        }
                        break;
                    case TGUI_EVENT_TOUCH:
                        // Logger.Verbose(u"=>touch [{}]: [{}] {} evt {} pointer {} idx, at[{}]\n", evt.activity, enum_cast(evt.touch.action), evt.touch.events, evt.touch.num_pointers, evt.touch.index, evt.touch.time);
                        if (!host) break;
                        if (evt.touch.num_pointers >= 3 && Picker.Activity == -1)
                        {
                            host->OnKeyDown(event::CommonKeys::Ctrl);
                            host->OnKeyDown('1');
                            host->OnKeyUp('1');
                            host->OnKeyUp(event::CommonKeys::Ctrl);
                        }
                        if (auto& wd = *static_cast<WdHost*>(host); evt.touch.id == wd.View) // only check first 
                        {
                            switch (evt.touch.action)
                            {
                            case TGUI_TOUCH_DOWN:
                                IF_UNLIKELY(evt.touch.events != 1) { Logger.Warning(u"Multiple event for DOWN is unexpected.\n"); }
                                wd.MainPointer = evt.touch.pointers[0][0].id;
                                [[fallthrough]];
                            case TGUI_TOUCH_UP:
                            case TGUI_TOUCH_POINTER_UP:
                                for (uint32_t i = 0; i < evt.touch.events; ++i)
                                {
                                    for (uint32_t j = 0; j < evt.touch.num_pointers; ++j)
                                    {
                                        const auto& pt = evt.touch.pointers[i][j];
                                        if (j == evt.touch.index && pt.id == wd.MainPointer) // only check first 
                                        {
                                            event::Position pos(pt.x, pt.y);
                                            if (pos != wd.LastPos)
                                                host->OnMouseMove(pos);
                                            host->OnMouseButton(event::MouseButton::Left, evt.touch.action == TGUI_TOUCH_DOWN);
                                            if (evt.touch.action != TGUI_TOUCH_DOWN)
                                                wd.MainPointer = -1;
                                        }
                                    }
                                }
                                break;
                            case TGUI_TOUCH_MOVE:
                                for (uint32_t i = 0; i < evt.touch.events; ++i)
                                {
                                    for (uint32_t j = 0; j < evt.touch.num_pointers; ++j)
                                    {
                                        const auto& pt = evt.touch.pointers[i][j];
                                        if (pt.id == wd.MainPointer) // only check first 
                                        {
                                            event::Position pos(pt.x, pt.y);
                                            if (pos != wd.LastPos)
                                                host->OnMouseDrag(pos);
                                        }
                                    }
                                }
                                break;
                            default:
                                break;
                            }
                        }
                        break;
                    case TGUI_EVENT_SURFACE_CHANGED:
                        if (!host) break;
                        if (auto& wd = *static_cast<WdHost*>(host); evt.surfaceChanged.id == wd.View)
                        {
                            float pw = 0.f, ph = 0.f, iw = 0.f, ih = 0.f;
                            LOG_TERR(get_dimensions(Connection, wd.Activity, wd.View, TGUI_UNIT_PX, &pw, &ph), Logger, u"Can't get pixel dims");
                            LOG_TERR(get_dimensions(Connection, wd.Activity, wd.View, TGUI_UNIT_IN, &iw, &ih), Logger, u"Can't get inch dims");
                            host->OnResize(static_cast<int32_t>(evt.surfaceChanged.width), static_cast<int32_t>(evt.surfaceChanged.height));
                            if (pw > 0 && ph > 0 && iw > 0 && ih > 0)
                            {
                                const auto dw = pw / iw, dh = ph / ih;
                                if (std::abs(dw - wd.DPIs.first) > 0.1f || std::abs(dh - wd.DPIs.second) > 0.1f)
                                {
                                    wd.DPIs = { dw, dh };
                                    host->OnDPIChange(dw, dh);
                                }
                            }
                        }
                        break;
                    case TGUI_EVENT_FOCUS:
                        Logger.Verbose(u"=>focus [{}]\n", evt.activity);
                        break;
                    case TGUI_EVENT_FRAME_COMPLETE: // vsync like?
                        break;
                    case TGUI_EVENT_INSET:
                        Logger.Verbose(u"=>activity inset [{:08b}] for [{}]\n", enum_cast(evt.inset.current_inset), evt.activity);
                        break;
                    case TGUI_EVENT_BACK:
                        if (host && host->OnClose())
                            CloseWindow(host);
                        else if (evt.activity == Picker.Activity && !Picker.TaskList.empty())
                        {
                            const auto& task = Picker.TaskList.front();
                            FilePickRefresh(task.CurPath.parent_path());
                        }
                        break;
                    case TGUI_EVENT_CLICK:
                        if (evt.activity == Picker.Activity && !Picker.TaskList.empty())
                            FilePickClick(evt.click.id, evt.click.set);
                        break;
                    case TGUI_EVENT_ITEM_SELECTED:
                        if (evt.activity == Picker.Activity && evt.itemSelected.id == Picker.SpinFilter)
                        {
                            if (!Picker.TaskList.empty())
                            {
                                auto& task = Picker.TaskList.front();
                                const auto idx = static_cast<uint32_t>(evt.itemSelected.selected);
                                if (task.Info.ExtensionFilters.size() > idx && idx != task.FilterIdx)
                                {
                                    task.FilterIdx = idx;
                                    FilePickRefresh(task.CurPath);
                                }
                            }
                        }
                        break;
                    case TGUI_EVENT_WEBVIEW_NAVIGATION:
                        Logger.Verbose(u"=>webview navigate [{}]\n", evt.activity);
                        break;
                    case TGUI_EVENT_WEBVIEW_HTTP_ERROR:
                        Logger.Verbose(u"=>webview http error [{}]\n", evt.activity);
                        break;
                    case TGUI_EVENT_WEBVIEW_ERROR:
                        Logger.Verbose(u"=>webview error [{}]\n", evt.activity);
                        break;
                    case TGUI_EVENT_WEBVIEW_DESTROYED:
                        Logger.Verbose(u"=>webview destroyed [{}]\n", evt.activity);
                        break;
                    case TGUI_EVENT_WEBVIEW_PROGRESS:
                        Logger.Verbose(u"=>webview progress [{}]\n", evt.activity);
                        break;
                    case TGUI_EVENT_WEBVIEW_CONSOLE:
                        Logger.Info(u"[webview]{}\n", evt.webConsole.msg);
                        break;
                    default:
                        Logger.Verbose(u"=>event [{}]@[{}]\n", enum_cast(evt.type), evt.activity);
                        break;
                    }
                    event_destroy(&evt);
                }
            }
            Msg msg{};
            while (MsgQueue.pop(msg))
            {
                hasProcess = true;
                switch (msg.OpCode)
                {
                case MessageStop:
                    shouldContinue = false;
                    break;
                case MessageTask:
                    HandleTask();
                    break;
                case MessageCreate:
                    CreateNewWindow_(*reinterpret_cast<CreatePayload*>(msg.Ptr));
                    break;
                case MessageUpdTitle:
                case MessageUpdIcon:
                    UpdateTileIcon(*msg.Host);
                    break;
                case MessageFilePick:
                    if (Picker.Activity == -1)
                    {
                        LOG_TERR(activity_create(Connection, &Picker.Activity, TGUI_ACTIVITY_NORMAL, nullptr, true), Logger, u"Can't create picker activity");
                    }
                    break;
                default:
                    break;
                }
            }
            if (char* logstr = nullptr; TGUI_ERR_OK == get_log(Connection, true, &logstr))
            {
                std::string_view logs(logstr);
                Logger.Verbose(u"Plugin:\n{}\n", logs);
                free(logstr);
            }
            // no event now
            if (!hasProcess)
            {
                timer.Stop(); // get time since last pause
                if (const auto us = timer.ElapseUs(); us < 10000) // less than 10ms
                    std::this_thread::sleep_for(std::chrono::milliseconds((10000 - us) / 1000));
                timer.Start(); // restart timer
            }
        }

    }
    void OnTerminate() noexcept final
    {
    }
    bool RequestStop() noexcept final
    {
        SendControlMessage(nullptr, MessageStop);
        return true;
    }


    void NotifyTask() noexcept final
    {
        SendControlMessage(nullptr, MessageTask);
    }

    bool CheckCapsLock() const noexcept final
    {
        return IsCapsLock;
    }

    inline static const tgui_view_size WrapContent = { .type = TGUI_VIEW_WRAP_CONTENT, .value = { .unit = TGUI_UNIT_DP, .value = 0.f } };
    void FilePickRefresh(const common::fs::path& newPath)
    {
        auto& task = Picker.TaskList.front();
        try
        {
            task.UpdateList(Logger, newPath);
        }
        catch (const std::exception& e)
        {
            Logger.Warning(u"When UpdateList: {}\n", e.what());
            return;
        }
        Picker.CurItems.clear();
        delete_children(Connection, Picker.Activity, Picker.ViewList);
        set_text(Connection, Picker.Activity, Picker.TxtPath, reinterpret_cast<const char*>(task.CurPath.u8string().c_str()));
        uint32_t idx = 0;
        for (auto& item : task.CurItems)
        {
            tgui_view view = -1;
            const auto name = item.Path.filename().u8string();
            const auto txt = reinterpret_cast<const char*>(name.c_str());
            if (!item.IsFolder && task.Info.AllowMultiSelect)
            {
                create_checkbox(Connection, Picker.Activity, &view, &Picker.ViewList, TGUI_VIS_VISIBLE, txt, false);
            }
            else
            {
                create_text_view(Connection, Picker.Activity, &view, &Picker.ViewList, TGUI_VIS_VISIBLE, txt, false, false);
                set_clickable(Connection, Picker.Activity, view, true);
                send_click_event(Connection, Picker.Activity, view, true);
                set_margin(Connection, Picker.Activity, view, { .unit = TGUI_UNIT_DP, .value = 6.f }, TGUI_DIR_ALL);
            }
            set_height(Connection, Picker.Activity, view, WrapContent);
            linear_params(Connection, Picker.Activity, view, 0.f, 0);
            Picker.CurItems.insert_or_assign(view, idx++);
        }
    }
    void FilePickClick(int32_t id, bool isSet)
    {
        auto& task = Picker.TaskList.front();
        bool isCancel = false;
        if (id == Picker.BtnCancel)
        {
            isCancel = true;
        }
        else if (id == Picker.BtnOK)
        {
        }
        else
        {
            const auto it = Picker.CurItems.find(id);
            if (it == Picker.CurItems.end())
            {
                Logger.Verbose(u"click unknown [{}].\n", id);
                return;
            }
            IF_UNLIKELY(task.CurItems.size() <= it->second)
            {
                //Logger.Verbose(u"mapped [{}] exceed [{}].\n", it->second, task.CurItems.size());
                return;
            }
            auto& item = task.CurItems[it->second];
            if (item.IsFolder)
            {
                //Logger.Verbose(u"choose folder [{}].\n", item.Path.filename().string());
                FilePickRefresh(item.Path);
                return;
            }
            IF_UNLIKELY(task.Info.IsFolder)
            {
                Logger.Verbose(u"click a file when select folder, unexpected.\n");
                return;
            }
            if (task.Info.AllowMultiSelect)
            {
                item.IsSelected = isSet;
                //Logger.Verbose(u"toggle [{}] to [{}].\n", item.Path.filename().string(), isSet);
                return;
            }
            Logger.Verbose(u"single select [{}].\n", item.Path.filename().string());
            item.IsSelected = true;
        }
        FileList ret;
        if (!isCancel)
        {
            if (task.Info.IsFolder)
            {
                ret.AppendFile(task.CurPath.u16string());
            }
            else
            {
                for (const auto& item : task.CurItems)
                {
                    if (item.IsSelected)
                    {
                        ret.AppendFile(item.Path.u16string());
                    }
                }
            }
        }
        task.Pms.set_value(std::move(ret));
        // cleanup
        Picker.CurItems.clear();
        delete_children(Connection, Picker.Activity, Picker.ViewList);
        delete_children(Connection, Picker.Activity, Picker.ViewBtns);
        delete_children(Connection, Picker.Activity, Picker.View);
        delete_view(Connection, Picker.Activity, Picker.View);
        activity_finish(Connection, Picker.Activity);
        Picker.View = -1;
        Picker.Activity = -1;
        FilePicker::Task* nextTask = nullptr;
        {
            std::unique_lock<std::mutex> lock(Picker.MainMutex);
            Picker.TaskList.pop_front();
            if (!Picker.TaskList.empty())
                nextTask = &Picker.TaskList.front();
        }
        if (nextTask)
            LOG_TERR(activity_create(Connection, &Picker.Activity, TGUI_ACTIVITY_NORMAL, nullptr, true), Logger, u"Can't create picker activity");
    }
    void FilePickInit()
    {
        const auto& task = Picker.TaskList.front();

        const auto title = common::str::to_string(task.Info.Title, common::str::Encoding::UTF8);
        activity_set_task_description(Connection, Picker.Activity, nullptr, 0, title.c_str());
        create_linear_layout(Connection, Picker.Activity, &Picker.View, nullptr, TGUI_VIS_VISIBLE, false);
        set_margin(Connection, Picker.Activity, Picker.View, { .unit = TGUI_UNIT_DP, .value = 4.f }, TGUI_DIR_ALL);

        create_text_view(Connection, Picker.Activity, &Picker.TxtTitle, &Picker.View, TGUI_VIS_VISIBLE, title.c_str(), false, false);
        text_size(Connection, Picker.Activity, Picker.TxtTitle, { .unit = TGUI_UNIT_SP, .value = 5.f });
        set_margin(Connection, Picker.Activity, Picker.TxtTitle, { .unit = TGUI_UNIT_DP, .value = 6.f }, TGUI_DIR_ALL);
        set_height(Connection, Picker.Activity, Picker.TxtTitle, WrapContent);
        linear_params(Connection, Picker.Activity, Picker.TxtTitle, 0.f, 0);

        create_text_view(Connection, Picker.Activity, &Picker.TxtPath, &Picker.View, TGUI_VIS_VISIBLE, "current at:", false, false);
        set_margin(Connection, Picker.Activity, Picker.TxtPath, { .unit = TGUI_UNIT_DP, .value = 6.f }, TGUI_DIR_BOTTOM);
        set_height(Connection, Picker.Activity, Picker.TxtPath, WrapContent);
        linear_params(Connection, Picker.Activity, Picker.TxtPath, 0.f, 0);

        create_nested_scroll_view(Connection, Picker.Activity, &Picker.ViewListHolder, &Picker.View, TGUI_VIS_VISIBLE, false, true, true);
        linear_params(Connection, Picker.Activity, Picker.ViewListHolder, 1.f, 0);
        create_linear_layout(Connection, Picker.Activity, &Picker.ViewList, &Picker.ViewListHolder, TGUI_VIS_VISIBLE, false);

        create_linear_layout(Connection, Picker.Activity, &Picker.ViewBtns, &Picker.View, TGUI_VIS_VISIBLE, true);
        set_margin(Connection, Picker.Activity, Picker.ViewBtns, { .unit = TGUI_UNIT_DP, .value = 6.f }, TGUI_DIR_ALL);
        set_height(Connection, Picker.Activity, Picker.ViewBtns, WrapContent);
        linear_params(Connection, Picker.Activity, Picker.ViewBtns, 0.f, 0);

        const auto useFilter = !task.Info.ExtensionFilters.empty();
        create_spinner(Connection, Picker.Activity, &Picker.SpinFilter, &Picker.ViewBtns, useFilter ? TGUI_VIS_VISIBLE : TGUI_VIS_GONE);
        if (useFilter)
        {
            const auto options = common::linq::FromIterable(task.Info.ExtensionFilters)
                .Select([](const auto& item) { return common::str::to_u8string(item.first); })
                .ToVector();
            const auto ptrs = common::linq::FromIterable(options).Select([](const auto& txt) { return reinterpret_cast<const char*>(txt.c_str()); }).ToVector();
            set_list(Connection, Picker.Activity, Picker.SpinFilter, ptrs.data());
            select_item(Connection, Picker.Activity, Picker.SpinFilter, 0);
        }
        set_height(Connection, Picker.Activity, Picker.SpinFilter, WrapContent);
        linear_params(Connection, Picker.Activity, Picker.SpinFilter, 1.f, 0);

        create_button(Connection, Picker.Activity, &Picker.BtnOK, &Picker.ViewBtns, TGUI_VIS_VISIBLE, "OK");
        set_width(Connection, Picker.Activity, Picker.BtnOK, WrapContent);
        set_height(Connection, Picker.Activity, Picker.BtnOK, WrapContent);
        linear_params(Connection, Picker.Activity, Picker.BtnOK, 0.f, 0);

        create_button(Connection, Picker.Activity, &Picker.BtnCancel, &Picker.ViewBtns, TGUI_VIS_VISIBLE, "Cancel");
        set_width(Connection, Picker.Activity, Picker.BtnCancel, WrapContent);
        set_height(Connection, Picker.Activity, Picker.BtnCancel, WrapContent);
        linear_params(Connection, Picker.Activity, Picker.BtnCancel, 0.f, 0);

        FilePickRefresh(task.CurPath);
    }

    void CreateNewWindow(CreatePayload& payload) final
    {
        SendControlMessage(static_cast<WdHost*>(payload.Host), MessageCreate, 0, &payload);
        payload.Promise.get_future().get();
    }
    void CreateNewWindow_(CreatePayload& payload)
    {
        auto& host = *static_cast<WdHost*>(payload.Host);
        host.NeedCheckDrag = false;
        THROW_TERR(activity_create(Connection, &host.Activity, TGUI_ACTIVITY_NORMAL, nullptr, true), u"Can't create termux-gui activity");
        Logger.Verbose(u"ask create [{}]\n", host.Activity);
        payload.Promise.set_value(); // can release now
        RegisterHost(host.Activity, &host);
    }
    void UpdateTileIcon(WdHost& host) const
    {
        TitleLock lockT(&host);
        IconLock lockI(&host);
        if (lockT || lockI)
        {
            const auto& icon = *GetWindowResource(&host, WdAttrIndex::Icon);
            std::vector<std::byte>* data = icon ? &static_cast<const IconHolder&>(icon).Data() : nullptr;
            const auto title = common::str::to_string(host.Title, common::str::Encoding::UTF8);
            activity_set_task_description(Connection, host.Activity, data ? data->data() : nullptr, data ? data->size() : 0, title.c_str());
        }
    }
    void UpdateTitle(WindowHost_* host_) const final
    {
        SendControlMessage(static_cast<WdHost*>(host_), MessageUpdTitle);
    }
    void UpdateIcon(WindowHost_* host_) const final
    {
        SendControlMessage(static_cast<WdHost*>(host_), MessageUpdIcon);
    }
    void CloseWindow(WindowHost_* host) const final
    {
        auto& wd = *static_cast<WdHost*>(host);
        activity_finish(Connection, wd.Activity);
    }
    void ReleaseWindow(WindowHost_* host) final
    {
        auto& wd = *static_cast<WdHost*>(host);
        delete_view(Connection, wd.Activity, wd.View);
        UnregisterHost(host);
    }

    OpaqueResource PrepareIcon(WindowHost_&, ImageView img) const noexcept final
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

        auto data = xziar::img::WriteImage<std::byte>(img, u"PNG");
        if (!data.empty())
            return static_cast<OpaqueResource>(IconHolder(std::make_unique<std::vector<std::byte>>(std::move(data))));
        return {};
    }

    WindowHost Create(const CreateInfo& info_) final
    {
        TermuxGUICreateInfo info{};
        static_cast<CreateInfo&>(info) = info_;
        return Create(info);
    }
    std::shared_ptr<TermuxGUIWdHost> Create(const TermuxGUICreateInfo& info) final
    {
        return std::make_shared<WdHost>(*this, info);
    }

    common::PromiseResult<FileList> OpenFilePicker(const FilePickerInfo& info) noexcept final
    {
        std::unique_lock<std::mutex> lock(Picker.MainMutex);
        auto& task = Picker.TaskList.emplace_back(info);
        SendControlMessage(nullptr, MessageFilePick);
        return common::PromiseResultSTD<FileList>::Get(task.Pms.get_future());
    }

    static inline const auto Dummy = RegisterBackend<WindowManagerTermuxGUI>();

};


void WindowManagerTermuxGUI::CPURenderer::Render(bool forceRedraw) noexcept
{
    const auto [sizeChanged, needInit] = Update(TGUI_HARDWARE_BUFFER_CPU_OFTEN);
    bool needDraw = false;
    {
        auto imgLock = UseImg();
        const bool imgChanged = imgLock;
        needDraw = forceRedraw || needInit || imgChanged || (sizeChanged && AttachedImage);
        if (forceRedraw || imgChanged || sizeChanged) // need to be larger than needDraw to ensure buffer is correct
        {
            auto& backBuf = Back();
            auto& manager = Window.GetManager();
            SimpleTimer timer;
            timer.Start();
            void* bufptr = nullptr;
            const auto bufStride = backBuf.RealWidth * BackBufType.ElementSize();
            if (manager.AHB_lockAndGetInfo)
            {
                int32_t bpp = 0, stride_ = 0;
                if (const auto ret = manager.AHB_lockAndGetInfo(backBuf.HWBuf.buffer, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, nullptr, &bufptr, &bpp, &stride_); ret)
                    Log().Error(FmtString(u"Failed to lock AHB : {:#}.\n"), common::ErrnoHolder(-ret));
                Ensures(bpp > 0 && static_cast<uint32_t>(bpp) == BackBufType.ElementSize());
                Ensures(stride_ > 0 && static_cast<uint32_t>(stride_) == bufStride);
            }
            else
            {
                if (const auto ret = manager.AHB_lock(backBuf.HWBuf.buffer, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, nullptr, &bufptr); ret)
                    Log().Error(FmtString(u"Failed to lock AHB : {:#}.\n"), common::ErrnoHolder(-ret));
            }
            if (AttachedImage)
            {
                const auto bufSize = static_cast<size_t>(bufStride) * backBuf.Shape.Height;
                memset(bufptr, 0, bufSize); // clear first
                auto backImg = Image::CreateViewFromTemp({ reinterpret_cast<std::byte*>(bufptr), bufSize }, BackBufType, backBuf.RealWidth, backBuf.Shape.Height);
                const auto [tw, th] = backBuf.Shape.ResizeWithin(AttachedImage->GetWidth(), AttachedImage->GetHeight());
                // back buffer and img both are flipped
                Ensures(backImg.GetHeight() >= static_cast<uint32_t>(th));
                AttachedImage->ResizeTo(backImg, 0, 0, 0, backImg.GetHeight() - th, tw, th, true, true);
            }
            else
            {
                FillBufferColorXXXA(reinterpret_cast<uint32_t*>(bufptr), backBuf.Shape.Width, backBuf.Shape.Height, backBuf.RealWidth, BackBufIdx);
            }
            if (const auto ret = manager.AHB_unlock(backBuf.HWBuf.buffer, nullptr); ret)
                Log().Error(FmtString(u"Failed to unlock AHB : {:#}.\n"), common::ErrnoHolder(-ret));
            timer.Stop();
            Log().Verbose(FmtString(u"Window[{:x}] rebuild backbuffer[{}] in [{} ms].\n"), Window, BackBufIdx, timer.ElapseMs<true>());
        }
        else
            Ensures(!needDraw);
    }
    if (needDraw)
    {
        Draw();
    }
}


}

#endif
