
#if defined(__has_include) && __has_include(<wayland-client-protocol.h>) && __has_include(<libdecor-0/libdecor.h>)

#include "WindowManager.h"
#include "WindowHost.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/SharedMemory.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/DynamicLibrary.h"
#include "SystemCommon/ErrorCodeHelper.h"
#include "common/ContainerEx.hpp"
#include "common/StaticLookup.hpp"
#include "common/StringEx.hpp"
#include "common/StringPool.hpp"
#include "common/StringLinq.hpp"
#include "common/TrunckedContainer.hpp"
#include "common/MemoryStream.hpp"
#include "common/Linq2.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/lockfree/queue.hpp>
//#include <boost/preprocessor/seq/filter.hpp>
//#include <boost/preprocessor/seq/fold_left.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
//#include <boost/preprocessor/seq/transform.hpp>
//#include <boost/preprocessor/selection/max.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/vmd/is_number.hpp>


#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>
#include <wayland-util.h>
#pragma message("Compiling WindowHost with wayland[" WAYLAND_VERSION "]")
#include "wayland-ext/zxdg-decoration-client-protocol.h"
#include "wayland-ext/xdg-toplevel-icon-client-protocol.h"
#include "wayland-ext/xdg-shell-client-protocol.h"

#include <libdecor-0/libdecor.h>
#include <xkbcommon/xkbcommon.h>

#include <deque>
#include <mutex>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <linux/input.h>


#if COMMON_COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunused-const-variable"
#endif

constexpr uint32_t MessageCreate    = 1;
constexpr uint32_t MessageTask      = 2;
constexpr uint32_t MessageClose     = 3;
constexpr uint32_t MessageStop      = 4;
constexpr uint32_t MessageDpi       = 5;
constexpr uint32_t MessageUpdTitle  = 10;
constexpr uint32_t MessageUpdIcon   = 11;
constexpr uint32_t MessageClipboard = 12;

constexpr uint32_t FdCookieComplex      = 0x80000000u;
constexpr uint32_t FdCookieClipboard    = 1;
constexpr uint32_t FdCookieDrag         = 2;

#if COMMON_COMPILER_CLANG
#   pragma clang diagnostic pop
#endif


namespace hack
{
#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wattributes"
#endif
#include "wayland-ext/zxdg-decoration-protocol.inl"
#include "wayland-ext/xdg-toplevel-icon-protocol.inl"
#include "wayland-ext/xdg-shell-protocol.inl"
#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic pop
#endif
const wl_interface wl_output_interface{};
const wl_interface wl_seat_interface{};
const wl_interface wl_surface_interface{};
const wl_interface wl_buffer_interface{};
}



namespace xziar::gui::detail
{
using namespace std::string_view_literals;
using xziar::gui::event::CommonKeys;
using xziar::img::Image;
using xziar::img::ImageView;
using xziar::img::ImgDType;
using common::BaseException;

static constexpr auto ShmFormatLookup = BuildStaticLookup(uint32_t, ImgDType,
    { WL_SHM_FORMAT_ARGB8888,       xziar::img::ImageDataType::BGRA   },
    { WL_SHM_FORMAT_XRGB8888,       xziar::img::ImageDataType::BGRA   },
    { WL_SHM_FORMAT_RGB888,         xziar::img::ImageDataType::BGR    },
    { WL_SHM_FORMAT_BGR888,         xziar::img::ImageDataType::RGB    },
    { WL_SHM_FORMAT_ABGR8888,       xziar::img::ImageDataType::RGBA   },
    { WL_SHM_FORMAT_XBGR8888,       xziar::img::ImageDataType::RGBA   },
    { WL_SHM_FORMAT_R8,             xziar::img::ImageDataType::GRAY   },
    { WL_SHM_FORMAT_R16,            xziar::img::ImageDataType::GRAY16 },
    { WL_SHM_FORMAT_ARGB16161616F,  xziar::img::ImageDataType::BGRAh  },
    { WL_SHM_FORMAT_XRGB16161616F,  xziar::img::ImageDataType::BGRAh  },
    { WL_SHM_FORMAT_ABGR16161616F,  xziar::img::ImageDataType::RGBAh  },
    { WL_SHM_FORMAT_XBGR16161616F,  xziar::img::ImageDataType::RGBAh  },
    { WL_SHM_FORMAT_ARGB16161616,   xziar::img::ImageDataType::BGRA16 },
    { WL_SHM_FORMAT_XRGB16161616,   xziar::img::ImageDataType::BGRA16 },
    { WL_SHM_FORMAT_ABGR16161616,   xziar::img::ImageDataType::RGBA16 },
    { WL_SHM_FORMAT_XBGR16161616,   xziar::img::ImageDataType::RGBA16 }
);


#define EnumStrEntry_(en, str) if constexpr ( requires { T::en; } ) { dst[idx++] = { T::en, STRINGIZE(str) }; }
#define EnumStrEntry(r, prefix, tp) EnumStrEntry_(BOOST_PP_CAT(prefix, BOOST_PP_TUPLE_ELEM(2, 0, tp)), BOOST_PP_TUPLE_ELEM(2, 1, tp))
#define EnumStrMap(type, prefix, ...)[]()                                                           \
{                                                                                                   \
    constexpr auto ret = []([[maybe_unused]]auto dummy)                                             \
    {                                                                                               \
        using T = decltype(dummy);                                                                  \
        std::array<std::pair<type, std::string_view>, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)> dst{};   \
        uint32_t idx = 0;                                                                           \
        BOOST_PP_SEQ_FOR_EACH(EnumStrEntry, prefix, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))          \
    return std::pair{dst, idx};                                                                     \
    }(type{});                                                                                      \
    return common::detail::BuildTableStoreFrom<type, std::string_view, ret.second>(ret.first);      \
}()

static constexpr auto XdgWmCapNames = EnumStrMap(xdg_toplevel_wm_capabilities, XDG_TOPLEVEL_WM_CAPABILITIES_,
    (WINDOW_MENU,   WindowMenu),
    (MAXIMIZE,      Maximize),
    (FULLSCREEN,    FullScreen),
    (MINIMIZE,      Minimize)
);
static constexpr auto XdgTopLvStateNames = EnumStrMap(xdg_toplevel_state, XDG_TOPLEVEL_STATE_,
    (MAXIMIZED,     Maximized),
    (FULLSCREEN,    FullScreen),
    (RESIZING,      Resizing),
    (ACTIVATED,     Activated),
    (TILED_LEFT,    TiledLeft),
    (TILED_RIGHT,   TiledRight),
    (TILED_TOP,     TiledTop),
    (TILED_BOTTOM,  TiledBottom),
    (SUSPENDED,     Suspended)
);
static constexpr auto DecorWindowStateNames = EnumStrMap(libdecor_window_state, LIBDECOR_WINDOW_STATE_,
    (NONE,          None),
    (ACTIVE,        Active),
    (MAXIMIZED,     Maximized),
    (FULLSCREEN,    FullScreen),
    (TILED_LEFT,    TiledLeft),
    (TILED_RIGHT,   TiledRight),
    (TILED_TOP,     TiledTop),
    (TILED_BOTTOM,  TiledBottom),
    (SUSPENDED,     Suspended)
);

#undef EnumStrMap
#undef EnumStrEntry
#undef EnumStrEntry_


struct WaylandProxy
{
    const wl_interface* Interface;
    wl_proxy* Proxy = nullptr;
    constexpr WaylandProxy(const wl_interface* iface = nullptr) noexcept : Interface(iface) {}
    explicit constexpr operator bool() const noexcept { return Proxy; }
    explicit constexpr operator const wl_interface**() noexcept { return &Interface; }
    explicit constexpr operator const wl_interface*() const noexcept { return Interface; }
    explicit constexpr operator wl_proxy*() const noexcept { return Proxy; }
    template<typename T>
    [[nodiscard]] bool operator==(const T* rhs) const noexcept 
    {
        using U = common::remove_cvref_t<T>;
        if constexpr (std::is_same_v<U, wl_interface>)
            return Interface == rhs;
        else
            return Proxy == reinterpret_cast<const wl_proxy*>(rhs); 
    }
};

class ReWriter
{
    boost::container::small_vector<std::pair<const wl_interface*, const wl_interface**>, 8> Mapper;
    std::deque<std::pair<const wl_interface*, wl_interface>> Replaced;
    common::container::TrunckedContainer<const wl_interface*> TypePool;
    common::container::TrunckedContainer<wl_message> MsgPool;
    boost::container::flat_map<const wl_interface**, common::span<const wl_interface*>> TypeTLB;
    const wl_interface* FindMapping(const wl_interface* obj) const noexcept
    {
        for (const auto& [src, dst] : Mapper)
        {
            if (src == obj)
            {
                Ensures(*dst);
                return *dst;
            }
        }
        for (const auto& [src, dst] : Replaced)
        {
            if (src == obj)
                return &dst;
        }
        return nullptr;
    }
public:
    ReWriter() noexcept : TypePool(4096 / sizeof(const wl_interface*), alignof(const wl_interface*)),
        MsgPool(4096 / sizeof(wl_message), alignof(wl_message))
    {}
    void PlaceMapper(const wl_interface* src, const wl_interface** dst) noexcept
    {
        Mapper.emplace_back(src, dst);
    }
    const wl_message* ReplaceMessage(const wl_message* src, uint32_t count)
    {
        if (!src || !count)
            return nullptr;
        const auto space = MsgPool.Alloc(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            space[i].name = src[i].name;
            space[i].signature = src[i].signature;
            auto it = TypeTLB.upper_bound(src[i].types);
            Ensures(it != TypeTLB.begin());
            --it;
            Ensures(it != TypeTLB.end());
            const auto pool = it->second;
            const auto diff = src[i].types - it->first;
            Ensures(diff >= 0 && static_cast<size_t>(diff) < pool.size());
            space[i].types = pool.data() + diff;
        }
        return space.data();
    }
    template<size_t N>
    void ReplaceTypes(const wl_interface*(&src)[N])
    {
        if (const auto it = TypeTLB.find(src); it != TypeTLB.end())
        {
            Ensures(it->second.size() == N);
            return;
        }
        const auto space = TypePool.Alloc(N);
        TypeTLB.insert_or_assign(src, space);
        for (size_t i = 0; i < N; ++i)
            space[i] = Replace(src[i]);
    }
    const wl_interface* Replace(const wl_interface* iface)
    {
        if (iface == nullptr)
            return nullptr;
        if (const auto mapped = FindMapping(iface); mapped)
            return mapped;
        wl_interface& newface = Replaced.emplace_back(iface, *iface).second;
        newface.methods = ReplaceMessage(iface->methods, iface->method_count);
        newface.events  = ReplaceMessage(iface->events,  iface->event_count );
        return &newface;
    }
};


struct ImageBuffer
{
    WaylandProxy ShmPool;
    WaylandProxy Buf;
    common::span<std::byte> Space;
    Image RealImage;
    int Fd = -1;
    ImageBuffer()
    {
        Fd = common::SharedMemory::CreateMemFd("wayland_buf", MFD_CLOEXEC | MFD_HUGETLB);
        if (Fd == -1)
        {
            //manager.Logger.Warning(u"failed for memfd_create: {}", common::ErrnoHolder::GetCurError());
            Fd = common::SharedMemory::CreateMemFd("wayland_buf", MFD_CLOEXEC);
            Ensures(Fd != -1);
        }
    }
    ~ImageBuffer()
    {
        close(Fd);
    }
    void ReleaseObjects(const WindowManagerWayland& manager);
    void PrepareForSize(const WindowManagerWayland& manager, int32_t w, int32_t h);
};
struct IconData : public ImageBuffer
{
    WaylandProxy Icon;
};
struct IconHolder : public OpaqueResource
{
    explicit IconHolder(const WindowManagerWayland& manager, std::unique_ptr<IconData>&& data) noexcept : 
        OpaqueResource(&TheDisposer, reinterpret_cast<uintptr_t>(&manager), reinterpret_cast<uintptr_t>(data.release()))
    { }
    IconData& Data() const noexcept { return *reinterpret_cast<IconData*>(Cookie[1]); }
    std::unique_ptr<IconData> Extract() noexcept
    {
        std::unique_ptr<IconData> ret(&Data());
        Disposer = nullptr;
        return ret;
    }
    static void TheDisposer(const OpaqueResource& res) noexcept;
};
struct ImgHolder : public OpaqueResource
{
    explicit ImgHolder(const ImageView& img) noexcept : OpaqueResource(&TheDisposer, reinterpret_cast<uintptr_t>(new ImageView(img))) {}
    const ImageView& Image() const noexcept { return *reinterpret_cast<const ImageView*>(Cookie[0]); }
    static void TheDisposer(const OpaqueResource& res) noexcept
    {
        const auto& holder = static_cast<const ImgHolder&>(res);
        delete &holder.Image();
    }
};


#define ItemPair(str, type) { str ""sv, ClipBoardTypes::type }
static constexpr auto MIMEMappings = BuildStaticLookup(std::string_view, ClipBoardTypes,
    ItemPair("text/plain",      Text),
    ItemPair("text/uri-list",   File),
    ItemPair("image/png",       Image),
    ItemPair("image/avif",      Image),
    ItemPair("image/bmp",       Image),
    ItemPair("image/heic",      Image),
    ItemPair("image/heif",      Image),
    ItemPair("image/jpeg",      Image),
    ItemPair("image/jpg",       Image),
    ItemPair("image/jxl",       Image),
    ItemPair("image/tga",       Image),
    ItemPair("image/tif",       Image),
    ItemPair("image/tiff",      Image),
    ItemPair("image/webp",      Image)
);

#undef ItemPair
struct OfferHolder
{
    struct Item
    {
        common::StringPiece<char> TypeStr;
        ClipBoardTypes Category;
    };
    wl_data_offer* DataOffer = nullptr;
    common::StringPool<char> Pool;
    std::vector<Item> Types;
    std::vector<std::byte> Data;
    int Fd[2] = {-1, -1};
    void Clear()
    {
        Types.clear();
        Pool.Clear();
    }
    void Add(std::string_view txt)
    {
        if (const auto type = MIMEMappings(txt); type)
        {
            const auto str = Pool.AllocateConcatString(txt, "\0");
            Types.push_back({str, *type});
        }
    }
    std::string_view Get(size_t idx) const noexcept
    {
        return Pool.GetStringView(Types[idx].TypeStr);
    }
    enum class States { More, Faulted, Finished };
    States PerformRead(common::mlog::MiniLogger<false>& logger, uint32_t typeIdx)
    {
        const auto mime = Get(typeIdx);
        std::byte tmp[4096];
        while (true)
        {
            const auto size = read(Fd[0], tmp, sizeof(tmp));
            if (size > 0)
            {
                Data.insert(Data.end(), tmp, tmp + size);
            }
            else if (size == 0) // closed
            {
                logger.Verbose(u"DataOffer type[{}]({}) readed [{}]bytes.\n", typeIdx, mime, Data.size());
                return States::Finished;
            }
            else if (size == -1)
            {
                const auto err = common::ErrnoHolder::GetCurError();
                if (err.Value == EAGAIN) // fully readed, wait for next signal
                {
                    logger.Verbose(u"DataOffer type[{}]({}) wait for next signal, cur [{}]bytes.\n", typeIdx, mime, Data.size());
                    return States::More;
                }
                logger.Verbose(u"DataOffer type[{}]({}) read error: [{}].\n", typeIdx, mime, err);
                Data.clear();
                return States::Faulted;
            }
        }
    }
};


struct ClipboardTask
{
    WindowHost_& Host;
    std::function<bool(ClipBoardTypes, const std::any&)> Handler;
    uint32_t TypeIndex = 0;
    ClipBoardTypes SupportTypes;
    ClipboardTask(WindowHost_& host, const std::function<bool(ClipBoardTypes, const std::any&)>& handler, ClipBoardTypes types) noexcept :
        Host(host), Handler(handler),  SupportTypes(types) {}
    ~ClipboardTask() { }
};

struct ClipboardInfo
{
    std::mutex Mutex;
    std::deque<ClipboardTask> Tasks;
    OfferHolder* Offer = nullptr;
    ClipboardTask* GetFront() noexcept
    {
        std::unique_lock<std::mutex> lock(Mutex);
        return Tasks.empty() ? nullptr : &Tasks.front();
    }
};
struct DragInfo
{
    OfferHolder* Offer = nullptr;
    WindowHost_* Target = nullptr;
    int32_t PosX = 0, PosY = 0;
    uint32_t TypeIndex = 0;
    bool InDrop = false;
};


template<typename T, auto Func>
inline constexpr auto WrapWayland() noexcept
{
    return [](void* data, auto... args)
    {
        (reinterpret_cast<T*>(data)->*Func)(args...);
    };
}
#define WaylandCB__(clz, func) BOOST_PP_IIF(BOOST_PP_IS_EMPTY(func), [](auto...){}, (WrapWayland<clz, &clz::func>()))
#define WaylandCB_(clz, name, func) if constexpr (requires { dst.name; }) { dst.name = WaylandCB__(clz, func); }
#define WaylandCB(r, clz, tp) WaylandCB_(clz, BOOST_PP_TUPLE_ELEM(2, 0, tp), BOOST_PP_TUPLE_ELEM(2, 1, tp))
#define WaylandListener(type, bind, name, ...) static inline constexpr type##_listener Listener##name = \
    [](auto dst){ BOOST_PP_SEQ_FOR_EACH(WaylandCB, bind, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) return dst; }(type##_listener{})


//#define ReduceToMax(seq) BOOST_PP_SEQ_FOLD_LEFT(BOOST_PP_MAX_D, BOOST_PP_SEQ_HEAD(seq), BOOST_PP_SEQ_TAIL(seq))
//#define IsVerDefined(s, _, x) BOOST_VMD_IS_NUMBER(x)
//#define VerName(r, prefix, x) BOOST_PP_CAT(prefix, BOOST_PP_CAT(x, _SINCE_VERSION))
//#define DefineVersionMax(prefix, ...) ReduceToMax(BOOST_PP_SEQ_FILTER(IsVerDefined, _, BOOST_PP_SEQ_TRANSFORM(VerName, prefix, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))))
//#define WLVER_Keybaord DefineVersionMax(WL_KEYBOARD_, KEYMAP, MODIFIERS, REPEAT_INFO, XD)
//#pragma message("Header protocal version: wl_keyboard[" STRINGIZE(WLVER_Keybaord) "]")

struct Version
{
#define UpdateMax(x) BOOST_PP_IIF(BOOST_VMD_IS_NUMBER(x), dst = std::max(dst, x);, )
#define EachUpdate(r, prefix, x) UpdateMax(BOOST_PP_CAT(prefix, BOOST_PP_CAT(x, _SINCE_VERSION)))
#define DefineVer(name, minver, prefix, ...) static inline constexpr int name##Min = minver;\
static inline constexpr int name = [](){ int dst = 0; BOOST_PP_SEQ_FOR_EACH(EachUpdate, prefix, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) return dst; }();\
static_assert(name >= name##Min)
#define DefineVerSet(name, minver, cur) static inline constexpr int name##Min = minver; static inline constexpr int name = cur; static_assert(name >= name##Min)

DefineVer(Surface, 4, WL_SURFACE_, ATTACH, SET_BUFFER_TRANSFORM, SET_BUFFER_SCALE, DAMAGE_BUFFER, OFFSET, PREFERRED_BUFFER_SCALE);
DefineVerSet(DataDev, 3, 3);
DefineVer(DataOffer, 3, WL_DATA_OFFER_, OFFER, FINISH);
DefineVer(Pointer, 5, WL_POINTER_, ENTER, RELEASE, FRAME, AXIS_VALUE120, AXIS_RELATIVE_DIRECTION);
DefineVer(Keyboard, 4, WL_KEYBOARD_, KEYMAP, RELEASE, REPEAT_INFO);

#undef DefineVerSet
#undef DefineVer
#undef EachUpdate
#undef UpdateMax
};


class WindowManagerWayland final : public WaylandBackend, public WindowManager
{
    friend ImageBuffer;
    friend IconHolder;
public:
    static constexpr std::string_view BackendName = "Wayland"sv;
private:
    class WdHost;
    struct Msg
    {
        WdHost* Host;
        void* Ptr;
        uint32_t OpCode;
        uint32_t Data;
        constexpr Msg(WdHost* host, uint32_t op, uint32_t data = 0, void* ptr = nullptr) noexcept : Host(host), Ptr(ptr), OpCode(op), Data(data) {}
        constexpr Msg() noexcept : Msg(nullptr, 0) {}
    };
    class WdHost final : public WaylandBackend::WaylandWdHost
    {
    public:
        WindowManagerWayland& GetManager() noexcept { return static_cast<WindowManagerWayland&>(WindowHost_::GetManager()); }

        void BackBufRelease(wl_buffer* buffer)
        {
            uint8_t bufIdx = UINT8_MAX;
            if (BackBuf[0].Buf == buffer)
                bufIdx = 0;
            else if (BackBuf[1].Buf == buffer)
                bufIdx = 1;
            if (bufIdx <= 1)
            {
                auto& buf = BackBuf[bufIdx];
                buf.UseMutex.unlock();
                buf.Timer.Stop();
                const auto lockTime = buf.Timer.ElapseUs();
                Manager.Logger.Verbose(u"buffer [{}] used [{}ms].\n", bufIdx, lockTime / 1000.f);
            }
        }
        WaylandListener(wl_buffer, WdHost, BackBuf, (release, BackBufRelease));

        void FrameDone(wl_callback* cb, uint32_t dat)
        {
            const auto elapse = LastFrame ? dat - LastFrame : 0;
            LastFrame = dat;
            auto& manager = GetManager();
            manager.Destroy(reinterpret_cast<wl_proxy*>(cb));
            manager.Logger.Verbose(u"frame done with [{}ms].\n", elapse);
        }
        WaylandListener(wl_callback, WdHost, Frame, (done, FrameDone), (xdy, BackBufRelease));

        void SurfaceConfig(xdg_surface* surface, uint32_t serial)
        {
            Ensures(XdgSurface == surface);
            auto& manager = GetManager();
            manager.MarshalFlags<void, XDG_SURFACE_ACK_CONFIGURE>(XdgSurface, serial); // xdg_surface_ack_configure(xdg_surface, serial);
            Invalidate();
            if (!IsRunning())
            {
                manager.Logger.Verbose(u"now initialize.\n");
                Initialize();
            }
        }
        WaylandListener(xdg_surface, WdHost, XdgSurface, (configure, SurfaceConfig));

        void ToplevelConfig(xdg_toplevel* toplevel, int32_t width, int32_t height, wl_array* states_)
        {
            Ensures(Toplevel == toplevel);

            common::span<const xdg_toplevel_state> states;
            if (states_)
                states = { reinterpret_cast<const xdg_toplevel_state*>(states_->data), states_->size / sizeof(xdg_toplevel_state) };
            std::string stateStr;
            for (const auto state : states)
            {
                if (!stateStr.empty()) stateStr.append(", ");
                if (const auto name = XdgTopLvStateNames(state); name)
                    stateStr.append(*name);
                else
                    stateStr.append(std::to_string(common::enum_cast(state)));
                //if (state == xdg_toplevel_state::XDG_TOPLEVEL_STATE_ACTIVATED)
                //    hasActivate = true;
            }

            std::optional<RectBase<int32_t>> newSize(std::in_place, width, height);
            const RectBase<int32_t> curSize(*this);
            Manager.Logger.Verbose(u"toplevel conf size[{}] real[{}] state[{}].\n", *newSize, curSize, stateStr);
            // should be lost focus, not minimized
            //if (!hasActivate)
            //    newSize.emplace(0, 0);
            if (*newSize == curSize) // size no change
                newSize.reset();
            if (newSize)
                OnResize(newSize->Width, newSize->Height);
            Invalidate();
        }
        void ToplevelClose(xdg_toplevel* toplevel)
        {
            Ensures(Toplevel == toplevel);
            if (!ShouldClose)
            {
                Manager.Logger.Verbose(u"toplevel close\n");
                ShouldClose = OnClose();
            }
            if (ShouldClose)
            {
                Stop();
            }
        }
        void ToplevelConfigBounds(xdg_toplevel* toplevel, int32_t width, int32_t height)
        {
            Ensures(Toplevel == toplevel);
            Manager.Logger.Verbose(u"toplevel conf bounds[{}x{}].\n", width, height);
        }
        void ToplevelWmCaps(xdg_toplevel* toplevel, wl_array* wmCaps)
        {
            Ensures(Toplevel == toplevel);

            common::span<const xdg_toplevel_wm_capabilities> caps;
            if (wmCaps)
                caps = { reinterpret_cast<const xdg_toplevel_wm_capabilities*>(wmCaps->data), wmCaps->size / sizeof(xdg_toplevel_wm_capabilities) };
            std::string capStr;
            for (const auto cap : caps)
            {
                if (!capStr.empty()) capStr.append(", ");
                if (const auto name = XdgWmCapNames(cap); name)
                    capStr.append(*name);
                else
                    capStr.append(std::to_string(common::enum_cast(cap)));
            }
            Manager.Logger.Verbose(u"toplevel WM Cpas[{}].\n", capStr);
        }
        WaylandListener(xdg_toplevel, WdHost, XdgToplevel, (configure, ToplevelConfig), (close, ToplevelClose), (configure_bounds, ToplevelConfigBounds), (wm_capabilities, ToplevelWmCaps));

        void ToplvDecorConfig(zxdg_toplevel_decoration_v1* decor, uint32_t mode)
        {
            Ensures(ToplvDecor == decor);
            Manager.Logger.Verbose(u"window[{:x}] configured [{}] decor.\n", *this, 
                mode == ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE ? u"server"sv : u"client"sv);
        }
        WaylandListener(zxdg_toplevel_decoration_v1, WdHost, XdgToplvDecor, (configure, ToplvDecorConfig));
        
        static void DecorFrameConfig(libdecor_frame* frame, libdecor_configuration* config, void* data)
        {
            auto& host = *reinterpret_cast<WdHost*>(data);
            Ensures(host.DecorFrame == frame);
            auto& manager = host.GetManager();

            libdecor_window_state windowState = LIBDECOR_WINDOW_STATE_NONE;
            manager.decor_configuration_get_window_state(config, &windowState);
            const auto stateDiff = host.DecorState ^ windowState;
            std::string stateStr;
            for (const auto state : DecorWindowStateNames.Items)
            {
                if (state.Key && (windowState & state.Key))
                {
                    if (!stateStr.empty()) stateStr.append(", ");
                    stateStr.append(state.Value);
                }
            }

            std::optional<RectBase<int32_t>> newSize;
            if (stateDiff & LIBDECOR_WINDOW_STATE_MAXIMIZED)
            {
                if (host.DecorState & LIBDECOR_WINDOW_STATE_MAXIMIZED) // max->non-max, restore size
                    newSize = host.NonMaximizeSize;
                else // non-max->max, cache size
                    host.NonMaximizeSize = host;
            }
            else if (stateDiff & LIBDECOR_WINDOW_STATE_ACTIVE)
            {
                // should be lost focus, not minimized
                //if (host.DecorState & LIBDECOR_WINDOW_STATE_ACTIVE) // active->none, treaetd as minimized
                //    newSize.emplace(0, 0);
            }
            if (!newSize)
            {
                newSize.emplace();
                if (manager.decor_configuration_get_content_size(config, frame, &newSize->Width, &newSize->Height))
                {
                    if (*newSize == host) // size no change
                        newSize.reset();
                }
                else
                    newSize.reset();
            }
            if (newSize)
                host.OnResize(newSize->Width, newSize->Height);

            host.DecorState = windowState;
            {
                ReSizingLock lock(host);
                auto state = manager.decor_state_new(host.LastSize.Width, host.LastSize.Height);
                manager.Logger.Verbose(u"decor size[{}] real[{}] state [{:8b}]({}).\n", 
                    host.LastSize, RectBase<int32_t>(host), static_cast<int>(windowState), stateStr);
                manager.decor_frame_commit(frame, state, config);
                manager.decor_state_free(state);
            }

            host.Invalidate(stateDiff);
            if (!host.IsRunning())
            {
                manager.Logger.Verbose(u"now initialize.\n");
                host.Initialize();
            }
        }
        static void DecorFrameClose(libdecor_frame* frame, void* data)
        {
            auto& host = *reinterpret_cast<WdHost*>(data);
            Ensures(host.DecorFrame == frame);
            auto& manager = host.GetManager();
            if (!host.ShouldClose)
            {
                manager.Logger.Verbose(u"decor frame close\n");
                host.ShouldClose = host.OnClose();
            }
            if (host.ShouldClose)
            {
                host.Stop();
            }
        }
        static void DecorFrameCommit(libdecor_frame* frame, void* data)
        {
            auto& host = *reinterpret_cast<WdHost*>(data);
            Ensures(host.DecorFrame == frame);
            auto& manager = host.GetManager();
            manager.MarshalFlags<void, WL_SURFACE_COMMIT>(host.Surface); // wl_surface_commit
        }
        static void DecorFrameDismissPopup(libdecor_frame* frame, const char* seat, void* data)
        {
            auto& host = *reinterpret_cast<WdHost*>(data);
            Ensures(host.DecorFrame == frame);
        }

        struct BackBuffer : public ImageBuffer
        {
            std::mutex UseMutex;
            common::SimpleTimer Timer;
            CacheRect<int32_t> Rect;
            bool Update(WdHost& wd)
            {
                const auto [ sizeChanged, needInit ] = Rect.Update(wd);
                if (sizeChanged)
                {
                    auto& manager = wd.GetManager();
                    PrepareForSize(manager, Rect.Width, Rect.Height);
                    manager.AddListener(Buf, ListenerBackBuf, &wd);
                    manager.Logger.Verbose(u"Resize shm to [{}].\n", Space.size());
                }
                return sizeChanged;
            }
        };
        
        libdecor_frame_interface DecorFrameCB = 
        {
            .configure = &DecorFrameConfig,
            .close = &DecorFrameClose,
            .commit = &DecorFrameCommit,
            .dismiss_popup = &DecorFrameDismissPopup,
        };
        WaylandProxy Surface;
        WaylandProxy XdgSurface;
        WaylandProxy Toplevel;
        WaylandProxy ToplvDecor;
        wl_egl_window* EGLWindow = nullptr;
        libdecor_frame* DecorFrame = nullptr;
        std::unique_ptr<IconData> CurIcon;
        BackBuffer BackBuf[2];
        uint32_t BackBufIdx = 0;
        uint32_t LastFrame = 0; // main thread use
        RectBase<int32_t> LastSize, NonMaximizeSize;
        libdecor_window_state DecorState = LIBDECOR_WINDOW_STATE_NONE;
        bool UseLibDecor = false;
        bool NeedBackground = true;
        bool BgChangedLasttime = false;
        bool ShouldClose = false;
        WdHost(WindowManagerWayland& manager, const WaylandCreateInfo& info) noexcept :
            WaylandWdHost(manager, info) , UseLibDecor(info.PreferLibDecor)
        { }
        ~WdHost() final
        { }
        uintptr_t FormatAs() const noexcept
        {
            return reinterpret_cast<uintptr_t>(Surface.Proxy);
        }
        void* GetSurface() const noexcept final { return Surface.Proxy; }
        void* GetEGLWindow() const noexcept final { return EGLWindow; }
        void OnResize(int32_t width, int32_t height) noexcept final
        {
            WindowHost_::OnResize(width, height);
            if (EGLWindow && width && height)
            {
                GetManager().egl_window_resize(EGLWindow, width, height, 0, 0);
            }
        }
        void OnDisplay(bool forceRedraw) noexcept final;
        void GetClipboard(const std::function<bool(ClipBoardTypes, const std::any&)>& handler, ClipBoardTypes type) final
        {
            auto& manager = GetManager();
            auto& cp = manager.CurrentClipboard;
            std::unique_lock<std::mutex> lock(cp.Mutex);
            if (cp.Offer)
            {
                cp.Tasks.emplace_back(*this, handler, type);
                if (cp.Tasks.size() == 1)
                    manager.SendControlMessage(this, MessageClipboard);
            }
        }
    };

    common::DynLib Wayland;
    common::DynLib WlCursor;
    common::DynLib WlEGL;
    common::DynLib Decor;
#define DECLARE_FUNC(func) decltype(&wl_##func) func = nullptr
    DECLARE_FUNC(proxy_marshal_flags);
    DECLARE_FUNC(proxy_get_version);
    DECLARE_FUNC(proxy_add_listener);
    DECLARE_FUNC(proxy_destroy);
    DECLARE_FUNC(display_connect);
    DECLARE_FUNC(display_disconnect);
    DECLARE_FUNC(display_get_fd);
    DECLARE_FUNC(display_prepare_read);
    DECLARE_FUNC(display_read_events);
    DECLARE_FUNC(display_dispatch_pending);
    DECLARE_FUNC(display_cancel_read);
    DECLARE_FUNC(display_flush);
    DECLARE_FUNC(display_roundtrip);
    DECLARE_FUNC(display_dispatch);
    DECLARE_FUNC(registry_bind);
    DECLARE_FUNC(seat_get_keyboard);
    DECLARE_FUNC(seat_get_pointer);
    DECLARE_FUNC(data_device_manager_get_data_device);
#undef DECLARE_FUNC
#define DECLARE_FUNC(func) decltype(&wl_egl_##func) egl_##func = nullptr
    DECLARE_FUNC(window_create);
    DECLARE_FUNC(window_resize);
    DECLARE_FUNC(window_destroy);
#undef DECLARE_FUNC
#define DECLARE_FUNC(func) decltype(&wl_cursor_##func) cursor_##func = nullptr
    DECLARE_FUNC(theme_load);
    DECLARE_FUNC(theme_get_cursor);
    DECLARE_FUNC(image_get_buffer);
#undef DECLARE_FUNC
#define DECLARE_FUNC(func) decltype(&libdecor_##func) decor_##func = nullptr
    DECLARE_FUNC(new);
    DECLARE_FUNC(unref);
    DECLARE_FUNC(get_fd);
    DECLARE_FUNC(dispatch);
    DECLARE_FUNC(decorate);
    DECLARE_FUNC(frame_set_title);
    DECLARE_FUNC(frame_set_app_id);
    DECLARE_FUNC(frame_map);
    DECLARE_FUNC(frame_commit);
    DECLARE_FUNC(frame_close);
    DECLARE_FUNC(frame_unref);
    DECLARE_FUNC(frame_has_capability);
    DECLARE_FUNC(frame_set_capabilities);
    DECLARE_FUNC(frame_unset_capabilities);
    DECLARE_FUNC(frame_set_visibility);
    DECLARE_FUNC(frame_get_xdg_toplevel);
    DECLARE_FUNC(configuration_get_window_state);
    DECLARE_FUNC(configuration_get_content_size);
    DECLARE_FUNC(state_new);
    DECLARE_FUNC(state_free);
#undef DECLARE_FUNC

    mutable boost::lockfree::queue<Msg, boost::lockfree::capacity<1024>> MsgQueue;
    boost::container::flat_map<std::string_view, WaylandProxy*> InterfaceMap;
    ReWriter InterfaceRewriter;
    WaylandProxy Registry;
    WaylandProxy Output;
    WaylandProxy Shm;
    WaylandProxy Seat;
    WaylandProxy Compositor;
    WaylandProxy WmBase;
    WaylandProxy XdgDecor;
    WaylandProxy XdgIcon;
    WaylandProxy DataDeviceManager;
    WaylandProxy DataDevice;
    WaylandProxy Keyboard;
    WaylandProxy Pointer;
    wl_display* Display = nullptr;
    libdecor* LibDecor = nullptr;
    const wl_interface* ISurface = nullptr;
    const wl_interface* IShmPool = nullptr;
    const wl_interface* IBuffer = nullptr;
    const wl_interface* ICallback = nullptr;
    const wl_interface* IXdgSurface = nullptr;
    const wl_interface* IXdgToplevel = nullptr;
    const wl_interface* IXdgToplevelDecor = nullptr;
    const wl_interface* IXdgToplevelIcon = nullptr;
    xkb_context* XKBContext = nullptr;
    xkb_keymap* XKBKeymap = nullptr;
    xkb_state* XKBState = nullptr;
    wl_cursor_theme* CursorTheme = nullptr;
    wl_cursor* Cursor = nullptr;
    wl_buffer* CursorBuf = nullptr;
    wl_surface* CursorSurface = nullptr;

    int EpollFd = 0, MsgQueueFd = 0, WaylandFd = 0, DecorFd = 0;
    std::optional<std::pair<uint32_t, ImgDType>> ShmDType;
    OfferHolder CurrentOffer;
    ClipboardInfo CurrentClipboard;
    DragInfo CurrentDrag;
    wl_surface* MouseCurSurface = nullptr;
    wl_surface* KeyboardCurSurface = nullptr;
    xkb_mod_index_t CapsLockIndex = 0;
    int32_t PreferredIconSize = 128;
    bool IsCapsLock = false;

    std::string_view Name() const noexcept final { return BackendName; }
    bool CheckFeature(std::string_view feat) const noexcept final
    {
        constexpr std::string_view Features[] =
        {
            "Vulkan"sv, "NewThread"sv,
        };
        if (std::find(std::begin(Features), std::end(Features), feat) != std::end(Features))
            return true;
        if (feat == "OpenGL"sv || feat == "OpenGLES"sv)
            return static_cast<bool>(WlEGL);
        if (feat == "ClientDecor"sv)
            return LibDecor;
        if (feat == "ServerDecor"sv)
            return (bool)XdgDecor;
        return false;
    }
    void* GetDisplay() const noexcept final { return Display; }
    
    forceinline void Destroy(wl_proxy* proxy)
    {
        proxy_destroy(proxy);
    }
    template<typename... P>
    forceinline void Destroy(WaylandProxy& proxy, P&... proxies)
    {
        if (proxy)
        {
            proxy_destroy(proxy.Proxy);
            proxy.Proxy = nullptr;
        }
        if constexpr (sizeof...(P) > 0)
        {
            (..., Destroy(proxies));
        }
    }

    template<typename R, uint32_t Op, typename T, typename... Args>
    [[nodiscard]] forceinline std::enable_if_t<!std::is_void_v<R>, R*> MarshalFlags(T&& obj, uint32_t ver, const wl_interface* retIface, const Args&... args) const noexcept
    {
        using U = std::decay_t<T>;
        wl_proxy* proxy = nullptr;
        if constexpr (std::is_same_v<U, WaylandProxy>)
            proxy = obj.Proxy;
        else if constexpr (std::is_pointer_v<U>)
            proxy = reinterpret_cast<wl_proxy*>(obj);
        else
            static_assert(!common::AlwaysTrue<T>, "unknown obj type");
        return reinterpret_cast<R*>(proxy_marshal_flags(proxy, Op, retIface, ver, 0, args...));
    }
    template<typename R, uint32_t Op, typename T, typename... Args>
    [[nodiscard]] forceinline std::enable_if_t<!std::is_void_v<R>, R*> MarshalFlags(T&& obj, const wl_interface* retIface, const Args&... args) const noexcept
    {
        using U = std::decay_t<T>;
        wl_proxy* proxy = nullptr;
        if constexpr (std::is_same_v<U, WaylandProxy>)
            proxy = obj.Proxy;
        else if constexpr (std::is_pointer_v<U>)
            proxy = reinterpret_cast<wl_proxy*>(obj);
        else
            static_assert(!common::AlwaysTrue<T>, "unknown obj type");
        return reinterpret_cast<R*>(proxy_marshal_flags(proxy, Op, retIface, proxy_get_version(proxy), 0, args...));
    }
    template<typename R, uint32_t Op, typename T, typename... Args>
    forceinline std::enable_if_t<std::is_void_v<R>, void> MarshalFlags(T&& obj, const Args&... args) const noexcept
    {
        using U = std::decay_t<T>;
        wl_proxy* proxy = nullptr;
        if constexpr (std::is_same_v<U, WaylandProxy>)
            proxy = obj.Proxy;
        else if constexpr (std::is_pointer_v<U>)
            proxy = reinterpret_cast<wl_proxy*>(obj);
        else
            static_assert(!common::AlwaysTrue<T>, "unknown obj type");
        proxy_marshal_flags(proxy, Op, nullptr, proxy_get_version(proxy), 0, args...);
    }

    // 1st arg is nullptr for new_id
    template<uint32_t Op, typename T, typename... Args>
    forceinline void GetFrom(T&& obj, WaylandProxy& dst, const Args&... args) const noexcept
    {
        dst.Proxy = MarshalFlags<wl_proxy, Op>(std::forward<T>(obj), dst.Interface, nullptr, args...);
    }
    // 1st arg is nullptr for new_id
    template<uint32_t Op, typename T, typename... Args>
    forceinline void GetFrom(T&& obj, uint32_t ver, WaylandProxy& dst, const Args&... args) const noexcept
    {
        dst.Proxy = MarshalFlags<wl_proxy, Op>(std::forward<T>(obj), ver, dst.Interface, nullptr, args...);
    }
    // 1st arg is nullptr for new_id
    template<uint32_t Op, typename... Args>
    [[nodiscard]] forceinline WaylandProxy GetFrom(const WaylandProxy& src, const wl_interface* iface, const Args&... args) const noexcept
    {
        WaylandProxy dst(iface);
        GetFrom<Op>(src, dst, args...);
        return dst;
    }
    // 1st arg is nullptr for new_id
    template<uint32_t Op, typename... Args>
    [[nodiscard]] forceinline WaylandProxy GetFrom(const WaylandProxy& src, uint32_t ver, const wl_interface* iface, const Args&... args) const noexcept
    {
        WaylandProxy dst(iface);
        GetFrom<Op>(src, ver, dst, args...);
        return dst;
    }

    template<uint32_t Op, typename T>
    forceinline void Destroy(T*& obj) const noexcept
    {
        if (obj)
        {
            const auto proxy = reinterpret_cast<wl_proxy*>(obj);
            proxy_marshal_flags(proxy, Op, nullptr, proxy_get_version(proxy), WL_MARSHAL_FLAG_DESTROY);
            obj = nullptr;
        }
    }
    template<uint32_t Op>
    forceinline void Destroy(WaylandProxy& proxy) const noexcept
    {
        Destroy<Op>(proxy.Proxy);
    }

    template<typename T, typename L>
    void AddListener(T* proxy, L& listener, void *data) const noexcept
    {
        proxy_add_listener(reinterpret_cast<wl_proxy*>(proxy), (void (**)(void))&listener, data);
    }
    template<typename L>
    void AddListener(const WaylandProxy& proxy, L& listener, void *data) const noexcept
    {
        AddListener(proxy.Proxy, listener, data);
    }

    void UpdateXKBState()
    {
        IsCapsLock = xkb_state_mod_index_is_active(XKBState, CapsLockIndex, XKB_STATE_MODS_EFFECTIVE) > 0;
    }

    void DisplayError(wl_display* display, void* objId, uint32_t code, const char* message)
    {
        Ensures(Display == display);
        const auto msg = common::str::to_u16string(message, common::str::Encoding::UTF8);
        Logger.Error(u"Display error obj[{:p}] code[{}]: {}.\n", objId, code, msg);
        COMMON_THROW(BaseException, u"wayland display fatal error").Attach("detail", msg);
    }
    WaylandListener(wl_display, WindowManagerWayland, Display, (error, DisplayError), (delete_id,));

    void AttachGlobal(wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
    {
        const std::string_view iname(interface);
        WaylandProxy* proxy = nullptr;
        if (const auto it = InterfaceMap.find(iname); it != InterfaceMap.end())
            proxy = it->second;
        else if (iname == hack::xdg_wm_base_interface.name)
        {
            InterfaceRewriter.ReplaceTypes(hack::xdg_shell_types);
            IXdgSurface = InterfaceRewriter.Replace(&hack::xdg_surface_interface);
            IXdgToplevel = InterfaceRewriter.Replace(&hack::xdg_toplevel_interface);
            WmBase.Interface = InterfaceRewriter.Replace(&hack::xdg_wm_base_interface);
            proxy = &WmBase;
        }
        else if (iname == hack::xdg_toplevel_icon_manager_v1_interface.name)
        {
            InterfaceRewriter.ReplaceTypes(hack::xdg_toplevel_icon_v1_types);
            IXdgToplevelIcon = InterfaceRewriter.Replace(&hack::xdg_toplevel_icon_v1_interface);
            XdgIcon.Interface = InterfaceRewriter.Replace(&hack::xdg_toplevel_icon_manager_v1_interface);
            proxy = &XdgIcon;
        }
        else if (iname == hack::zxdg_decoration_manager_v1_interface.name)
        {
            InterfaceRewriter.ReplaceTypes(hack::xdg_decoration_unstable_v1_types);
            IXdgToplevelDecor = InterfaceRewriter.Replace(&hack::zxdg_toplevel_decoration_v1_interface);
            XdgDecor.Interface = InterfaceRewriter.Replace(&hack::zxdg_decoration_manager_v1_interface);
            proxy = &XdgDecor;
        }
        common::mlog::LogLevel loglv = common::mlog::LogLevel::Verbose;
        std::u16string extraMsg;
        if (proxy)
        {
            loglv = common::mlog::LogLevel::Info;
            const uint32_t verClient = proxy->Interface->version;
            auto ver = std::min(version, verClient);
            common::str::Formatter<char16_t> fmter{};
            fmter.FormatToStatic(extraMsg, FmtString(u" -binded cver[{}]"), verClient);

            std::optional<std::pair<uint32_t, uint32_t>> verReq;
#define IFACE_VER(target, name) if (proxy == &target) verReq.emplace(Version::name##Min, Version::name)
            IFACE_VER(DataDeviceManager, DataDev);
            else IFACE_VER(Keyboard, Keyboard);
            else IFACE_VER(Pointer, Pointer);
#undef IFACE_VER
            if (verReq)
            {
                const auto [verMin, verHead] = *verReq;
                fmter.FormatToStatic(extraMsg, FmtString(u" hver[>={}] minver[{}]"), verHead, verMin);
                if (ver < verMin)
                {
                    extraMsg += u" TooLow!";
                    loglv = common::mlog::LogLevel::Warning;
                }
                else
                    ver = verMin;
            }
            proxy->Proxy = MarshalFlags<wl_proxy, WL_REGISTRY_BIND>(registry, ver, proxy->Interface, name, proxy->Interface->name, ver, nullptr);
        }
        Logger.Log(loglv, u"wayland global [{}] name[{}] sver[{}]{}\n", iname, name, version, extraMsg);
    }
    WaylandListener(wl_registry, WindowManagerWayland, Reg, (global, AttachGlobal), (global_remove,));

    void ShmFormat(wl_shm* shm, uint32_t format)
    {
        if (const auto dtype = ShmFormatLookup(format); dtype)
        {
            Logger.Verbose(u"Shm format[{:08X}] -> [{}]\n", format, dtype->ToString());
            if (!ShmDType) ShmDType.emplace(format, *dtype);
        }
    }
    WaylandListener(wl_shm, WindowManagerWayland, Shm, (format, ShmFormat));

    void WmBasePing(xdg_wm_base* wmBase, uint32_t serial)
    {
        Ensures(WmBase == wmBase);
        MarshalFlags<void, XDG_WM_BASE_PONG>(WmBase, serial); // xdg_wm_base_pong(xdg_wm_base, serial);
    }
    WaylandListener(xdg_wm_base, WindowManagerWayland, XdgWm, (ping, WmBasePing));

    void XdgIconSize(xdg_toplevel_icon_manager_v1* xdgIcon, int32_t size)
    {
        Ensures(XdgIcon == xdgIcon);
        if (size > 0)
            PreferredIconSize = size;
    }
    WaylandListener(xdg_toplevel_icon_manager_v1, WindowManagerWayland, XdgIcon, (icon_size, XdgIconSize), (done,));

    void KeyboardKeymap(wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size)
    {
        if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
        {
	        const auto mapstr = reinterpret_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
            if (mapstr != MAP_FAILED)
            {
                const auto keymap = xkb_keymap_new_from_string(XKBContext, mapstr, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
                const auto state = xkb_state_new(keymap);
                if (state)
                {
                    XKBKeymap = keymap;
                    XKBState = state;
                    CapsLockIndex = xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
                    UpdateXKBState();
                }
	            munmap(mapstr, size);
            }
        }
        close(fd);
    }
    void KeyboardEnter(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array*)
    {
        KeyboardCurSurface = surface;
    }
    void KeyboardLeave(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface)
    {
        KeyboardCurSurface = nullptr;
    }
    void KeyboardKey(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
    {
        const bool isPressed = state == WL_KEYBOARD_KEY_STATE_PRESSED;
        const auto ckey = ProcessXKBKey(XKBState, key + 8);
        if (const auto host = GetWindow(KeyboardCurSurface); host)
        {
            if (isPressed) 
            {
                // dirty fix for caps state since it's updated in next message
                if (ckey.Key == event::CommonKeys::CapsLock)
                    IsCapsLock = !IsCapsLock;
                host->OnKeyDown(ckey);
            }
            else
                host->OnKeyUp(ckey);
        }
        else
            Logger.Verbose(u"Key [{}]({}) state[{}].\n", key, static_cast<uint8_t>(ckey.Key), isPressed);
    }
    void KeyboardModifier(wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
    {
        xkb_state_update_mask(XKBState, mods_depressed, mods_latched, mods_locked, 0, 0, group);
        UpdateXKBState();
        Logger.Verbose(u"Current CapsLock [{}].\n", IsCapsLock ? 'Y' : 'N');
    }
    WaylandListener(wl_keyboard, WindowManagerWayland, KeyBoard, 
        (keymap, KeyboardKeymap), (enter, KeyboardEnter), (leave, KeyboardLeave), (key, KeyboardKey), (modifiers, KeyboardModifier), (repeat_info,));

    void PointerEnter(wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy)
    {
        MouseCurSurface = surface;
        if (const auto host = GetWindow(surface); host)
        {
            const auto x = static_cast<int32_t>(wl_fixed_to_double(sx));
            const auto y = static_cast<int32_t>(wl_fixed_to_double(sy));
            event::Position pos(x, y);
            host->OnMouseEnter(pos);
            MarshalFlags<void, WL_POINTER_SET_CURSOR>(Pointer, serial, CursorSurface, Cursor->images[0]->hotspot_x, Cursor->images[0]->hotspot_y); // wl_pointer_set_cursor
        }
    }
    void PointerLeave(wl_pointer* pointer, uint32_t serial, wl_surface* surface)
    {
        if (const auto host = GetWindow(surface); host)
        {
            host->OnMouseLeave();
        }
        MouseCurSurface = nullptr;
    }
    void PointerMotion(wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
    {
        if (const auto host = GetWindow(MouseCurSurface); host)
        {
            const auto x = static_cast<int32_t>(wl_fixed_to_double(sx));
            const auto y = static_cast<int32_t>(wl_fixed_to_double(sy));
            event::Position pos(x, y);
            host->OnMouseMove(pos);
        }
    }
    void PointerButton(wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
    {
        if (const auto host = GetWindow(MouseCurSurface); host)
        {
            event::MouseButton btn = event::MouseButton::None;
            switch (button)
            {
            case BTN_LEFT:   btn = event::MouseButton::Left;   break;
            case BTN_MIDDLE: btn = event::MouseButton::Middle; break;
            case BTN_RIGHT:  btn = event::MouseButton::Right;  break;
            default: break;
            }
            const bool isPress = state == WL_POINTER_BUTTON_STATE_PRESSED;
            host->OnMouseButton(btn, isPress);
        }
    }
    void PointerScroll(wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
    {
        if (const auto host = GetWindow(MouseCurSurface); host)
        {
            const auto val = static_cast<float>(wl_fixed_to_double(value)) / 20;
            float dh = 0, dv = 0;
            if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
                dv = -val;
            else
                dh = -val;
            host->OnMouseScroll(host->LastPos, dh, dv);
        }
        //Logger.Verbose(u"Mouse scroll[{}] val[{}].\n", axis, val);
    }
    WaylandListener(wl_pointer, WindowManagerWayland, Pointer, 
        (enter, PointerEnter), (leave, PointerLeave), (motion, PointerMotion), (button, PointerButton), (axis, PointerScroll), 
        (frame,), (axis_source,), (axis_stop,), (axis_discrete,), (axis_value120,), (axis_relative_direction,));

    void DataOfferOffer(wl_data_offer* offer, const char* mime) 
    {
        Logger.Verbose(u"data[{:p}] mime[{}].\n", offer, mime);
        if (CurrentOffer.DataOffer == offer)
            CurrentOffer.Add(mime);
    }
    void DataOfferSrcAction(wl_data_offer* offer, uint32_t actions) 
    {
        Logger.Verbose(u"data[{:p}] src action[{}].\n", offer, actions);
    }
    void DataOfferAction(wl_data_offer* offer, uint32_t actions) 
    {
        Logger.Verbose(u"data[{:p}] dnd action[{}].\n", offer, actions);
    }
    WaylandListener(wl_data_offer, WindowManagerWayland, DataOffer, (offer, DataOfferOffer), (source_actions, DataOfferSrcAction), (action, DataOfferAction));

    void DataDevOffer(wl_data_device* data_device, wl_data_offer* offer) 
    {
        Logger.Verbose(u"data[{:p}] here, prev[{:p}].\n", offer, CurrentOffer.DataOffer);
        ClipboardTaskClean(false);
        CurrentDrag.Offer = nullptr;
        if (CurrentOffer.DataOffer) 
        {
            Destroy<WL_DATA_OFFER_DESTROY>(CurrentOffer.DataOffer);
            CurrentOffer.Clear();
        }
        CurrentOffer.DataOffer = offer;
        if (offer)
            AddListener(offer, ListenerDataOffer, this);
    }
    void DataDevSelection(wl_data_device* data_device, wl_data_offer* offer) 
    {
        Logger.Verbose(u"data[{:p}] selection here.\n", offer);
        ClipboardTaskClean(offer && offer == CurrentOffer.DataOffer);
        if (offer && offer == CurrentOffer.DataOffer)
            CurrentClipboard.Offer = &CurrentOffer;
        else
            CurrentClipboard.Offer = nullptr;
    }
    void DataDevEnter(wl_data_device* data_device, uint32_t serial, wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy, wl_data_offer* offer) 
    {
        const auto x = static_cast<int32_t>(wl_fixed_to_double(sx));
        const auto y = static_cast<int32_t>(wl_fixed_to_double(sy));
        Logger.Verbose(u"data[{:p}] dnd enter [{:x}] at [{},{}].\n", offer, reinterpret_cast<uintptr_t>(surface), x, y);
        ClipboardTaskClean(false);
        if (offer && offer == CurrentOffer.DataOffer)
        {
            if (const auto host = GetWindow(surface); host)
            {
                const auto it = std::find_if(CurrentOffer.Types.begin(), CurrentOffer.Types.end(), [](const auto& item){ return item.Category == ClipBoardTypes::File; });
                if (it != CurrentOffer.Types.end())
                {
                    CurrentDrag.Offer = &CurrentOffer;
                    CurrentDrag.Target = host;
                    CurrentDrag.PosX = x, CurrentDrag.PosY = y;
                    CurrentDrag.TypeIndex = static_cast<uint32_t>(std::distance(CurrentOffer.Types.begin(), it));
                    MarshalFlags<void, WL_DATA_OFFER_SET_ACTIONS>(CurrentDrag.Offer->DataOffer, 
                        WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY, WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY); // wl_data_offer_set_actions
                    MarshalFlags<void, WL_DATA_OFFER_ACCEPT>(CurrentDrag.Offer->DataOffer, serial, 
                        CurrentOffer.Pool.GetStringView(it->TypeStr).data()); // wl_data_offer_accept
                    return;
                }
            }
        }
        else
            CurrentDrag.Offer = nullptr;
        CurrentDrag.Target = nullptr;
    }
    void DataDevLeave(wl_data_device* data_device) 
    {
        Logger.Verbose(u"data[{:p}] dnd leave.\n", CurrentDrag.Offer ? CurrentDrag.Offer->DataOffer : nullptr);
        Ensures(CurrentClipboard.Tasks.empty() && !CurrentClipboard.Offer);
        if (!CurrentDrag.InDrop)
            CurrentDrag.Target = nullptr;
    }
    void DataDevMotion(wl_data_device* data_device, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) 
    {
        const auto x = static_cast<int32_t>(wl_fixed_to_double(sx));
        const auto y = static_cast<int32_t>(wl_fixed_to_double(sy));
        // Logger.Verbose(u"data[{:p}] dnd motion at [{},{}].\n", CurrentDrag.Offer ? CurrentDrag.Offer->DataOffer : nullptr, x, y);
        Ensures(CurrentClipboard.Tasks.empty() && !CurrentClipboard.Offer);
        if (CurrentDrag.Offer && CurrentDrag.Target)
        {
            CurrentDrag.PosX = x, CurrentDrag.PosY = y;
        }
    }
    void DataDevDrop(wl_data_device* data_device) 
    {
        Logger.Verbose(u"data[{:p}] dnd drop.\n", CurrentDrag.Offer ? CurrentDrag.Offer->DataOffer : nullptr);
        Ensures(CurrentClipboard.Tasks.empty() && !CurrentClipboard.Offer);
        if (CurrentDrag.Offer && CurrentDrag.Target)
        {
            if (IssueOfferRead(CurrentDrag.TypeIndex, FdCookieDrag))
                CurrentDrag.InDrop = true;
            else // failed
                MarshalFlags<void, WL_DATA_OFFER_FINISH>(CurrentDrag.Offer->DataOffer); // wl_data_offer_finish
        }
    }
    WaylandListener(wl_data_device, WindowManagerWayland, DataDev, 
        (data_offer, DataDevOffer), (enter, DataDevEnter), (leave, DataDevLeave), (motion, DataDevMotion), (drop, DataDevDrop), (selection, DataDevSelection));

    void OutputGeometry(wl_output *output, int32_t x, int32_t y, int32_t pw, int32_t ph, int32_t subpix, const char* make, const char* model, int32_t transform)
    {
        const auto dpix = 25.4f * x / pw, dpiy = 25.4f * y / ph;
        Logger.Verbose(u"output[{} {}], res[{}x{}] dpi[{} {}].\n", make, model, x, y, dpix, dpiy);
    }
    void OutputMode(wl_output *output, uint32_t flags, int32_t width, int32_t height, int32_t refresh)
    {
        Logger.Verbose(u"output HW [{}x{}], [{}]hz.\n", width, height, refresh / 1000.f);
    }
    WaylandListener(wl_output, WindowManagerWayland, Output, (geometry, OutputGeometry), (mode, OutputMode),
        (done,), (scale,), (name,), (description,));

    template<typename T>
    forceinline void LoadInterface(T& target, std::string_view name) noexcept
    {
        const auto ptr = Wayland.GetFunction<const wl_interface*>(name);
        target = ptr;
        if constexpr (std::is_same_v<WaylandProxy, T>)
        {
            const auto ret = InterfaceMap.try_emplace(ptr->name, &target);
            Ensures(ret.second);
        }
    }
    void SendControlMessage(WdHost* host, uint32_t op, uint32_t data = 0, void* ptr = nullptr) const noexcept
    {
        //Logger.Verbose(u"Push Msg[{}] on window[{:x}].\n", op, (uintptr_t)host->Surface.Proxy);
        MsgQueue.push({host, op, data, ptr});
        constexpr uint64_t val = 1;
        write(MsgQueueFd, &val, 8);
    }
    void SetTitle(WdHost& host, std::u16string_view name)
    {
        const auto title = common::str::to_u8string(name, common::str::Encoding::UTF16LE);
        if (host.UseLibDecor)
            decor_frame_set_title(host.DecorFrame, reinterpret_cast<const char*>(title.c_str()));
        else
            MarshalFlags<void, XDG_TOPLEVEL_SET_TITLE>(host.Toplevel, title.c_str()); // xdg_toplevel_set_title
    }
    void SetIcon(WdHost& host, std::unique_ptr<IconData>&& icon)
    {
        MarshalFlags<void, XDG_TOPLEVEL_ICON_MANAGER_V1_SET_ICON>(XdgIcon, host.Toplevel.Proxy, icon ? icon->Icon.Proxy : nullptr);
        if (host.CurIcon)
        {
            Destroy<XDG_TOPLEVEL_ICON_V1_DESTROY>(host.CurIcon->Icon);
            host.CurIcon->ReleaseObjects(*this);
        }
        host.CurIcon = std::move(icon);
    }
    bool IssueOfferRead(uint32_t typeIdx, uint32_t cookieId)
    {
        Expects(CurrentOffer.Fd[0] == -1 && CurrentOffer.Fd[1] == -1);
        if (pipe2(CurrentOffer.Fd, O_CLOEXEC | O_NONBLOCK) == -1) // failed
            return false;
        const auto mime = CurrentOffer.Get(typeIdx);
        Logger.Verbose(u"DataOffer type[{}]({}) init pipe [{},{}].\n", typeIdx, mime, CurrentOffer.Fd[0], CurrentOffer.Fd[1]);
        MarshalFlags<void, WL_DATA_OFFER_RECEIVE>(CurrentOffer.DataOffer, mime.data(), CurrentOffer.Fd[1]);
        close(CurrentOffer.Fd[1]);
        
        epoll_event ee
        {
            .events = EPOLLIN,
            .data = { .u32 = FdCookieComplex | cookieId }
        };
        epoll_ctl(EpollFd, EPOLL_CTL_ADD, CurrentOffer.Fd[0], &ee);
        return true;
    }
    void CloseOfferRead()
    {
        if (CurrentOffer.Fd[0] != -1)
        {
            epoll_ctl(EpollFd, EPOLL_CTL_DEL, CurrentOffer.Fd[0], nullptr);
            close(CurrentOffer.Fd[0]);
            CurrentOffer.Fd[0] = CurrentOffer.Fd[1] = -1;
        }
    }
    void DragDropRead()
    {
        OfferHolder::States readRet = OfferHolder::States::Faulted;
        if (CurrentDrag.Offer == &CurrentOffer && CurrentDrag.Target)
        {
            readRet = CurrentDrag.Offer->PerformRead(Logger, CurrentDrag.TypeIndex);
            if (readRet == OfferHolder::States::More) return;
        }
        CloseOfferRead();
        MarshalFlags<void, WL_DATA_OFFER_FINISH>(CurrentDrag.Offer->DataOffer); // wl_data_offer_finish
        if (readRet == OfferHolder::States::Finished && !CurrentDrag.Offer->Data.empty())
        {
            event::Position pos(CurrentDrag.PosX, CurrentDrag.PosY);
            auto files = UriStringToFiles({ reinterpret_cast<const char*>(CurrentDrag.Offer->Data.data()), CurrentDrag.Offer->Data.size() });
            static_cast<WdHost*>(CurrentDrag.Target)->OnDropFile(pos, std::move(files));
            CurrentDrag.Offer->Data.clear();
        }
    }
    void ClipboardTaskNewRead(ClipboardTask& task)
    {
        Expects(CurrentClipboard.Offer);
        Expects(CurrentOffer.Data.empty());
        Expects(CurrentOffer.Fd[0] == -1 && CurrentOffer.Fd[1] == -1);
        while (task.TypeIndex < CurrentClipboard.Offer->Types.size())
        {
            const auto& type = CurrentClipboard.Offer->Types[task.TypeIndex];
            if (HAS_FIELD(task.SupportTypes, type.Category))
            {
                if (IssueOfferRead(task.TypeIndex, FdCookieClipboard)) // success
                    return;
                break;
            }
            task.TypeIndex++;
        }
        // pipe failed or all type iterated, finished this task
        ClipboardTaskEnded(task);
    }
    void ClipboardTaskEnded(ClipboardTask& task)
    {
        Expects(CurrentOffer.Data.empty());
        Expects(CurrentOffer.Fd[0] == -1 && CurrentOffer.Fd[1] == -1); // reading should be closed
        Logger.Verbose(u"Clipboard task from window[{:x}] ends.\n", static_cast<WdHost&>(task.Host));
        ClipboardTask* next = nullptr;
        {
            std::unique_lock<std::mutex> lock(CurrentClipboard.Mutex);
            Ensures(&CurrentClipboard.Tasks.front() == &task);
            CurrentClipboard.Tasks.pop_front();
            if (!CurrentClipboard.Tasks.empty())
                next = &CurrentClipboard.Tasks.front();
        }
        if (next)
            SendControlMessage(&static_cast<WdHost&>(next->Host), MessageClipboard); // delay to next iteration
    }
    // return is handled
    bool ClipboardProcess(ClipboardTask& task)
    {
        constexpr bool StopAtFault = true;
        auto& offer = *CurrentClipboard.Offer;
        const auto mime = offer.Get(task.TypeIndex);
        const auto type = offer.Types[task.TypeIndex].Category;
        switch (type)
        {
        case ClipBoardTypes::Text:
        {
            std::string_view txt(reinterpret_cast<const char*>(offer.Data.data()), offer.Data.size());
            Logger.Verbose(u"Clipboard: Text({}): [{}].\n", mime, txt);
            return InvokeClipboard(task.Handler, ClipBoardTypes::Text, txt, StopAtFault);
        } break;
        case ClipBoardTypes::File:
        {
            auto files = UriStringToFiles({ reinterpret_cast<const char*>(offer.Data.data()), offer.Data.size() });
            return InvokeClipboard(task.Handler, ClipBoardTypes::File, std::move(files), StopAtFault);
            // logger.Verbose(u"Clipboard: File({}): [{}].\n", mime, str);
        } break;
        case ClipBoardTypes::Image:
        {
            std::u16string ext;
            if (mime.starts_with("image/"))
                ext = common::str::to_u16string(mime.substr(6));
            Logger.Verbose(u"Clipboard: Image({}): ext[{}] [{}]bytes.\n", mime, ext, offer.Data.size());
            if (ImageView img = TryReadImage(offer.Data, ext, xziar::img::ImageDataType::RGBA); img.GetSize() > 0)
                return InvokeClipboard(task.Handler, ClipBoardTypes::Image, img, StopAtFault);
        } break;
        default: break;
        }
        return false;
    }
    void ClipboardTaskRead()
    {
        const auto task = CurrentClipboard.GetFront();
        if (!task)
        {
            Logger.Warning(u"Clipbaord read while task list is empty.\n");
            return;
        }
        const auto readRet = CurrentClipboard.Offer->PerformRead(Logger, task->TypeIndex);
        bool shouldContinue = false;
        switch(readRet)
        {
        case OfferHolder::States::More: return;
        case OfferHolder::States::Finished:
            if (!CurrentClipboard.Offer->Data.empty())
            {
                shouldContinue = !ClipboardProcess(*task); 
                CurrentClipboard.Offer->Data.clear();
            }
            else
                shouldContinue = true;
            task->TypeIndex++;
            break;
        default:
            break;
        }
        Ensures(CurrentClipboard.Offer->Data.empty());
        CloseOfferRead();
        if (shouldContinue)
            ClipboardTaskNewRead(*task);
        else // finish or fault
            ClipboardTaskEnded(*task);
    }
    void ClipboardTaskClean(bool keepOffer = false)
    {
        std::unique_lock<std::mutex> lock(CurrentClipboard.Mutex);
        CloseOfferRead();
        CurrentClipboard.Tasks.clear();
        CurrentClipboard.Offer = keepOffer ? &CurrentOffer : nullptr;
    }
public:
    WindowManagerWayland() : WaylandBackend(true), 
        Wayland(common::DynLib::TryCreate("libwayland-client.so", "libwayland-client.so.0")), 
        WlCursor(common::DynLib::TryCreate("libwayland-cursor.so", "libwayland-cursor.so.0")), 
        WlEGL(common::DynLib::TryCreate("libwayland-egl.so", "libwayland-egl.so.1")), 
        Decor{common::DynLib::TryCreate("libdecor-0.so", "libdecor-0.so.0")}
    {
        if (const auto tmp = common::SharedMemory::CreateMemFd("wayland_buf", MFD_CLOEXEC); tmp != -1)
            close(tmp);

        Wayland.Validate();
        WlCursor.Validate();
#define LOAD_FUNC(func) func = Wayland.GetFunction<decltype(&wl_##func)>("wl_" #func ""sv)
        LOAD_FUNC(proxy_marshal_flags);
        LOAD_FUNC(proxy_get_version);
        LOAD_FUNC(proxy_add_listener);
        LOAD_FUNC(proxy_destroy);
        LOAD_FUNC(display_connect);
        LOAD_FUNC(display_disconnect);
        LOAD_FUNC(display_get_fd);
        LOAD_FUNC(display_prepare_read);
        LOAD_FUNC(display_read_events);
        LOAD_FUNC(display_dispatch_pending);
        LOAD_FUNC(display_cancel_read);
        LOAD_FUNC(display_flush);
        LOAD_FUNC(display_roundtrip);
        LOAD_FUNC(display_dispatch);
#undef LOAD_FUNC

#define LOAD_IFACE(iface, target) LoadInterface(target, "wl_" #iface "_interface"sv)
        LOAD_IFACE(registry, Registry);
        LOAD_IFACE(output, Output);
        LOAD_IFACE(surface, ISurface);
        LOAD_IFACE(shm_pool, IShmPool);
        LOAD_IFACE(buffer, IBuffer);
        LOAD_IFACE(callback, ICallback);
        LOAD_IFACE(shm, Shm);
        LOAD_IFACE(seat, Seat);
        LOAD_IFACE(compositor, Compositor);
        LOAD_IFACE(data_device_manager, DataDeviceManager);
        LOAD_IFACE(data_device, DataDevice);
        LOAD_IFACE(keyboard, Keyboard);
        LOAD_IFACE(pointer, Pointer);
#undef LOAD_IFACE

#define LOAD_FUNC(func) cursor_##func = WlCursor.GetFunction<decltype(&wl_cursor_##func)>("wl_cursor_" #func ""sv)
        LOAD_FUNC(theme_load);
        LOAD_FUNC(theme_get_cursor);
        LOAD_FUNC(image_get_buffer);
#undef LOAD_FUNC
        
        if (WlEGL)
        {
#define LOAD_FUNC(func) egl_##func = WlEGL.GetFunction<decltype(&wl_egl_##func)>("wl_egl_" #func ""sv)
            LOAD_FUNC(window_create);
            LOAD_FUNC(window_resize);
            LOAD_FUNC(window_destroy);
#undef LOAD_FUNC
        }

        if (Decor)
        {
#define LOAD_FUNC(func) decor_##func = Decor.GetFunction<decltype(&libdecor_##func)>("libdecor_" #func ""sv)
            LOAD_FUNC(new);
            LOAD_FUNC(unref);
            LOAD_FUNC(get_fd);
            LOAD_FUNC(dispatch);
            LOAD_FUNC(decorate);
            LOAD_FUNC(frame_set_title);
            LOAD_FUNC(frame_set_app_id);
            LOAD_FUNC(frame_map);
            LOAD_FUNC(frame_commit);
            LOAD_FUNC(frame_close);
            LOAD_FUNC(frame_unref);
            LOAD_FUNC(frame_has_capability);
            LOAD_FUNC(frame_set_capabilities);
            LOAD_FUNC(frame_unset_capabilities);
            LOAD_FUNC(frame_set_visibility);
            LOAD_FUNC(frame_get_xdg_toplevel);
            LOAD_FUNC(configuration_get_window_state);
            LOAD_FUNC(configuration_get_content_size);
            LOAD_FUNC(state_new);
            LOAD_FUNC(state_free);
#undef LOAD_FUNC
        }

        InterfaceRewriter.PlaceMapper(&hack::wl_seat_interface, &Seat.Interface);
        InterfaceRewriter.PlaceMapper(&hack::wl_output_interface, &Output.Interface);
        InterfaceRewriter.PlaceMapper(&hack::wl_surface_interface, &ISurface);
        InterfaceRewriter.PlaceMapper(&hack::wl_buffer_interface, &IBuffer);

        EpollFd = epoll_create1(EPOLL_CLOEXEC);
        MsgQueueFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        epoll_event ee
        {
            .events = EPOLLIN,
            .data = { .fd = MsgQueueFd }
        };
        epoll_ctl(EpollFd, EPOLL_CTL_ADD, MsgQueueFd, &ee);
    }
    ~WindowManagerWayland() final 
    {
        close(EpollFd);
        close(MsgQueueFd);
    }
    bool CanServerDecorate() const noexcept final
    {
        EnsureInitState(true);
        return (bool)XdgDecor;
    }
    bool CanClientDecorate() const noexcept final
    {
        EnsureInitState(true);
        return LibDecor;
    }

    void OnInitialize(const void* info_) final
    {
        using common::BaseException;

        const auto info = reinterpret_cast<const WaylandInitInfo*>(info_);
        const auto dispName = (info && !info->DisplayName.empty()) ? info->DisplayName.c_str() : nullptr;

        Display = display_connect(dispName);
        if (!Display)
        {
            COMMON_THROW(BaseException, u"Can't connect wayland display");
        }

        //AddListener(Display, ListenerDisplay, this);
        GetFrom<WL_DISPLAY_GET_REGISTRY>(Display, Registry);
        AddListener(Registry, ListenerReg, this);
        XKBContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        //XKBKeymap = xkb_x11_keymap_new_from_device(XKBContext, Connection, kbId, XKB_KEYMAP_COMPILE_NO_FLAGS);
        //XKBState = xkb_x11_state_new_from_device(XKBKeymap, Connection, kbId);
        display_roundtrip(Display);
        Destroy(Registry);

        if (!Shm || !Seat || !Compositor || !WmBase || !IXdgSurface || !IXdgToplevel)
        {
            COMMON_THROW(BaseException, u"Wayland display missing required interfaces");
        }
        {
            WaylandFd = display_get_fd(Display);
            epoll_event ee
            {
                .events = EPOLLIN,
                .data = { .fd = WaylandFd }
            };
            epoll_ctl(EpollFd, EPOLL_CTL_ADD, WaylandFd, &ee);
        }
        if (Output)
            AddListener(Output, ListenerOutput, this);
        AddListener(Shm, ListenerShm, this);
        AddListener(WmBase, ListenerXdgWm, this);
        GetFrom<WL_SEAT_GET_KEYBOARD>(Seat, 1, Keyboard);
        AddListener(Keyboard, ListenerKeyBoard, this);
        GetFrom<WL_SEAT_GET_POINTER>(Seat, 1, Pointer);
        AddListener(Pointer, ListenerPointer, this);
        if (DataDeviceManager)
        {
            GetFrom<WL_DATA_DEVICE_MANAGER_GET_DATA_DEVICE>(DataDeviceManager, DataDevice, Seat.Proxy);
            AddListener(DataDevice, ListenerDataDev, this);
        }
        if (XdgIcon)
            AddListener(XdgIcon, ListenerXdgIcon, this);
        CursorTheme = cursor_theme_load(nullptr, 24, reinterpret_cast<wl_shm*>(Shm.Proxy));
        Cursor = cursor_theme_get_cursor(CursorTheme, "left_ptr");
        CursorBuf = cursor_image_get_buffer(Cursor->images[0]);
        CursorSurface = MarshalFlags<wl_surface, WL_COMPOSITOR_CREATE_SURFACE>(Compositor, ISurface);
        MarshalFlags<void, WL_SURFACE_ATTACH>(CursorSurface, CursorBuf, 0, 0); // wl_surface_attach
        MarshalFlags<void, WL_SURFACE_COMMIT>(CursorSurface); // wl_surface_commit

        display_roundtrip(Display);
        if (!ShmDType)
            COMMON_THROW(BaseException, u"Not found preferred shm dtype");

        if (!XdgDecor && !Decor)
            COMMON_THROW(BaseException, u"No decor available");
        if (Decor)
        {
            libdecor_interface decorCB
            {
                .error = [](libdecor* context, libdecor_error error, const char *message)
                {
                    if (const auto be = WindowBackend::GetBackend(BackendName); be)
                    {
                        static_cast<const WindowManagerWayland*>(be)->Logger.Error(u"libdecor error: [{}] {}.\n", (uint32_t)error, message);
                    }
                }
            };
            LibDecor = decor_new(Display, &decorCB);
            if (!LibDecor)
                Logger.Warning(u"Failed to new libdecor context.\n");
            DecorFd = decor_get_fd(LibDecor);
            if (DecorFd != WaylandFd)
            {
                epoll_event ee
                {
                    .events = EPOLLIN,
                    .data = { .fd = DecorFd }
                };
                epoll_ctl(EpollFd, EPOLL_CTL_ADD, DecorFd, &ee);
            }
        }
        if (!XdgDecor && !LibDecor)
            COMMON_THROW(BaseException, u"No decor available");
        WaylandBackend::OnInitialize(info_);
    }
    void OnDeInitialize() noexcept final
    {
        if (LibDecor)
            decor_unref(LibDecor);
        Destroy<ZXDG_DECORATION_MANAGER_V1_DESTROY>(XdgDecor);
        Destroy<XDG_TOPLEVEL_ICON_MANAGER_V1_DESTROY>(XdgIcon);
        Destroy<XDG_WM_BASE_DESTROY>(WmBase);
        Destroy(Pointer, Keyboard, DataDevice, DataDeviceManager, Compositor, Seat, Shm, Output);
        if (XKBState)
            xkb_state_unref(XKBState);
        if (XKBKeymap)
            xkb_keymap_unref(XKBKeymap);
		xkb_context_unref(XKBContext);
        display_flush(Display);
        display_disconnect(Display);
        WaylandBackend::OnDeInitialize();
    }

    void OnMessageLoop() final
    {
        bool shouldContinue = true;
        while (shouldContinue) 
        {
            display_flush(Display);
            while (display_prepare_read(Display))
                display_dispatch_pending(Display);
            display_flush(Display);
            epoll_event evts[4];
            const auto cnt = epoll_wait(EpollFd, evts, 4, -1);
            bool hasWlMsg = false;
            if (cnt <= 0)
            {
                if (cnt == -1)
                    Logger.Warning(u"epoll error: {}\n", common::ErrnoHolder::GetCurError());
                continue;
            }
            for (int i = 0; i < cnt; ++i)
            {
                const auto& cookie = evts[i].data;
                if (cookie.fd == WaylandFd)
                {
                    hasWlMsg = true;
                    display_read_events(Display);
                    display_dispatch_pending(Display);
                    display_flush(Display);
                }
                else if (cookie.fd == DecorFd)
                {
                    Logger.Warning(u"decor dispatch\n");
                    if (LibDecor)
                        decor_dispatch(LibDecor, 0);
                }
                else if (cookie.fd == MsgQueueFd)
                    continue;
                else if (cookie.u32 & FdCookieComplex)
                {
                    Logger.Verbose(u"epoll event complex ([{:08x}]).\n", cookie.u32);
                    if (cookie.u32 == (FdCookieComplex | FdCookieClipboard)) // clipboard read
                    {
                        ClipboardTaskRead();
                    }
                    else if (cookie.u32 == (FdCookieComplex | FdCookieDrag)) // drag read
                    {
                        DragDropRead();
                    }
                }
            }
            if (!hasWlMsg)
            {
                display_cancel_read(Display);
            }
            Msg msg{};
            for (bool retried = false; !retried; retried = true)
            {
                uint64_t dummy = 0;
                read(MsgQueueFd, &dummy, 8);
                while (MsgQueue.pop(msg))
                {
                    //Logger.Verbose(u"Read Msg[{}] on window[{:x}].\n", msg.OpCode, (uintptr_t)msg.Host->Surface.Proxy);
                    switch(msg.OpCode)
                    {
                    case MessageStop:
                        shouldContinue = false;
                        break;
                    case MessageTask:
                        HandleTask();
                        break;
                    case MessageCreate:
                        CreateNewWindow_(*msg.Host, *reinterpret_cast<CreatePayload*>(msg.Ptr));
                        break;
                    case MessageClose:
                        Logger.Verbose(u"close message\n");
                        if (msg.Host->UseLibDecor)
                            decor_frame_close(msg.Host->DecorFrame);
                        display_flush(Display);
                        msg.Host->Stop();
                        break;
                    case MessageUpdTitle:
                    {
                        TitleLock lock(msg.Host);
                        SetTitle(*msg.Host, msg.Host->Title);
                    } break;
                    case MessageUpdIcon:
                    {
                        auto& icon = *GetWindowResource(msg.Host, WdAttrIndex::Icon);
                        if (icon)
                        {
                            IconLock lock(msg.Host);
                            auto& holder = static_cast<IconHolder&>(icon);
                            SetIcon(*msg.Host, holder.Extract());
                        }
                        else
                        {
                            SetIcon(*msg.Host, {});
                        }
                    } break;
                    case MessageClipboard:
                    {
                        const auto task = CurrentClipboard.GetFront();
                        if (CurrentClipboard.Offer && task)
                        {
                            Ensures(task->TypeIndex == 0);
                            Ensures(&task->Host == msg.Host);
                            Logger.Verbose(u"Clipboard task from window[{:x}] starts.\n", static_cast<WdHost&>(task->Host));
                            ClipboardTaskNewRead(*task);
                        }
                    } break;
                    default:
                        break;
                    }
                }
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

    void CreateNewWindow_(WdHost& host, CreatePayload& payload)
    {
        if (payload.ExtraData)
        {
            const auto bg_ = (*payload.ExtraData)("background");
            if (const auto bg = TryGetFinally<bool>(bg_); bg)
                host.NeedBackground = *bg;
        }
        host.UseLibDecor = !XdgDecor || (host.UseLibDecor && LibDecor);

        host.Surface = GetFrom<WL_COMPOSITOR_CREATE_SURFACE>(Compositor, ISurface);
        RegisterHost(host.Surface.Proxy, &host);
        payload.Promise.set_value(); // can release now

        host.LastSize = { host.Width, host.Height };
        if (host.UseLibDecor)
        {
            host.DecorFrame = decor_decorate(LibDecor, reinterpret_cast<wl_surface*>(host.Surface.Proxy), &host.DecorFrameCB, &host);
            SetTitle(host, host.Title);
            //decor_frame_set_capabilities(host.DecorFrame, static_cast<libdecor_capabilities>
            //    (LIBDECOR_ACTION_MOVE | LIBDECOR_ACTION_RESIZE | LIBDECOR_ACTION_MINIMIZE | LIBDECOR_ACTION_FULLSCREEN | LIBDECOR_ACTION_CLOSE));
            decor_frame_map(host.DecorFrame);
            //decor_frame_set_visibility(host.DecorFrame, true);
        }
        else
        {
            host.XdgSurface = GetFrom<XDG_WM_BASE_GET_XDG_SURFACE>(WmBase, IXdgSurface, host.Surface.Proxy);
            AddListener(host.XdgSurface, WdHost::ListenerXdgSurface, &host);
            host.Toplevel = GetFrom<XDG_SURFACE_GET_TOPLEVEL>(host.XdgSurface, IXdgToplevel);
            AddListener(host.Toplevel, WdHost::ListenerXdgToplevel, &host);
            host.ToplvDecor = GetFrom<ZXDG_DECORATION_MANAGER_V1_GET_TOPLEVEL_DECORATION>(XdgDecor, IXdgToplevelDecor, host.Toplevel.Proxy); // get_toplevel_decoration
            AddListener(host.ToplvDecor, WdHost::ListenerXdgToplvDecor, &host);
            MarshalFlags<void, ZXDG_TOPLEVEL_DECORATION_V1_SET_MODE>(host.ToplvDecor, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE); // zxdg_toplevel_decoration_v1_set_mode
            SetTitle(host, host.Title);
        }
        if (WlEGL)
        {
            host.EGLWindow = egl_window_create(reinterpret_cast<wl_surface*>(host.Surface.Proxy), host.Width, host.Height);
        }
        MarshalFlags<void, WL_SURFACE_COMMIT>(host.Surface); // wl_surface_commit
        display_roundtrip(Display);
    }
    void CreateNewWindow(CreatePayload& payload) final
    {
        SendControlMessage(static_cast<WdHost*>(payload.Host), MessageCreate, 0, &payload);
        payload.Promise.get_future().get();
    }
    void AfterWindowOpen(WindowHost_* host) const final
    {
    }
    void UpdateTitle(WindowHost_* host_) const final
    {
        SendControlMessage(static_cast<WdHost*>(host_), MessageUpdTitle);
    }
    void UpdateIcon(WindowHost_* host_) const final
    {
        if (!XdgIcon) return;
        SendControlMessage(static_cast<WdHost*>(host_), MessageUpdIcon);
    }
    void CloseWindow(WindowHost_* host) const final
    {
        SendControlMessage(static_cast<WdHost*>(host), MessageClose);
    }
    void ReleaseWindow(WindowHost_* host) final
    {
        auto& wd = *static_cast<WdHost*>(host);
        if (wd.EGLWindow)
            egl_window_destroy(wd.EGLWindow);
        for (auto& buf : wd.BackBuf)
            buf.ReleaseObjects(*this);
        if (wd.DecorFrame)
            decor_frame_unref(wd.DecorFrame);
        Destroy<ZXDG_TOPLEVEL_DECORATION_V1_DESTROY>(wd.ToplvDecor);
        Destroy<XDG_TOPLEVEL_DESTROY>(wd.Toplevel);
        Destroy<XDG_SURFACE_DESTROY>(wd.XdgSurface);
        Destroy<WL_SURFACE_DESTROY>(wd.Surface);
        display_flush(Display);
        UnregisterHost(host);
    }

    OpaqueResource PrepareIcon(WindowHost_& host, ImageView img) const noexcept final
    {
        if (!XdgIcon || !ShmDType)
            return {};
        if (!img.GetDataType().Is(ImgDType::DataTypes::Uint8))
            return {};
        const auto ow = img.GetWidth(), oh = img.GetHeight();
        if (!ow || !oh)
            return {};

        auto data = std::make_unique<IconData>();
        data->PrepareForSize(*this, PreferredIconSize, PreferredIconSize);
        img.ResizeTo(data->RealImage, 0, 0, 0, 0, PreferredIconSize, PreferredIconSize, true, true);
        data->Icon = GetFrom<XDG_TOPLEVEL_ICON_MANAGER_V1_CREATE_ICON>(XdgIcon, IXdgToplevelIcon);
        MarshalFlags<void, XDG_TOPLEVEL_ICON_V1_ADD_BUFFER>(data->Icon, data->Buf.Proxy, 1);

        return static_cast<OpaqueResource>(IconHolder(*this, std::move(data)));
    }
    OpaqueResource CacheRenderImage(WindowHost_& host_, ImageView img) const noexcept final
    {
        if (!ShmDType) return {};
        if (const auto w = img.GetWidth(), h = img.GetHeight(); w >= UINT16_MAX || h >= UINT16_MAX)
        {
            uint32_t neww = 0, newh = 0;
            (w > h ? neww : newh) = UINT16_MAX;
            img = img.ResizeTo(neww, newh, true, true);
        }
        if (img.GetDataType() != ShmDType->second)
        {
            img = img.ConvertTo(ShmDType->second);
        }
        return static_cast<OpaqueResource>(ImgHolder(img));
    }

    WindowHost Create(const CreateInfo& info_) final
    {
        WaylandCreateInfo info{};
        static_cast<CreateInfo&>(info) = info_;
        return Create(info);
    }
    std::shared_ptr<WaylandWdHost> Create(const WaylandCreateInfo& info) final
    {
        return std::make_shared<WdHost>(*this, info);
    }

    static inline const auto Dummy = RegisterBackend<WindowManagerWayland>();
};


void ImageBuffer::ReleaseObjects(const WindowManagerWayland& manager)
{
    RealImage = {};
    manager.Destroy<WL_BUFFER_DESTROY>(Buf);
    manager.Destroy<WL_SHM_POOL_DESTROY>(ShmPool);
    if (!Space.empty())
        munmap(Space.data(), Space.size());
}
void ImageBuffer::PrepareForSize(const WindowManagerWayland& manager, int32_t w, int32_t h)
{
    ReleaseObjects(manager);
    const auto stride = gsl::narrow_cast<int32_t>(w * manager.ShmDType->second.ElementSize());
    const auto size = gsl::narrow_cast<int32_t>(stride * h);
    ftruncate(Fd, size);
    Space = { reinterpret_cast<std::byte*>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, Fd, 0)), static_cast<size_t>(size) };
    ShmPool = manager.GetFrom<WL_SHM_CREATE_POOL>(manager.Shm, manager.IShmPool, Fd, size);
    Buf = manager.GetFrom<WL_SHM_POOL_CREATE_BUFFER>(ShmPool, manager.IBuffer, 0, w, h, stride, manager.ShmDType->first);
    RealImage = Image::CreateViewFromTemp(Space, manager.ShmDType->second, w, h);
}
void IconHolder::TheDisposer(const OpaqueResource& res) noexcept
{
    const auto& holder = static_cast<const IconHolder&>(res);
    auto& data = holder.Data();
    auto& manager = *reinterpret_cast<WindowManagerWayland*>(res.Cookie[0]);
    manager.Destroy<XDG_TOPLEVEL_ICON_V1_DESTROY>(data.Icon);
    data.ReleaseObjects(manager);
    delete &data;
}


void WindowManagerWayland::WdHost::OnDisplay(bool forceRedraw) noexcept
{
    auto& manager = GetManager();
    auto& backBuf = BackBuf[BackBufIdx];
    auto& rect = backBuf.Rect;
    common::SimpleTimer timer;
    timer.Start();
    backBuf.UseMutex.lock();
    timer.Stop();
    const auto lockBufTime = timer.ElapseNs() / 1000.f;
    const auto sizeChanged = backBuf.Update(*this);
    if (sizeChanged)
    {
        memset(backBuf.Space.data(), 0xff, backBuf.Space.size());
        //const auto ptr = reinterpret_cast<uint32_t*>(backBuf.Space.data());
        //for (int32_t h = 0; h < rect.Height; ++h)
        //{
        //    const auto hval = h * 255u / rect.Height;
        //    for (int32_t w = 0; w < rect.Width; ++w)
        //    {
        //        const auto wval = w * 255u / rect.Width;
        //        const auto clr = ((hval << 8) + wval) << (BackBufIdx ? 8 : 0);
        //        ptr[h * rect.Width + w] = 0xff000000u + clr;
        //    }
        //}
    }
    bool needDraw = false;
    {
        BgLock lock(this);
        const bool bgChanged = lock;
        const auto& holder = static_cast<ImgHolder&>(*GetWindowResource(this, WdAttrIndex::Background));
        if (bgChanged || BgChangedLasttime || sizeChanged)
        {
            common::SimpleTimer timer;
            timer.Start();
            memset(backBuf.Space.data(), 0xff, backBuf.Space.size());
            if (holder)
            {
                const auto& img = holder.Image();
                const auto [tw, th] = rect.ResizeWithin(img.GetWidth(), img.GetHeight());
                img.ResizeTo(backBuf.RealImage, 0, 0, 0, 0, tw, th, true, true);
                timer.Stop();
                Manager.Logger.Verbose(u"Window[{:x}] rebuild backbuffer in [{} ms].\n", *this, timer.ElapseMs());
            }
        }
        needDraw = forceRedraw || bgChanged || BgChangedLasttime || (sizeChanged && (NeedBackground || holder));
        if (needDraw)
            manager.Logger.Verbose(u"draw reason force[{}] bg[{}] lastbg[{}] size[{}].\n", forceRedraw, bgChanged, BgChangedLasttime, sizeChanged);
        BgChangedLasttime = bgChanged;
    }
    if (!needDraw)
        backBuf.UseMutex.unlock();
    if (needDraw || rect != LastSize)
    {
        timer.Start();
        ReSizingLock sizeLock(*this);
        timer.Stop();
        if (rect != LastSize) // update size
        {
            LastSize = rect;
            if (UseLibDecor)
            {
                auto state = manager.decor_state_new(LastSize.Width, LastSize.Height);
                manager.Logger.Verbose(u"update decor size to [{}].\n", LastSize);
                manager.decor_frame_commit(DecorFrame, state, nullptr);
                manager.decor_state_free(state);
            }
        }
        if (needDraw)
        {
            const auto lockSizeTime = timer.ElapseNs() / 1000.f;
            manager.Logger.Verbose(u"draw [{}] with lock Buf[{}us] Size[{}us].\n", BackBufIdx, lockBufTime, lockSizeTime);
            backBuf.Timer.Start();
            manager.MarshalFlags<void, WL_SURFACE_ATTACH>(Surface, backBuf.Buf.Proxy, 0, 0); // wl_surface_attach
            manager.MarshalFlags<void, WL_SURFACE_DAMAGE_BUFFER>(Surface, 0u, 0u, backBuf.Rect.Width, backBuf.Rect.Height); // wl_surface_damage_buffer
            const auto cb = manager.GetFrom<WL_SURFACE_FRAME>(Surface, manager.ICallback); // wl_surface_frame
            manager.AddListener(cb, ListenerFrame, this);
            manager.MarshalFlags<void, WL_SURFACE_COMMIT>(Surface); // wl_surface_commit
            BackBufIdx = BackBufIdx ^ 0b1u;
        }
    }
    WindowHost_::OnDisplay(forceRedraw);
    manager.display_flush(manager.Display);
}


}

#endif
